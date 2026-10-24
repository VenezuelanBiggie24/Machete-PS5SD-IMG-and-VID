// SPDX-License-Identifier: GPL-3.0-or-later
#include "storage_manager.hpp"

#include <sys/statvfs.h>
#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace ps5sd {

StorageManager & StorageManager::instance() {
    static StorageManager mgr;
    return mgr;
}

std::string StorageManager::resolve_default_base() const {
    const char * env_data = std::getenv("PS5SD_DATA");
    if (env_data && env_data[0] != '\0') {
        return env_data;
    }
    std::error_code ec;
    // If user already has an existing /data/PS5SD directory, use it to seamlessly reuse models
    if (fs::exists("/data/PS5SD", ec) && fs::is_directory("/data/PS5SD", ec)) {
        return "/data/PS5SD";
    }
    // Default persistent location for Machete on console storage
    return "/data/MacheteAI";
}

void StorageManager::init(const std::string & custom_data_dir) {
    if (!custom_data_dir.empty()) {
        base_dir_ = custom_data_dir;
    } else {
        base_dir_ = resolve_default_base();
    }
    std::string err;
    ensure_directories(err);
}

std::string StorageManager::models_dir() const {
    return (fs::u8path(base_dir_) / "models").u8string();
}

std::string StorageManager::presets_dir() const {
    return (fs::u8path(base_dir_) / "presets").u8string();
}

std::string StorageManager::loras_dir() const {
    return (fs::u8path(base_dir_) / "loras").u8string();
}

std::string StorageManager::outputs_dir() const {
    return (fs::u8path(base_dir_) / "outputs").u8string();
}

std::string StorageManager::upscalers_dir() const {
    return (fs::u8path(base_dir_) / "upscalers").u8string();
}

std::string StorageManager::embeddings_dir() const {
    return (fs::u8path(base_dir_) / "embeddings").u8string();
}

std::string StorageManager::tags_dir() const {
    return (fs::u8path(base_dir_) / "tags").u8string();
}

std::string StorageManager::faces_dir() const {
    return (fs::u8path(base_dir_) / "faces").u8string();
}

std::string StorageManager::log_file() const {
    return (fs::u8path(base_dir_) / "app.log").u8string();
}

std::string StorageManager::crash_log_file() const {
    return (fs::u8path(base_dir_) / "crash.log").u8string();
}

std::string StorageManager::settings_file() const {
    return (fs::u8path(base_dir_) / "settings.json").u8string();
}

std::string StorageManager::model_file() const {
    return (fs::u8path(base_dir_) / "model.txt").u8string();
}

std::string StorageManager::quit_file() const {
    return (fs::u8path(base_dir_) / "quit").u8string();
}

std::string StorageManager::api_key_file() const {
    return (fs::u8path(base_dir_) / "api-key.txt").u8string();
}

std::string StorageManager::app_args_file() const {
    return (fs::u8path(base_dir_) / "app-args.txt").u8string();
}

std::string StorageManager::server_args_file() const {
    return (fs::u8path(base_dir_) / "server-args.txt").u8string();
}

std::string StorageManager::env_file() const {
    return (fs::u8path(base_dir_) / "env.txt").u8string();
}

std::vector<std::string> StorageManager::search_model_dirs() const {
    std::vector<std::string> dirs;
    dirs.push_back(models_dir());

    std::error_code ec;
    // Fallback: check /data/PS5SD/models if active base is not /data/PS5SD
    if (base_dir_ != "/data/PS5SD") {
        const std::string alt = "/data/PS5SD/models";
        if (fs::exists(fs::u8path(alt), ec) && fs::is_directory(fs::u8path(alt), ec)) {
            dirs.push_back(alt);
        }
    }
    // Also scan all connected USB drives for models
    for (int i = 0; i < 8; ++i) {
        const std::string usb_root = "/mnt/usb" + std::to_string(i);
        if (fs::exists(fs::u8path(usb_root), ec) && fs::is_directory(fs::u8path(usb_root), ec)) {
            const std::string usb_models1 = (fs::u8path(usb_root) / "MacheteAI" / "models").u8string();
            const std::string usb_models2 = (fs::u8path(usb_root) / "models").u8string();
            const std::string usb_models3 = (fs::u8path(usb_root) / "PS5SD" / "models").u8string();
            for (const auto & candidate : { usb_models1, usb_models2, usb_models3 }) {
                if (candidate != models_dir() && fs::exists(fs::u8path(candidate), ec)) {
                    dirs.push_back(candidate);
                }
            }
        }
    }
    return dirs;
}

