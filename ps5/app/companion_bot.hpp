// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace ps5sd {

struct CompanionParams {
    int   steps  = 20;
    float cfg    = 7.0f;
    int   width  = 1024;
    int   height = 1024;
    int   frames = 33;
};

enum class BotState {
    IDLE,
    AWAITING_PROMPT_IMG,
    AWAITING_PROMPT_VID,
    SETTINGS_MENU,
    AWAITING_STEPS,
    AWAITING_CFG,
    AWAITING_SIZE,
    AWAITING_FRAMES,
    AWAITING_MODEL
};

class CompanionBot {
public:
    static CompanionBot & instance();

    using GenerateCallback = std::function<std::string(const std::string & prompt, const CompanionParams & params, std::string & error)>;
    using StatusCallback   = std::function<std::string()>;
    using ModelsCallback   = std::function<std::string()>;
    using SwitchCallback   = std::function<bool(const std::string & model_id, std::string & error)>;
    using ModelIdsCallback = std::function<std::vector<std::string>()>;

    // Configures and starts the background Telegram companion bot thread.
    void start(const std::string & token, const std::string & allowed_chat_id,
               GenerateCallback gen_fn, StatusCallback status_fn,
               ModelsCallback models_fn = nullptr, SwitchCallback switch_fn = nullptr,
               ModelIdsCallback ids_fn = nullptr);

    // Stops the bot thread.
    void stop();

    bool is_running() const { return running_; }

    // Sends an image directly to the paired Telegram user.
    bool send_to_paired(const std::string & photo_path, const std::string & caption);

private:
    CompanionBot() = default;
    ~CompanionBot() { stop(); }

    void poll_loop();
    bool send_message(const std::string & chat_id, const std::string & text, bool with_menu = true);
    bool send_photo(const std::string & chat_id, const std::string & photo_path, const std::string & caption);
    std::string build_menu_markup() const;

    std::string token_;
    std::string allowed_chat_id_;
    GenerateCallback gen_fn_;
    StatusCallback   status_fn_;
    ModelsCallback   models_fn_;
    SwitchCallback   switch_fn_;
    ModelIdsCallback ids_fn_;

    CompanionParams params_;
    BotState        state_{ BotState::IDLE };

    std::atomic<bool> running_{ false };
    std::thread       worker_;
    int64_t           last_update_id_{ 0 };
};

}  // namespace ps5sd
