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

static std::string html_escape(const std::string & s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        if (c == '<') {
            out += "&lt;";
        } else if (c == '>') {
            out += "&gt;";
        } else if (c == '&') {
            out += "&amp;";
        } else {
            out += c;
        }
    }
    return out;
}

static std::string friendly_model_label(int idx, const std::string & id) {
    std::string name = id;
    if (name.find("sdxl") != std::string::npos) {
        name = "SDXL Base";
    } else if (name.find("flux") != std::string::npos) {
        name = "FLUX Klein";
    } else if (name.find("photon") != std::string::npos) {
        name = "Photon v1";
    } else if (name.find("wan") != std::string::npos) {
        name = "Wan Video";
    } else if (name.find("ltx") != std::string::npos) {
        name = "LTX Video";
    } else if (name.size() > 14) {
        name = name.substr(0, 14);
    }
    return std::to_string(idx) + ". " + name;
}

static bool parse_resolution(const std::string & text, int & w, int & h) {
    for (size_t i = 1; i < text.size(); ++i) {
        if (text[i] == 'x' || text[i] == 'X') {
            size_t w_end = i;
            while (w_end > 0 && (text[w_end - 1] == ' ' || text[w_end - 1] == '\t')) {
                w_end--;
            }
            if (w_end == 0 || (std::isdigit(static_cast<unsigned char>(text[w_end - 1])) == 0)) {
                continue;
            }
            size_t w_start = w_end - 1;
            while (w_start > 0 && (std::isdigit(static_cast<unsigned char>(text[w_start - 1])) != 0)) {
                w_start--;
            }
            size_t h_start = i + 1;
            while (h_start < text.size() && (text[h_start] == ' ' || text[h_start] == '\t')) {
                h_start++;
            }
            if (h_start >= text.size() || (std::isdigit(static_cast<unsigned char>(text[h_start])) == 0)) {
                continue;
            }
            size_t h_end = h_start;
            while (h_end < text.size() && (std::isdigit(static_cast<unsigned char>(text[h_end])) != 0)) {
                h_end++;
            }
            int tw = std::atoi(text.substr(w_start, w_end - w_start).c_str());
            int th = std::atoi(text.substr(h_start, h_end - h_start).c_str());
            if (tw >= 64 && tw <= 2048 && th >= 64 && th <= 2048) {
                w = tw;
                h = th;
                return true;
            }
        }
    }
    return false;
}

static bool parse_steps(const std::string & text, int & steps) {
    for (size_t i = 0; i < text.size(); ++i) {
        if (std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
            size_t end = i;
            while (end < text.size() && (std::isdigit(static_cast<unsigned char>(text[end])) != 0)) {
                end++;
            }
            int s = std::atoi(text.substr(i, end - i).c_str());
            if (s >= 1 && s <= 150) {
                steps = s;
                return true;
            }
            i = end;
        }
    }
    return false;
}

static bool parse_cfg(const std::string & text, float & cfg) {
    for (size_t i = 0; i < text.size(); ++i) {
        if (std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
            size_t end = i;
            bool has_dot = false;
            while (end < text.size() && ((std::isdigit(static_cast<unsigned char>(text[end])) != 0) || (text[end] == '.' && !has_dot))) {
                if (text[end] == '.') {
                    has_dot = true;
                }
                end++;
            }
            float c = static_cast<float>(std::atof(text.substr(i, end - i).c_str()));
            if (c >= 0.5f && c <= 30.0f) {
                cfg = c;
                return true;
            }
            i = end;
        }
    }
    return false;
}

static bool parse_frames(const std::string & text, int & frames) {
    for (size_t i = 0; i < text.size(); ++i) {
        if (std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
            size_t end = i;
            while (end < text.size() && (std::isdigit(static_cast<unsigned char>(text[end])) != 0)) {
                end++;
            }
            int f = std::atoi(text.substr(i, end - i).c_str());
            if (f >= 5 && f <= 161) {
                frames = f;
                return true;
            }
            i = end;
        }
    }
    return false;
}

}  // namespace

CompanionBot & CompanionBot::instance() {
    static CompanionBot bot;
    return bot;
}

