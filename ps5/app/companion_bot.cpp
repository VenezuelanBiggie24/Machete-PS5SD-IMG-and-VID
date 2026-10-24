// SPDX-License-Identifier: GPL-3.0-or-later
#include "companion_bot.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>

#include "download_core.hpp"
#include "https_client.hpp"
#include "json.hpp"

namespace ps5sd {

namespace {

static std::string trim_str(const std::string & s) {
    size_t start = 0;
    while (start < s.size() && (std::isspace(static_cast<unsigned char>(s[start])) != 0)) {
        start++;
    }
    size_t end = s.size();
    while (end > start && (std::isspace(static_cast<unsigned char>(s[end - 1])) != 0)) {
        end--;
    }
    return s.substr(start, end - start);
}

static std::string to_lower_str(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

}  // namespace

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
    state_           = BotState::IDLE;

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

std::string CompanionBot::build_menu_markup() const {
    nlohmann::json reply_markup;
    nlohmann::json keyboard = nlohmann::json::array();

    if (state_ == BotState::AWAITING_PROMPT_IMG || state_ == BotState::AWAITING_PROMPT_VID || state_ == BotState::AWAITING_MODEL) {
        keyboard.push_back(nlohmann::json::array({ "Cancel" }));
    } else if (state_ == BotState::AWAITING_STEPS) {
        keyboard.push_back(nlohmann::json::array({ "10", "20", "30", "50" }));
        keyboard.push_back(nlohmann::json::array({ "Cancel" }));
    } else if (state_ == BotState::AWAITING_CFG) {
        keyboard.push_back(nlohmann::json::array({ "1.0", "3.5", "7.0", "12.0" }));
        keyboard.push_back(nlohmann::json::array({ "Cancel" }));
    } else if (state_ == BotState::AWAITING_SIZE) {
        keyboard.push_back(nlohmann::json::array({ "512x512", "768x768" }));
        keyboard.push_back(nlohmann::json::array({ "1024x1024", "1280x720" }));
        keyboard.push_back(nlohmann::json::array({ "832x480" }));
        keyboard.push_back(nlohmann::json::array({ "Cancel" }));
    } else if (state_ == BotState::AWAITING_FRAMES) {
        keyboard.push_back(nlohmann::json::array({ "17", "33", "49", "65", "81" }));
        keyboard.push_back(nlohmann::json::array({ "Cancel" }));
    } else {
        // Main menu
        keyboard.push_back(nlohmann::json::array({ "Generate Image", "Generate Video" }));
        keyboard.push_back(nlohmann::json::array({ "Models", "Parameters", "Status" }));
        keyboard.push_back(nlohmann::json::array({ "Help" }));
    }

    reply_markup["keyboard"]          = keyboard;
    reply_markup["resize_keyboard"]   = true;
    reply_markup["one_time_keyboard"] = false;
    return reply_markup.dump();
}

bool CompanionBot::send_message(const std::string & chat_id, const std::string & text, bool with_menu) {
    if (token_.empty() || chat_id.empty()) {
        return false;
    }

    nlohmann::json payload;
    payload["chat_id"] = chat_id;
    payload["text"]    = text;

    if (with_menu) {
        payload["reply_markup"] = nlohmann::json::parse(build_menu_markup(), nullptr, false);
    }

    HttpClient  client;
    client.set_timeout(20);
    std::string url = "https://api.telegram.org/bot" + token_ + "/sendMessage";
    std::string resp, error;
    int         status = 0;
    bool ok = http_post_bytes(client, url, "application/json", payload.dump(), resp, status, error);
    if (!ok || status != 200) {
        std::printf("ps5sd: companion_bot: sendMessage failed (status %d): %s\n", status, error.c_str());
        return false;
    }
    return true;
}

bool CompanionBot::send_photo(const std::string & chat_id, const std::string & photo_path, const std::string & caption) {
    if (token_.empty() || chat_id.empty() || photo_path.empty()) {
        return false;
    }

    std::ifstream file(photo_path, std::ios::binary);
    if (!file.is_open()) {
        std::printf("ps5sd: companion_bot: could not open %s for upload\n", photo_path.c_str());
        send_message(chat_id, "Generation finished! Saved to console at: " + photo_path);
        return false;
    }

    std::vector<char> file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (file_data.empty()) {
        send_message(chat_id, "Generation finished! Saved to console at: " + photo_path);
        return false;
    }

    const bool is_video = (photo_path.size() >= 5 && photo_path.substr(photo_path.size() - 5) == ".webm") ||
                          (photo_path.size() >= 4 && photo_path.substr(photo_path.size() - 4) == ".mp4");

    const std::string boundary = "----MacheteBotBoundary" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    std::string body;

    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
    body += chat_id + "\r\n";

    if (!caption.empty()) {
        std::string safe_caption = caption;
        if (safe_caption.size() > 900) {
            safe_caption = safe_caption.substr(0, 900) + "...";
        }
        body += "--" + boundary + "\r\n";
        body += "Content-Disposition: form-data; name=\"caption\"\r\n\r\n";
        body += safe_caption + "\r\n";
    }

    std::string filename = photo_path;
    const size_t slash = filename.find_last_of("/\\");
    if (slash != std::string::npos) {
        filename = filename.substr(slash + 1);
    }

    const std::string field_name = is_video ? "document" : "photo";
    const std::string mime_type  = is_video ? "video/webm" : "image/png";

    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"" + field_name + "\"; filename=\"" + filename + "\"\r\n";
    body += "Content-Type: " + mime_type + "\r\n\r\n";
    body.append(file_data.data(), file_data.size());
    body += "\r\n--" + boundary + "--\r\n";

    const std::string endpoint = is_video ? "/sendDocument" : "/sendPhoto";
    const std::string url      = "https://api.telegram.org/bot" + token_ + endpoint;

    HttpClient client;
    client.set_timeout(120);
    std::string resp, error;
    int         status = 0;
    bool ok = http_post_bytes(client, url, "multipart/form-data; boundary=" + boundary, body, resp, status, error);
    if (!ok || status != 200) {
        std::printf("ps5sd: companion_bot: photo upload failed (status %d): %s\n", status, error.c_str());
        send_message(chat_id, "Generation finished! Saved to console at:\n" + photo_path +
                              "\n(Telegram upload failed: " + (error.empty() ? std::to_string(status) : error) + ")");
        return false;
    }
    return true;
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
                        std::string raw_text      = msg.value("text", "");
                        std::string text          = trim_str(raw_text);

                        if (text.empty()) {
                            continue;
                        }

                        // Enforce single-owner chat authorization if configured
                        if (!allowed_chat_id_.empty() && chat_id != allowed_chat_id_) {
                            send_message(chat_id, "Unauthorized. Machete PS5SD companion is locked to its owner.", false);
                            continue;
                        }

                        // Universal cancel
                        if (text == "/cancel" || text == "Cancel") {
                            state_ = BotState::IDLE;
                            send_message(chat_id, "Action cancelled. Main menu restored.");
                            continue;
                        }

                        // Start / Help / Info
                        if (text == "/start" || text == "/help" || text == "Help") {
                            state_ = BotState::IDLE;
                            send_message(chat_id,
                                         "Machete PS5SD Studio Companion\n\n"
                                         "Hardware & Models:\n"
                                         "- Tap 'Status' to monitor console temperature, fan & power\n"
                                         "- Tap 'Models' to view and switch models by number\n"
                                         "- Tap 'Parameters' to adjust steps, CFG, size and frames\n\n"
                                         "Quick Generation:\n"
                                         "- Tap 'Generate Image' and enter a prompt\n"
                                         "- Tap 'Generate Video' and enter a video prompt\n"
                                         "- Or send: /gen <prompt> or /video <prompt>\n\n"
                                         "Tap any button below to begin:");
                            continue;
                        }

                        // Hardware status
                        if (text == "/status" || text == "Status") {
                            std::string st = status_fn_ ? status_fn_() : "Status: Online";
                            send_message(chat_id, st);
                            continue;
                        }

                        // Model listing and switching
                        if (text == "/models" || text == "Models") {
                            state_ = BotState::AWAITING_MODEL;
                            std::string m = models_fn_ ? models_fn_() : "No models found.";
                            send_message(chat_id, m);
                            continue;
                        }

                        if (state_ == BotState::AWAITING_MODEL) {
                            state_ = BotState::IDLE;
                            send_message(chat_id, "Switching model to \"" + text + "\"...\nPlease wait while PS5 loads weights.");
                            if (switch_fn_) {
                                std::string err;
                                if (switch_fn_(text, err)) {
                                    send_message(chat_id, "Model switched successfully: " + (err.empty() ? text : err));
                                } else {
                                    send_message(chat_id, "Failed to switch model: " + err);
                                }
                            }
                            continue;
                        }

                        if (text.rfind("/model", 0) == 0) {
                            std::string id = trim_str(text.substr(6));
                            if (id.empty()) {
                                state_ = BotState::AWAITING_MODEL;
                                std::string m = models_fn_ ? models_fn_() : "No models found.";
                                send_message(chat_id, m);
                                continue;
                            }
                            send_message(chat_id, "Switching model to \"" + id + "\"...\nPlease wait while PS5 loads weights.");
                            if (switch_fn_) {
                                std::string err;
                                if (switch_fn_(id, err)) {
                                    send_message(chat_id, "Model switched successfully: " + (err.empty() ? id : err));
                                } else {
                                    send_message(chat_id, "Failed to switch model: " + err);
                                }
                            }
                            continue;
                        }

                        // Parameters menu
                        if (text == "/params" || text == "Parameters") {
                            state_ = BotState::IDLE;
                            send_message(chat_id,
                                         "Current Parameters:\n"
                                         "- Steps: " + std::to_string(params_.steps) + "\n"
                                         "- CFG Scale: " + std::to_string(params_.cfg) + "\n"
                                         "- Resolution: " + std::to_string(params_.width) + "x" + std::to_string(params_.height) + "\n"
                                         "- Video Frames: " + std::to_string(params_.frames) + "\n\n"
                                         "To modify, send /steps, /cfg, /size, or /frames.");
                            continue;
                        }

                        // Steps setting
                        if (text == "Set Steps") {
                            state_ = BotState::AWAITING_STEPS;
                            send_message(chat_id, "Enter sampling steps (1-150) or select a preset:");
                            continue;
                        }
                        if (state_ == BotState::AWAITING_STEPS || text.rfind("/steps", 0) == 0) {
                            std::string val = (state_ == BotState::AWAITING_STEPS) ? text : trim_str(text.substr(6));
                            state_ = BotState::IDLE;
                            int s = std::atoi(val.c_str());
                            if (s >= 1 && s <= 150) {
                                params_.steps = s;
                                send_message(chat_id, "Sampling steps set to " + std::to_string(s) + ".");
                            } else {
                                send_message(chat_id, "Invalid steps. Please specify a number between 1 and 150.");
                            }
                            continue;
                        }

                        // CFG setting
                        if (text == "Set CFG") {
                            state_ = BotState::AWAITING_CFG;
                            send_message(chat_id, "Enter CFG scale (0.5 - 30.0) or select a preset:");
                            continue;
                        }
                        if (state_ == BotState::AWAITING_CFG || text.rfind("/cfg", 0) == 0) {
                            std::string val = (state_ == BotState::AWAITING_CFG) ? text : trim_str(text.substr(4));
                            state_ = BotState::IDLE;
                            float c = std::atof(val.c_str());
                            if (c >= 0.5f && c <= 30.0f) {
                                params_.cfg = c;
                                send_message(chat_id, "CFG scale set to " + val + ".");
                            } else {
                                send_message(chat_id, "Invalid CFG. Please specify a value between 0.5 and 30.0.");
                            }
                            continue;
                        }

                        // Size / Resolution setting
                        if (text == "Set Resolution" || text == "Set Size") {
                            state_ = BotState::AWAITING_SIZE;
                            send_message(chat_id, "Select or enter resolution as <width>x<height> (e.g. 1024x1024, 832x480):");
                            continue;
                        }
                        if (state_ == BotState::AWAITING_SIZE || text.rfind("/size", 0) == 0) {
                            std::string val = (state_ == BotState::AWAITING_SIZE) ? text : trim_str(text.substr(5));
                            state_ = BotState::IDLE;
                            size_t x = val.find('x');
                            if (x == std::string::npos) x = val.find('X');
                            if (x != std::string::npos) {
                                int w = std::atoi(val.substr(0, x).c_str());
                                int h = std::atoi(val.substr(x + 1).c_str());
                                if (w >= 64 && w <= 2048 && h >= 64 && h <= 2048) {
                                    params_.width  = w;
                                    params_.height = h;
                                    send_message(chat_id, "Resolution set to " + std::to_string(w) + "x" + std::to_string(h) + ".");
                                } else {
                                    send_message(chat_id, "Resolution must be between 64 and 2048.");
                                }
                            } else {
                                send_message(chat_id, "Usage: <width>x<height> (e.g. 1024x1024 or 832x480)");
                            }
                            continue;
                        }

                        // Video frames setting
                        if (text == "Set Video Frames") {
                            state_ = BotState::AWAITING_FRAMES;
                            send_message(chat_id, "Select or enter video frame count (5-161, e.g. 33 or 81):");
                            continue;
                        }
                        if (state_ == BotState::AWAITING_FRAMES || text.rfind("/frames", 0) == 0) {
                            std::string val = (state_ == BotState::AWAITING_FRAMES) ? text : trim_str(text.substr(7));
                            state_ = BotState::IDLE;
                            int f = std::atoi(val.c_str());
                            if (f >= 5 && f <= 161) {
                                params_.frames = f;
                                send_message(chat_id, "Video frames set to " + std::to_string(f) + ".");
                            } else {
                                send_message(chat_id, "Video frames must be between 5 and 161.");
                            }
                            continue;
                        }

                        // Interactive Generation Triggers
                        if (text == "Generate Image") {
                            state_ = BotState::AWAITING_PROMPT_IMG;
                            send_message(chat_id, "Mode: Image Generation\n"
                                                  "Settings: " + std::to_string(params_.width) + "x" + std::to_string(params_.height) +
                                                  ", " + std::to_string(params_.steps) + " steps, CFG " + std::to_string(params_.cfg) +
                                                  "\n\nPlease send your image prompt:\n(or tap Cancel)");
                            continue;
                        }

                        if (text == "Generate Video") {
                            state_ = BotState::AWAITING_PROMPT_VID;
                            send_message(chat_id, "Mode: Video Generation\n"
                                                  "Settings: " + std::to_string(params_.width) + "x" + std::to_string(params_.height) +
                                                  ", " + std::to_string(params_.frames) + " frames, " + std::to_string(params_.steps) + " steps"
                                                  "\n\nPlease send your video prompt:\n(or tap Cancel)");
                            continue;
                        }

                        // Prompt execution (either in awaiting state, slash command, or long plain text)
                        bool is_video_cmd = (state_ == BotState::AWAITING_PROMPT_VID || text.rfind("/video", 0) == 0);
                        bool is_img_cmd   = (state_ == BotState::AWAITING_PROMPT_IMG || text.rfind("/gen", 0) == 0);
                        bool is_direct_prompt = (state_ == BotState::IDLE && text.size() >= 12 && text[0] != '/');

                        if (is_video_cmd || is_img_cmd || is_direct_prompt) {
                            std::string prompt;
                            if (text.rfind("/video", 0) == 0) {
                                prompt = trim_str(text.substr(6));
                            } else if (text.rfind("/gen", 0) == 0) {
                                prompt = trim_str(text.substr(4));
                            } else {
                                prompt = text;
                            }

                            state_ = BotState::IDLE;

                            if (prompt.empty()) {
                                send_message(chat_id, "Prompt cannot be empty. Tap 'Generate Image' or 'Generate Video' to enter a prompt.");
                                continue;
                            }

                            CompanionParams call_params = params_;
                            if (is_video_cmd && call_params.width == 512 && call_params.height == 512) {
                                call_params.width  = 832;
                                call_params.height = 480;
                            }

                            send_message(chat_id, "Starting " + std::string(is_video_cmd ? "video" : "image") +
                                                  " generation on PS5 GPU (" + std::to_string(call_params.width) + "x" +
                                                  std::to_string(call_params.height) + ", " + std::to_string(call_params.steps) + " steps):\n\"" +
                                                  prompt + "\"\n\nPlease wait...");

                            if (gen_fn_) {
                                std::string gen_err;
                                std::string out_path = gen_fn_(prompt, call_params, gen_err);
                                if (!gen_err.empty()) {
                                    send_message(chat_id, "Generation failed: " + gen_err);
                                } else {
                                    send_photo(chat_id, out_path, prompt);
                                    send_message(chat_id, "Generation finished! Deliverable uploaded to your phone.");
                                }
                            } else {
                                send_message(chat_id, "Generation service unavailable.");
                            }
                            continue;
                        }

                        // Default catch-all for unrecognized input
                        state_ = BotState::IDLE;
                        send_message(chat_id, "Unrecognized command or input. Tap 'Generate Image' or 'Help' in the menu below.");
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
