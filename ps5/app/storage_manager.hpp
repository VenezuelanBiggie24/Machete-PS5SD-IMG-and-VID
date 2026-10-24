// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ps5sd {

struct StorageDevice {
    std::string mount_path;
    std::string label;
    uint64_t    total_bytes = 0;
    uint64_t    free_bytes  = 0;
    bool        is_active   = false;
    bool        is_usb      = false;
    bool        has_models  = false;
};

class StorageManager {
public:
    static StorageManager & instance();

    // Resolves and initializes active storage paths.
    void init(const std::string & custom_data_dir = "");

    // The active base directory (e.g. /data/MacheteAI, /data/PS5SD, or /mnt/usb0/MacheteAI).
    const std::string & base_dir() const { return base_dir_; }

    // Derived paths under the active base directory.
    std::string models_dir() const;
    std::string presets_dir() const;
    std::string loras_dir() const;
    std::string outputs_dir() const;
    std::string upscalers_dir() const;
    std::string embeddings_dir() const;
    std::string tags_dir() const;
    std::string faces_dir() const;

    std::string log_file() const;
    std::string crash_log_file() const;
    std::string settings_file() const;
    std::string model_file() const;
    std::string quit_file() const;
    std::string api_key_file() const;
    std::string app_args_file() const;
    std::string server_args_file() const;
    std::string env_file() const;

    // Returns all directories where models should be searched.
    // Primary models directory first, followed by fallback directories
    // (e.g. /data/PS5SD/models and any detected USB model folders).
    std::vector<std::string> search_model_dirs() const;

    // Scans connected USB drives (/mnt/usb0 .. /mnt/usb7) and internal storage (/data).
    std::vector<StorageDevice> list_devices() const;

    // Switches the active base directory to a specific path (e.g. a USB mount).
    bool set_active_storage(const std::string & mount_path, std::string & error);

    // Creates all essential subdirectories in the active base directory.
    bool ensure_directories(std::string & error) const;

private:
    StorageManager() = default;
    std::string resolve_default_base() const;

    std::string base_dir_;
};

}  // namespace ps5sd