void CompanionBot::start(const std::string & token, const std::string & allowed_chat_id,
                         GenerateCallback gen_fn, StatusCallback status_fn,
                         ModelsCallback models_fn, SwitchCallback switch_fn,
                         ModelIdsCallback ids_fn) {
    if (running_) {
        stop();
    }
    token_           = token;
    allowed_chat_id_ = allowed_chat_id;
    gen_fn_          = std::move(gen_fn);
    status_fn_       = std::move(status_fn);
    models_fn_       = std::move(models_fn);
    switch_fn_       = std::move(switch_fn);
    ids_fn_          = std::move(ids_fn);
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

    if (state_ == BotState::AWAITING_PROMPT_IMG || state_ == BotState::AWAITING_PROMPT_VID) {
        keyboard.push_back(nlohmann::json::array({ "Cancelar" }));
    } else if (state_ == BotState::AWAITING_MODEL) {
        std::vector<std::string> ids;
        if (ids_fn_) {
            ids = ids_fn_();
        }
        if (!ids.empty()) {
            for (size_t i = 0; i < ids.size(); i += 2) {
                nlohmann::json row = nlohmann::json::array();
                row.push_back(friendly_model_label(static_cast<int>(i + 1), ids[i]));
                if (i + 1 < ids.size()) {
                    row.push_back(friendly_model_label(static_cast<int>(i + 2), ids[i + 1]));
                }
                keyboard.push_back(row);
            }
        }
        keyboard.push_back(nlohmann::json::array({ "Volver al Menu" }));
    } else if (state_ == BotState::SETTINGS_MENU) {
        keyboard.push_back(nlohmann::json::array({ "Resolucion", "Pasos (Steps)" }));
        keyboard.push_back(nlohmann::json::array({ "Escala CFG", "Fotogramas Video" }));
        keyboard.push_back(nlohmann::json::array({ "Volver al Menu" }));
    } else if (state_ == BotState::AWAITING_STEPS) {
        keyboard.push_back(nlohmann::json::array({ "4 pasos (Turbo Video/SDXL)", "8 pasos (Lightning)" }));
        keyboard.push_back(nlohmann::json::array({ "10 pasos (Rapido)", "20 pasos (Normal)" }));
        keyboard.push_back(nlohmann::json::array({ "30 pasos (Detallado)", "50 pasos (Ultra)" }));
        keyboard.push_back(nlohmann::json::array({ "Cancelar" }));
    } else if (state_ == BotState::AWAITING_CFG) {
        keyboard.push_back(nlohmann::json::array({ "1.0 (Flux)", "3.5 (Suave)" }));
        keyboard.push_back(nlohmann::json::array({ "7.0 (Recomendado)", "12.0 (Fuerte)" }));
        keyboard.push_back(nlohmann::json::array({ "Cancelar" }));
    } else if (state_ == BotState::AWAITING_SIZE) {
        keyboard.push_back(nlohmann::json::array({ "1024x1024 (Cuadrada)", "768x1024 (Retrato)" }));
        keyboard.push_back(nlohmann::json::array({ "1280x720 (Paisaje)", "832x480 (Video)" }));
        keyboard.push_back(nlohmann::json::array({ "512x512 (Rapida)", "Cancelar" }));
    } else if (state_ == BotState::AWAITING_FRAMES) {
        keyboard.push_back(nlohmann::json::array({ "17 cuadros (~1s)", "33 cuadros (~2s)" }));
        keyboard.push_back(nlohmann::json::array({ "49 cuadros (~3s)", "81 cuadros (~5s)" }));
        keyboard.push_back(nlohmann::json::array({ "Cancelar" }));
    } else {
        // Main menu
        keyboard.push_back(nlohmann::json::array({ "Crear Imagen", "Crear Video" }));
        keyboard.push_back(nlohmann::json::array({ "Modo Rapido (Turbo)", "Modelos" }));
        keyboard.push_back(nlohmann::json::array({ "Ajustes", "Estado Consola" }));
        keyboard.push_back(nlohmann::json::array({ "Ayuda" }));
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
    payload["chat_id"]    = chat_id;
    payload["text"]       = text;
    payload["parse_mode"] = "HTML";

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
        send_message(chat_id, "<b>Generacion completada:</b> Guardado en la consola en:\n<code>" + html_escape(photo_path) + "</code>");
        return false;
    }

    std::vector<char> file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (file_data.empty()) {
        send_message(chat_id, "<b>Generacion completada:</b> Guardado en la consola en:\n<code>" + html_escape(photo_path) + "</code>");
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
        std::string safe_caption = "Machete AI Studio (PS5)\nPrompt: " + caption;
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
        send_message(chat_id, "<b>Generacion completada:</b> Guardado en consola:\n<code>" + html_escape(photo_path) +
                              "</code>\n<i>(No se pudo subir a Telegram: " + html_escape(error.empty() ? std::to_string(status) : error) + ")</i>");
        return false;
    }
    return true;
}

