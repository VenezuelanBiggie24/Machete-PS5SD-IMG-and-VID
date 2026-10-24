// Adapted from PS5LM (github.com/cobanov/PS5LM, GPL-3.0)
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>

// The app's version, semver. sce_sys/param.json's contentVersion follows it
// the way PS5LM's release check expects (major+1, minor, patch: 0.0.0 is
// 01.000.000); bump both together.
#ifndef PS5SD_VERSION
#define PS5SD_VERSION "0.9.6"
#endif

#ifndef PS5SD_BUILD
#define PS5SD_BUILD "build release"
#endif

namespace ps5sd {
// Returns the active version string, resolving dynamically from param.json if available.
std::string get_app_version();

// Returns the build identifier string.
std::string get_app_build();
}
