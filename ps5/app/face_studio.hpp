// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <vector>

namespace ps5sd {

class FaceStudio {
public:
    static FaceStudio & instance();

    // Initializes FaceStudio with the active faces directory.
    void init(const std::string & faces_dir, const std::string & models_dir);

    // Saves an uploaded image (PNG/JPEG) as the active reference face.
    bool set_reference_face(const std::string & image_bytes, std::string & error);

    // Checks if an active reference face is present.
    bool has_reference_face() const;

    // Path to the active reference face file.
    std::string face_path() const;

    // Clears the active reference face.
    bool clear_reference_face(std::string & error);

    // Status JSON: {"has_face": bool, "bytes": uint64, "photomaker_ready": bool}.
    std::string status_json() const;

    // Injects PhotoMaker or Face preservation arguments into model arguments and prompt.
    // Automatically appends the PhotoMaker trigger word 'img' if not already present.
    void inject_face_parameters(std::vector<std::string> & args, std::string & prompt) const;

private:
    FaceStudio() = default;

    std::string faces_dir_;
    std::string models_dir_;
    std::string find_photomaker_weights() const;
};

}  // namespace ps5sd