bool CompanionBot::send_to_paired(const std::string & photo_path, const std::string & caption) {
    if (!running_ || token_.empty() || allowed_chat_id_.empty()) {
        return false;
    }
    return send_photo(allowed_chat_id_, photo_path, caption);
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

                        std::string lower = to_lower_str(text);

                        // Enforce single-owner chat authorization if configured
                        if (!allowed_chat_id_.empty() && chat_id != allowed_chat_id_) {
                            send_message(chat_id, "<b>No autorizado.</b> Este bot acompanante esta emparejado con la consola de su dueno.", false);
                            continue;
                        }

                        // Universal cancel
                        if (lower == "cancelar" || lower == "cancel" || lower == "/cancel" || lower == "volver al menu") {
                            state_ = BotState::IDLE;
                            send_message(chat_id, "<b>Operacion cancelada.</b> Has vuelto al menu principal.");
                            continue;
                        }

                        // Start / Help / Info
                        if (lower == "/start" || lower == "/help" || lower == "ayuda" || lower == "help") {
                            state_ = BotState::IDLE;
                            send_message(chat_id,
                                         "<b>Machete AI Studio • PS5 Companion</b>\n"
                                         "━━━━━━━━━━━━━━━━━━━━━━\n"
                                         "¡Bienvenido a tu estudio de Inteligencia Artificial para PlayStation 5!\n"
                                         "Genera imagenes y videos con la GPU de tu consola directamente desde tu celular.\n\n"
                                         "<b>Funciones Principales:</b>\n"
                                         "• Toca <b>Crear Imagen</b> y escribe lo que imaginas.\n"
                                         "• Toca <b>Crear Video</b> para animaciones con modelos Wan.\n"
                                         "• Toca <b>Modelos</b> para cambiar facilmente entre modelos instalados.\n"
                                         "• Toca <b>Ajustes</b> para cambiar resolucion o pasos.\n"
                                         "• Toca <b>Estado Consola</b> para ver temperatura y ventilador.\n\n"
                                         "━━━━━━━━━━━━━━━━━━━━━━\n"
                                         "Toca una opcion abajo para comenzar:");
                            continue;
                        }

                        // Hardware status
                        if (lower == "/status" || lower == "estado consola" || lower == "status" || lower == "estado") {
                            std::string st = status_fn_ ? status_fn_() : "<b>Estado:</b> En linea";
                            send_message(chat_id, st);
                            continue;
                        }

                        // Model listing and switching
                        if (lower == "/models" || lower == "modelos" || lower == "models") {
                            state_ = BotState::AWAITING_MODEL;
                            std::string m = models_fn_ ? models_fn_() : "No se encontraron modelos instalados.";
                            send_message(chat_id, m);
                            continue;
                        }

                        if (state_ == BotState::AWAITING_MODEL) {
                            state_ = BotState::IDLE;
                            send_message(chat_id, "<b>Cambiando modelo...</b>\nPor favor espera mientras la PS5 carga los pesos en la memoria.");
                            if (switch_fn_) {
                                std::string err;
                                if (switch_fn_(text, err)) {
                                    send_message(chat_id, "<b>¡Modelo cambiado con exito!</b>\nModelo activo: <code>" + html_escape(err.empty() ? text : err) + "</code>");
                                } else {
                                    send_message(chat_id, "<b>No se pudo cambiar el modelo:</b> " + html_escape(err));
                                }
                            }
                            continue;
                        }

                        if (text.rfind("/model", 0) == 0) {
                            std::string id = trim_str(text.substr(6));
                            if (id.empty()) {
                                state_ = BotState::AWAITING_MODEL;
                                std::string m = models_fn_ ? models_fn_() : "No se encontraron modelos.";
                                send_message(chat_id, m);
                                continue;
                            }
                            send_message(chat_id, "<b>Cambiando modelo...</b>\nPor favor espera mientras la PS5 carga los pesos en la memoria.");
                            if (switch_fn_) {
                                std::string err;
                                if (switch_fn_(id, err)) {
                                    send_message(chat_id, "<b>¡Modelo cambiado con exito!</b>\nModelo activo: <code>" + html_escape(err.empty() ? id : err) + "</code>");
                                } else {
                                    send_message(chat_id, "<b>No se pudo cambiar el modelo:</b> " + html_escape(err));
                                }
                            }
                            continue;
                        }

                        // Settings menu
                        if (lower == "/params" || lower == "ajustes" || lower == "parametros" || lower == "parameters") {
                            state_ = BotState::SETTINGS_MENU;
                            send_message(chat_id,
                                         "<b>Ajustes y Parametros de Generacion</b>\n"
                                         "━━━━━━━━━━━━━━━━━━━━━━\n"
                                         "• <b>Resolucion:</b> <code>" + std::to_string(params_.width) + "x" + std::to_string(params_.height) + "</code>\n"
                                         "• <b>Pasos de muestreo (Steps):</b> <code>" + std::to_string(params_.steps) + "</code>\n"
                                         "• <b>Escala de guia (CFG):</b> <code>" + std::to_string(params_.cfg) + "</code>\n"
                                         "• <b>Fotogramas de video:</b> <code>" + std::to_string(params_.frames) + "</code>\n"
                                         "━━━━━━━━━━━━━━━━━━━━━━\n"
                                         "Toca el ajuste que deseas modificar:");
                            continue;
                        }

                        // Steps setting
                        if (lower == "pasos (steps)" || lower == "pasos" || lower == "steps" || lower == "set steps") {
                            state_ = BotState::AWAITING_STEPS;
                            send_message(chat_id, "<b>Selecciona los pasos de muestreo (Steps):</b>\n<i>Mas pasos = mayor nitidez y detalle, pero toma mas tiempo.</i>\n\nToca una opcion abajo o escribe un numero (1 a 150):");
                            continue;
                        }
                        if (text.rfind("/steps", 0) == 0) {
                            std::string val = trim_str(text.substr(6));
                            if (val.empty()) {
                                state_ = BotState::AWAITING_STEPS;
                                send_message(chat_id, "<b>Selecciona los pasos de muestreo (Steps):</b>\n<i>Mas pasos = mayor nitidez y detalle.</i>\n\nToca una opcion abajo o escribe un numero (1 a 150):");
                                continue;
                            }
                            int s = 0;
                            if (parse_steps(val, s)) {
                                params_.steps = s;
                                state_ = BotState::IDLE;
                                send_message(chat_id, "<b>Pasos de muestreo actualizados a:</b> <code>" + std::to_string(s) + "</code>");
                            } else {
                                send_message(chat_id, "<b>Valor invalido.</b> Por favor especifica un numero entre 1 y 150.");
                            }
                            continue;
                        }
                        if (state_ == BotState::AWAITING_STEPS) {
                            int s = 0;
                            if (parse_steps(text, s)) {
                                params_.steps = s;
                                state_ = BotState::IDLE;
                                send_message(chat_id, "<b>Pasos de muestreo actualizados a:</b> <code>" + std::to_string(s) + "</code>");
                            } else {
                                send_message(chat_id, "<b>Valor invalido.</b> Por favor toca una opcion o escribe un numero entre 1 y 150.");
                            }
                            continue;
                        }

                        // CFG setting
                        if (lower == "escala cfg" || lower == "cfg" || lower == "set cfg") {
                            state_ = BotState::AWAITING_CFG;
                            send_message(chat_id, "<b>Selecciona la escala de guia (CFG):</b>\n<i>Controla que tan estrictamente la imagen sigue el texto.</i>\n\nToca una opcion abajo o escribe un valor (0.5 a 30.0):");
                            continue;
                        }
                        if (text.rfind("/cfg", 0) == 0) {
                            std::string val = trim_str(text.substr(4));
                            if (val.empty()) {
                                state_ = BotState::AWAITING_CFG;
                                send_message(chat_id, "<b>Selecciona la escala de guia (CFG):</b>\n<i>Controla que tan estrictamente la imagen sigue el texto.</i>\n\nToca una opcion abajo o escribe un valor (0.5 a 30.0):");
                                continue;
                            }
                            float c = 0.0f;
                            if (parse_cfg(val, c)) {
                                params_.cfg = c;
                                state_ = BotState::IDLE;
                                char buf[32];
                                std::snprintf(buf, sizeof(buf), "%.1f", c);
                                send_message(chat_id, "<b>Escala CFG actualizada a:</b> <code>" + std::string(buf) + "</code>");
                            } else {
                                send_message(chat_id, "<b>Valor invalido.</b> Por favor especifica un numero entre 0.5 y 30.0.");
                            }
                            continue;
                        }
                        if (state_ == BotState::AWAITING_CFG) {
                            float c = 0.0f;
                            if (parse_cfg(text, c)) {
                                params_.cfg = c;
                                state_ = BotState::IDLE;
                                char buf[32];
                                std::snprintf(buf, sizeof(buf), "%.1f", c);
                                send_message(chat_id, "<b>Escala CFG actualizada a:</b> <code>" + std::string(buf) + "</code>");
                            } else {
                                send_message(chat_id, "<b>Valor invalido.</b> Por favor toca una opcion o escribe un valor entre 0.5 y 30.0.");
                            }
                            continue;
                        }

                        // Size / Resolution setting
                        if (lower == "resolucion" || lower == "size" || lower == "set resolution" || lower == "set size") {
                            state_ = BotState::AWAITING_SIZE;
                            send_message(chat_id, "<b>Selecciona la resolucion deseada:</b>\nToca una de las opciones abajo o escribe <ancho>x<alto> (ej: <code>1024x1024</code> o <code>1280x720</code>):");
                            continue;
                        }
                        if (text.rfind("/size", 0) == 0) {
                            std::string val = trim_str(text.substr(5));
                            if (val.empty()) {
                                state_ = BotState::AWAITING_SIZE;
                                send_message(chat_id, "<b>Selecciona la resolucion deseada:</b>\nToca una de las opciones abajo o escribe <ancho>x<alto> (ej: <code>1024x1024</code> o <code>1280x720</code>):");
                                continue;
                            }
                            int w = 0, h = 0;
                            if (parse_resolution(val, w, h)) {
                                params_.width  = w;
                                params_.height = h;
                                state_ = BotState::IDLE;
                                send_message(chat_id, "<b>Resolucion configurada a:</b> <code>" + std::to_string(w) + "x" + std::to_string(h) + "</code>");
                            } else {
                                send_message(chat_id, "<b>Formato invalido.</b> Usa <ancho>x<alto> (ej: <code>1024x1024</code> o <code>1280x720</code>).");
                            }
                            continue;
                        }
                        if (state_ == BotState::AWAITING_SIZE) {
                            int w = 0, h = 0;
                            if (parse_resolution(text, w, h)) {
                                params_.width  = w;
                                params_.height = h;
                                state_ = BotState::IDLE;
                                send_message(chat_id, "<b>Resolucion configurada a:</b> <code>" + std::to_string(w) + "x" + std::to_string(h) + "</code>");
                            } else {
                                send_message(chat_id, "<b>Resolucion no valida.</b> Selecciona una de las opciones abajo o escribe <ancho>x<alto> (ej: <code>1024x1024</code> o <code>1280x720</code>). O toca <b>Cancelar</b>.");
                            }
                            continue;
                        }

                        // Video frames setting
                        if (lower == "fotogramas video" || lower == "frames" || lower == "set video frames") {
                            state_ = BotState::AWAITING_FRAMES;
                            send_message(chat_id, "<b>Selecciona la duracion del video (fotogramas):</b>\n\nToca una opcion abajo o escribe un numero (5 a 161):");
                            continue;
                        }
                        if (text.rfind("/frames", 0) == 0) {
                            std::string val = trim_str(text.substr(7));
                            if (val.empty()) {
                                state_ = BotState::AWAITING_FRAMES;
                                send_message(chat_id, "<b>Selecciona la duracion del video (fotogramas):</b>\n\nToca una opcion abajo o escribe un numero (5 a 161):");
                                continue;
                            }
                            int f = 0;
                            if (parse_frames(val, f)) {
                                params_.frames = f;
                                state_ = BotState::IDLE;
                                send_message(chat_id, "<b>Fotogramas de video configurados a:</b> <code>" + std::to_string(f) + "</code>");
                            } else {
                                send_message(chat_id, "<b>Cantidad invalida.</b> Debe estar entre 5 y 161.");
                            }
                            continue;
                        }
                        if (state_ == BotState::AWAITING_FRAMES) {
                            int f = 0;
                            if (parse_frames(text, f)) {
                                params_.frames = f;
                                state_ = BotState::IDLE;
                                send_message(chat_id, "<b>Fotogramas de video configurados a:</b> <code>" + std::to_string(f) + "</code>");
                            } else {
                                send_message(chat_id, "<b>Cantidad invalida.</b> Por favor toca una opcion o escribe un numero entre 5 y 161.");
                            }
                            continue;
                        }

                        // Universal Resolution Match: if any input contains a valid resolution (e.g. "1024x1024", "1280x720"), apply it!
                        int any_w = 0, any_h = 0;
                        if (parse_resolution(text, any_w, any_h)) {
                            params_.width  = any_w;
                            params_.height = any_h;
                            state_         = BotState::IDLE;
                            send_message(chat_id, "<b>Resolucion configurada a:</b> <code>" + std::to_string(any_w) + "x" + std::to_string(any_h) + "</code>");
                            continue;
                        }

                        // Interactive Generation Triggers
                        if (lower == "crear imagen" || lower == "generate image" || lower == "/image" || lower == "/imagen") {
                            state_ = BotState::AWAITING_PROMPT_IMG;
                            send_message(chat_id,
                                         "<b>Modo: Crear Imagen</b>\n"
                                         "━━━━━━━━━━━━━━━━━━━━━━\n"
                                         "<b>Ajustes:</b> " + std::to_string(params_.width) + "x" + std::to_string(params_.height) +
                                         " • " + std::to_string(params_.steps) + " pasos • CFG " + std::to_string(params_.cfg) + "\n\n"
                                         "Escribe en el chat lo que quieres que dibuje tu PS5:\n"
                                         "<i>(Ejemplo: \"un astronauta en un bosque bioluminiscente de noche\")</i>\n\n"
                                         "━━━━━━━━━━━━━━━━━━━━━━\n"
                                         "<i>(O toca <b>Cancelar</b> para volver al menu)</i>");
                            continue;
                        }

                        if (lower == "crear video" || lower == "generate video") {
                            state_ = BotState::AWAITING_PROMPT_VID;
                            send_message(chat_id,
                                         "<b>Modo: Crear Video</b>\n"
                                         "━━━━━━━━━━━━━━━━━━━━━━\n"
                                         "<b>Ajustes:</b> " + std::to_string(params_.width) + "x" + std::to_string(params_.height) +
                                         " • " + std::to_string(params_.frames) + " cuadros • " + std::to_string(params_.steps) + " pasos\n\n"
                                         "Escribe en el chat la escena que quieres animar en video:\n"
                                         "<i>(Ejemplo: \"un cometa cruzando el cielo estrellado con estela luminosa\")</i>\n\n"
                                         "━━━━━━━━━━━━━━━━━━━━━━\n"
                                         "<i>(O toca <b>Cancelar</b> para volver al menu)</i>");
                            continue;
                        }

                        if (lower == "modo rapido (turbo)" || lower == "turbo" || lower == "modo rapido" ||
                            text.rfind("/turbo", 0) == 0 || text.rfind("/fast", 0) == 0 || text.rfind("/rapido", 0) == 0) {
                            state_ = BotState::IDLE;
                            params_.steps = 4;
                            params_.cfg = 1.0f;
                            params_.width = 832;
                            params_.height = 480;
                            params_.frames = 33;
                            send_message(chat_id,
                                         "<b>MODO RAPIDO (TURBO) ACTIVADO</b>\n"
                                         "━━━━━━━━━━━━━━━━━━━━━━\n"
                                         "Configuracion hiper-optimizada para maxima velocidad:\n"
                                         "• <b>Pasos:</b> 4 (Aceleracion instantanea por LoRA)\n"
                                         "• <b>Escala CFG:</b> 1.0 (Sin pasos dobles redundantes)\n"
                                         "• <b>Resolucion:</b> 832x480 (Ideal para video ultrarrapido)\n"
                                         "• <b>Duracion:</b> 33 cuadros (~2 segundos)\n\n"
                                         "¡Listo! Ahora escribe directamente en el chat lo que deseas generar.");
                            continue;
                        }

                        // Prompt execution (either in awaiting state, slash command, or direct text prompt)
                        bool is_video_cmd = (state_ == BotState::AWAITING_PROMPT_VID || text.rfind("/video", 0) == 0);
                        bool is_img_cmd   = (state_ == BotState::AWAITING_PROMPT_IMG || text.rfind("/gen", 0) == 0);
                        bool is_nav_phrase = (lower == "menu" || lower == "inicio" || lower == "ajustes" || lower == "configuracion" ||
                                              lower == "modelos" || lower == "estado consola" || lower == "ayuda" ||
                                              lower == "modo rapido (turbo)" || lower == "turbo" || lower == "modo rapido");
                        bool is_direct_prompt = (state_ == BotState::IDLE && text.size() >= 2 && text[0] != '/' && !is_nav_phrase);

                        if (is_video_cmd || is_img_cmd || is_direct_prompt) {
                            std::string prompt;
                            if (text.rfind("/video", 0) == 0) {
                                prompt = trim_str(text.substr(6));
                            } else if (text.rfind("/gen", 0) == 0) {
                                prompt = trim_str(text.substr(4));
                            } else {
                                prompt = text;
                            }

                            if (prompt.empty()) {
                                if (text.rfind("/video", 0) == 0) {
                                    state_ = BotState::AWAITING_PROMPT_VID;
                                    send_message(chat_id, "<b>Modo: Crear Video</b>\nPor favor escribe la escena que deseas animar:");
                                } else {
                                    state_ = BotState::AWAITING_PROMPT_IMG;
                                    send_message(chat_id, "<b>Modo: Crear Imagen</b>\nPor favor escribe la descripcion de la imagen que deseas generar:");
                                }
                                continue;
                            }

                            state_ = BotState::IDLE;

                            CompanionParams call_params = params_;
                            if (is_video_cmd && call_params.width == 1024 && call_params.height == 1024) {
                                call_params.width  = 832;
                                call_params.height = 480;
                            }

                            send_message(chat_id,
                                         "<b>Generando en tu PlayStation 5...</b>\n"
                                         "━━━━━━━━━━━━━━━━━━━━━━\n"
                                         "<b>Prompt:</b> <i>\"" + html_escape(prompt) + "\"</i>\n"
                                         "<b>Resolucion:</b> " + std::to_string(call_params.width) + "x" + std::to_string(call_params.height) +
                                         " | <b>Pasos:</b> " + std::to_string(call_params.steps) + "\n\n"
                                         "La GPU de tu consola esta procesando la solicitud. En breve recibiras el resultado directamente aqui...");

                            if (gen_fn_) {
                                std::string gen_err;
                                std::string out_path = gen_fn_(prompt, call_params, gen_err);
                                if (!gen_err.empty()) {
                                    send_message(chat_id, "<b>Error en la generacion:</b> " + html_escape(gen_err));
                                } else {
                                    send_photo(chat_id, out_path, prompt);
                                    send_message(chat_id, "<b>¡Generacion completada y entregada con exito!</b>");
                                }
                            } else {
                                send_message(chat_id, "<b>Servicio de generacion no disponible en este momento.</b>");
                            }
                            continue;
                        }

                        // Default friendly catch-all
                        state_ = BotState::IDLE;
                        send_message(chat_id,
                                     "<b>¡Hola!</b> Bienvenido a Machete AI Studio en tu PS5.\n\n"
                                     "¿Que deseas hacer?\n"
                                     "• Toca <b>Crear Imagen</b> y escribe lo que imaginas.\n"
                                     "• Toca <b>Modelos</b> para ver los modelos instalados.\n"
                                     "• Toca <b>Ajustes</b> para cambiar resolucion o pasos.");
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
