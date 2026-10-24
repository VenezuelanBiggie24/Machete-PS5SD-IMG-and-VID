// SPDX-License-Identifier: GPL-3.0-or-later
#include "face_studio.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include "json.hpp"

namespace fs = std::filesystem;

namespace ps5sd {

FaceStudio & FaceStudio::instance() {
    static FaceStudio studio;
    return studio;
}

void FaceStudio::init(const std::string & faces_dir, const std::string & models_dir) {
    faces_dir_  = faces_dir;
    models_dir_ = models_dir;
    std::error_code ec;
    fs::create_directories(fs::u8path(faces_dir_), ec);
}

std::string FaceStudio::face_path() const {
    if (faces_dir_.empty()) {
        return "/data/MacheteAI/faces/active_face.png";
    }
    return (fs::u8path(faces_dir_) / "active_face.png").u8string();
}

bool FaceStudio::has_reference_face() const {
    std::error_code ec;
    const fs::path p = fs::u8path(face_path());
    return fs::exists(p, ec) && fs::is_regular_file(p, ec) && fs::file_size(p, ec) > 0;
}

bool FaceStudio::set_reference_face(const std::string & image_bytes, std::string & error) {
    if (image_bytes.empty()) {
        error = "image payload is empty";
        return false;
    }
    std::error_code ec;
    fs::create_directories(fs::u8path(faces_dir_), ec);
    const std::string path = face_path();
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        error = "cannot open " + path + " for writing";
        return false;
    }
    out.write(image_bytes.data(), static_cast<std::streamsize>(image_bytes.size()));
    out.close();
    return true;
}

bool FaceStudio::clear_reference_face(std::string & error) {
    std::error_code ec;
    const fs::path p = fs::u8path(face_path());
    if (fs::exists(p, ec)) {
        fs::remove(p, ec);
        if (ec) {
            error = "cannot remove " + p.u8string() + ": " + ec.message();
            return false;
        }
    }
    return true;
}

std::string FaceStudio::find_photomaker_weights() const {
    const std::string candidates[] = {
        (fs::u8path(models_dir_) / "photomaker-v1.safetensors").u8string(),
        (fs::u8path(models_dir_) / "photomaker-v2.safetensors").u8string(),
        "/data/MacheteAI/models/photomaker-v1.safetensors",
        "/data/MacheteAI/models/photomaker-v2.safetensors",
        "/data/PS5SD/models/photomaker-v1.safetensors",
        "/data/PS5SD/models/photomaker-v2.safetensors"
    };
    std::error_code ec;
    for (const auto & c : candidates) {
        if (fs::exists(fs::u8path(c), ec) && fs::is_regular_file(fs::u8path(c), ec)) {
            return c;
        }
    }
    return "";
}

std::string FaceStudio::status_json() const {
    const bool has = has_reference_face();
    uint64_t bytes = 0;
    std::error_code ec;
    if (has) {
        bytes = static_cast<uint64_t>(fs::file_size(fs::u8path(face_path()), ec));
    }
    const std::string pm_weights = find_photomaker_weights();
    nlohmann::json out = {
        { "has_face", has },
        { "bytes", bytes },
        { "photomaker_ready", !pm_weights.empty() },
        { "photomaker_model", pm_weights }
    };
    return out.dump();
}

void FaceStudio::inject_face_parameters(std::vector<std::string> & args, std::string & prompt) const {
    if (!has_reference_face()) {
        return;
    }
    const std::string pm_weights = find_photomaker_weights();
    if (!pm_weights.empty()) {
        args.push_back("--photo-maker");
        args.push_back(pm_weights);
        args.push_back("--pm-id-images-dir");
        args.push_back(faces_dir_);
        args.push_back("--pm-style-strength");
        args.push_back("10");

        // The PhotoMaker trigger word 'img' links the portrait conditioning to the prompt
        if (prompt.find("img") == std::string::npos) {
            prompt = prompt + " img";
        }
    } else {
        // Fallback for standard diffusion / inpainting pipelines
        args.push_back("--init-img");
        args.push_back(face_path());
        args.push_back("--strength");
        args.push_back("0.45");
    }
}

}  // namespace ps5sd
