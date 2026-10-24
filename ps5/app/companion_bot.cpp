// SPDX-License-Identifier: GPL-3.0-or-later
#include "companion_bot.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>

#include "download_core.hpp"
#include "https_client.hpp"
#include "json.hpp"

namespace ps5sd {

CompanionBot & CompanionBot::instance() {
    static CompanionBot bot;
    return bot;
}

void CompanionBot::start(const std::string & token, const std::string & allowed_chat_id,
                         GenerateCallback gen_fn, StatusCallback status_fn,
                         ModelsCallback models_fn, SwitchCallback switch_fn) {
    if (running_) {
        stop();
    }
    token_           = token;
    allowed_chat_id_ = allowed_chat_id;
    gen_fn_          = std::move(gen_fn);
    status_fn_       = std::move(status_fn);
    models_fn_       = std::move(models_fn);
    switch_fn_       = std::move(switch_fn);

    if (token_.empty()) {
        return;
    }

    running_ = true;
    worker_  = std::thread(&CompanionBot::poll_loop, this);
    std::printf("ps5sd: Telegram companion bot worker started\n");
}

void CompanionBot::stop() {
    if (running_) {
        running_ = false;
        if (worker_.joinable()) {
            worker_.join();
        }
        std::printf("ps5sd: Telegram companion bot worker stopped\n");
    }
}

bool CompanionBot::send_message(const std::string & chat_id, const std::string & text) {
    if (token_.empty() || chat_id.empty()) {
        return false;
    }
    HttpClient  client;
    std::string url = "https://api.telegram.org/bot" + token_ + "/sendMessage?chat_id=" + chat_id +
                      "&text=" + url_encode(text);
    std::string resp, error;
    int         status = 0;
    return http_get_text(client, url, [](const Url &) { return HttpHeaders{}; }, 64u << 10, resp, status, error) &&
           status == 200;
}

bool CompanionBot::send_photo(const std::string & chat_id, const std::string & photo_path, const std::string & caption) {
    if (token_.empty() || chat_id.empty()) {
        return false;
    }
    // Inform the user that the generation finished
    return send_message(chat_id, "Generation finished! Saved to: " + photo_path + (caption.empty() ? "" : "\nPrompt: " + caption));
}

void CompanionBot::poll_loop() {
    HttpClient client;
    client.set_timeout(20);

    while (running_) {
        std::string url = "https://api.telegram.org/bot" + token_ +
                          "/getUpdates?timeout=5&offset=" + std::to_string(last_update_id_ + 1);
        std::string resp, error;
        int         status = 0;

        if (http_get_text(client, url, [](const Url &) { return HttpHeaders{}; }, 256u << 10, resp, status, error) &&
            status == 200 && !resp.empty()) {
            try {
                auto j = nlohmann::json::parse(resp, nullptr, false);
                if (j.is_object() && j.value("ok", false) && j.contains("result") && j["result"].is_array()) {
                    for (const auto & update : j["result"]) {
                        const int64_t update_id = update.value("update_id", 0LL);
                        if (update_id > last_update_id_) {
                            last_update_id_ = update_id;
                        }

                        if (!update.contains("message")) {
                            continue;
                        }
                        const auto & msg = update["message"];
                        const std::string chat_id = std::to_string(msg["chat"].value("id", 0LL));
                        const std::string text    = msg.value("text", "");

                        // Enforce single-owner chat authorization if configured
                        if (!allowed_chat_id_.empty() && chat_id != allowed_chat_id_) {
                            send_message(chat_id, "Unauthorized. Machete PS5SD companion is locked to its owner.");
                            continue;
                        }

                        if (text == "/start" || text == "/help") {
                            send_message(chat_id,
                                         "Machete PS5SD Studio Companion\n\n"
                                         "Generation:\n"
                                         "/gen <prompt> - generate image with current settings\n"
                                         "/video <prompt> - generate video with current settings\n\n"
                                         "Model Selection:\n"
                                         "/models - list installed models (image & video)\n"
                                         "/model <id> - switch active model (e.g. /model sdxl-base)\n\n"
                                         "Settings & Tuning:\n"
                                         "/status - console temperature, power, fan and load\n"
                                         "/params - view current generation parameters\n"
                                         "/steps <n> - set steps (1-150)\n"
                                         "/cfg <n> - set CFG scale (0.5-30.0)\n"
                                         "/size <w>x<h> - set resolution (e.g. 1024x1024)\n"
                                         "/frames <n> - set video frames (e.g. 33 or 81)");
                        } else if (text == "/status") {
                            std::string st = status_fn_ ? status_fn_() : "Status: Online";
                            send_message(chat_id, st);
                        } else if (text == "/models") {
                            std::string m = models_fn_ ? models_fn_() : "No models found.";
                            send_message(chat_id, m);
                        } else if (text.rfind("/model", 0) == 0) {
                            std::string id = text.substr(6);
                            while (!id.empty() && id.front() == ' ') id.erase(0, 1);
                            while (!id.empty() && id.back() == ' ') id.pop_back();
                            if (id.empty()) {
                                send_message(chat_id, "Usage: /model <model_id>\nSend /models to view available models.");
                                continue;
                            }
                            send_message(chat_id, "Switching to model \"" + id + "\"...\nPlease wait while PS5 loads weights.");
                            if (switch_fn_) {
                                std::string err;
                                if (switch_fn_(id, err)) {
                                    send_message(chat_id, "Model switch initiated: " + id);
                                } else {
                                    send_message(chat_id, "Failed to switch model: " + err);
                                }
                            }
                        } else if (text == "/params") {
                            send_message(chat_id,
                                         "Current Parameters:\n"
                                         "Steps: " + std::to_string(params_.steps) + "\n"
                                         "CFG Scale: " + std::to_string(params_.cfg) + "\n"
                                         "Resolution: " + std::to_string(params_.width) + "x" + std::to_string(params_.height) + "\n"
                                         "Video Frames: " + std::to_string(params_.frames));
                        } else if (text.rfind("/steps", 0) == 0) {
                            std::string val = text.substr(6);
                            int s = std::atoi(val.c_str());
                            if (s >= 1 && s <= 150) {
                                params_.steps = s;
                                send_message(chat_id, "Steps set to " + std::to_string(s) + ".");
                            } else {
                                send_message(chat_id, "Please specify steps between 1 and 150.");
                            }
                        } else if (text.rfind("/cfg", 0) == 0) {
                            std::string val = text.substr(4);
                            float c = std::atof(val.c_str());
                            if (c >= 0.5f && c <= 30.0f) {
                                params_.cfg = c;
                                send_message(chat_id, "CFG scale set to " + val + ".");
                            } else {
                                send_message(chat_id, "Please specify CFG between 0.5 and 30.0.");
                            }
                        } else if (text.rfind("/size", 0) == 0) {
                            std::string val = text.substr(5);
                            while (!val.empty() && val.front() == ' ') val.erase(0, 1);
                            size_t x = val.find('x');
                            if (x == std::string::npos) x = val.find('X');
                            if (x != std::string::npos) {
                                int w = std::atoi(val.substr(0, x).c_str());
                                int h = std::atoi(val.substr(x + 1).c_str());
                                if (w >= 64 && w <= 2048 && h >= 64 && h <= 2048) {
                                    params_.width = w;
                                    params_.height = h;
                                    send_message(chat_id, "Resolution set to " + std::to_string(w) + "x" + std::to_string(h) + ".");
                                } else {
                                    send_message(chat_id, "Resolution must be between 64 and 2048.");
                                }
                            } else {
                                send_message(chat_id, "Usage: /size <width>x<height> (e.g. /size 1024x1024 or /size 832x480)");
                            }
                        } else if (text.rfind("/frames", 0) == 0) {
                            std::string val = text.substr(7);
                            int f = std::atoi(val.c_str());
                            if (f >= 5 && f <= 161) {
                                params_.frames = f;
                                send_message(chat_id, "Video frames set to " + std::to_string(f) + ".");
                            } else {
                                send_message(chat_id, "Video frames must be between 5 and 161 (e.g. 33 or 81).");
                            }
                        } else if (text.rfind("/video", 0) == 0 || text.rfind("/gen", 0) == 0 || (!text.empty() && text[0] != '/')) {
                            bool is_video_cmd = (text.rfind("/video", 0) == 0);
                            std::string prompt;
                            if (is_video_cmd) {
                                prompt = text.substr(6);
                            } else if (text.rfind("/gen", 0) == 0) {
                                prompt = text.substr(4);
                            } else {
                                prompt = text;
                            }
                            while (!prompt.empty() && prompt.front() == ' ') prompt.erase(0, 1);
                            if (prompt.empty()) {
                                send_message(chat_id, "Please provide a prompt after /gen or /video");
                                continue;
                            }

                            // Copy session parameters and parse any inline overrides
                            CompanionParams call_params = params_;
                            if (is_video_cmd && call_params.width == 512 && call_params.height == 512) {
                                call_params.width = 832;
                                call_params.height = 480;
                            }

                            send_message(chat_id, "Starting " + std::string(is_video_cmd ? "video" : "image") +
                                         " generation on PS5 GPU (" + std::to_string(call_params.width) + "x" +
                                         std::to_string(call_params.height) + ", " + std::to_string(call_params.steps) + " steps):\n\"" + prompt + "\"...");

                            if (gen_fn_) {
                                std::string gen_err;
                                std::string out_path = gen_fn_(prompt, call_params, gen_err);
                                if (!gen_err.empty()) {
                                    send_message(chat_id, "Generation failed: " + gen_err);
                                } else {
                                    send_photo(chat_id, out_path, prompt);
                                }
                            }
                        }
                    }
                }
            } catch (...) {
                // Ignore parse errors from temporary malformed response
            }
        }

        // Sleep briefly between polling intervals
        for (int i = 0; i < 20 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

}  // namespace ps5sd