std::vector<StorageDevice> StorageManager::list_devices() const {
    std::vector<StorageDevice> devices;
    std::error_code ec;

    // Internal console data storage
    {
        StorageDevice dev;
        dev.mount_path = "/data";
        dev.label      = "Internal Console Storage (/data)";
        dev.is_usb     = false;
        dev.is_active  = (base_dir_.rfind("/data", 0) == 0);
        struct statvfs st{};
        if (statvfs("/data", &st) == 0) {
            dev.total_bytes = static_cast<uint64_t>(st.f_blocks) * st.f_frsize;
            dev.free_bytes  = static_cast<uint64_t>(st.f_bavail) * st.f_frsize;
        }
        dev.has_models = fs::exists(models_dir(), ec) || fs::exists("/data/PS5SD/models", ec);
        devices.push_back(dev);
    }

    // USB mount points /mnt/usb0 .. /mnt/usb7
    for (int i = 0; i < 8; ++i) {
        const std::string usb_mount = "/mnt/usb" + std::to_string(i);
        if (fs::exists(fs::u8path(usb_mount), ec) && fs::is_directory(fs::u8path(usb_mount), ec)) {
            StorageDevice dev;
            dev.mount_path = usb_mount;
            dev.label      = "USB Drive " + std::to_string(i) + " (" + usb_mount + ")";
            dev.is_usb     = true;
            dev.is_active  = (base_dir_.rfind(usb_mount, 0) == 0);
            struct statvfs st{};
            if (statvfs(usb_mount.c_str(), &st) == 0) {
                dev.total_bytes = static_cast<uint64_t>(st.f_blocks) * st.f_frsize;
                dev.free_bytes  = static_cast<uint64_t>(st.f_bavail) * st.f_frsize;
            }
            dev.has_models = fs::exists((fs::u8path(usb_mount) / "MacheteAI" / "models"), ec) ||
                             fs::exists((fs::u8path(usb_mount) / "models"), ec) ||
                             fs::exists((fs::u8path(usb_mount) / "PS5SD" / "models"), ec);
            devices.push_back(dev);
        }
    }

    return devices;
}

bool StorageManager::set_active_storage(const std::string & mount_path, std::string & error) {
    std::error_code ec;
    if (mount_path.empty()) {
        error = "empty mount path";
        return false;
    }
    if (!fs::exists(fs::u8path(mount_path), ec)) {
        error = "path does not exist: " + mount_path;
        return false;
    }
    if (mount_path == "/data") {
        base_dir_ = resolve_default_base();
    } else {
        base_dir_ = (fs::u8path(mount_path) / "MacheteAI").u8string();
    }
    return ensure_directories(error);
}

bool StorageManager::ensure_directories(std::string & error) const {
    std::error_code ec;
    const std::string dirs[] = {
        base_dir_,
        models_dir(),
        presets_dir(),
        loras_dir(),
        outputs_dir(),
        upscalers_dir(),
        embeddings_dir(),
        tags_dir(),
        faces_dir()
    };
    for (const auto & d : dirs) {
        fs::create_directories(fs::u8path(d), ec);
        if (ec) {
            error = "cannot create directory " + d + ": " + ec.message();
            return false;
        }
    }
    return true;
}

}  // namespace ps5sd
