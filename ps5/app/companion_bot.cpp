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
                         GenerateCallback gen_fn, StatusCallback status_fn) {
    if (running_) {
        stop();
    }
    token_           = token;
    allowed_chat_id_ = allowed_chat_id;
    gen_fn_          = std::move(gen_fn);
    status_fn_       = std::move(status_fn);

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
                                         "Machete PS5SD Studio Companion active!\n"
                                         "Commands:\n"
                                         "/gen <prompt> - generate image or video on PS5 GPU\n"
                                         "/status - view console temperatures, power and status");
                        } else if (text == "/status") {
                            std::string st = status_fn_ ? status_fn_() : "Status: Online";
                            send_message(chat_id, st);
                        } else if (text.rfind("/gen", 0) == 0 || (!text.empty() && text[0] != '/')) {
                            std::string prompt = text.rfind("/gen", 0) == 0 ? text.substr(4) : text;
                            while (!prompt.empty() && prompt.front() == ' ') prompt.erase(0, 1);
                            if (prompt.empty()) {
                                send_message(chat_id, "Please provide a prompt after /gen");
                                continue;
                            }
                            send_message(chat_id, "Starting generation on PS5 GPU: \"" + prompt + "\"...");
                            if (gen_fn_) {
                                std::string gen_err;
                                std::string out_path = gen_fn_(prompt, gen_err);
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
