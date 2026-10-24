// SPDX-License-Identifier: GPL-3.0-or-later
#include "dashboard.hpp"

#include "core/save_file.hpp"
#include "gfx/renderer.hpp"
#include "image_file.hpp"
#include "qrcodegen.hpp"
#include "ui/components/component.hpp"

#include <GL/glcorearb.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

namespace ps5sd {

namespace {

using hui::gfx::Align;
using hui::gfx::Color;
using hui::gfx::DrawList;
using hui::gfx::Rect;
using State = AppStatus::State;

// ---- the design language (after the kit's Pulse dashboard) --------------------

const Color kInk       = Color::rgb(0xf2f5fc);
const Color kClear     = Color::rgb(0x000000, 0.0f);
const Color kBlack     = Color::rgb(0x000000);
const Color kPanelTop  = Color::rgb(0x111728);
const Color kPanelBot  = Color::rgb(0x090d18);
const Color kWell      = Color::rgb(0x050710);
const Color kCyan      = Color::rgb(0x00f0ff);
const Color kLime      = Color::rgb(0x00ff9d);
const Color kAmber     = Color::rgb(0xffb703);
const Color kRose      = Color::rgb(0xff3366);
const Color kViolet    = Color::rgb(0x9d4edd);
const Color kSteel     = Color::rgb(0x718096);

constexpr float kMuted = 0.65f;  // alpha of secondary text
constexpr float kFaint = 0.40f;  // alpha of tertiary text and empty states

// Modern Cinema Studio layout: Left column (640px) for Connect, Engine Status,
// and Hardware Telemetry; Right column (1064px) for Hero Artwork Canvas and
// Recent Creations filmstrip.
constexpr Rect kServer{ 96, 148, 640, 232 };
constexpr Rect kActivity{ 96, 400, 640, 336 };
constexpr Rect kMemory{ 96, 756, 640, 236 };
constexpr Rect kImage{ 760, 148, 1064, 600 };
constexpr Rect kRecent{ 760, 768, 1064, 224 };
constexpr Rect kLogTile{ 0, 0, 0, 0 };
constexpr float kRadius = 24.0f;
constexpr float kPad    = 28.0f;
constexpr int   kThumbs = 6;

constexpr const char * kDot = "\xC2\xB7";  // middle dot

Color with_a(Color c, float a) {
    c.a = a;
    return c;
}

// A reading for the console line, or a dash while it is not read.
std::string reading(double v, const char * format, double scale = 1.0) {
    if (std::isnan(v)) {
        return "\xE2\x80\x93";  // en dash
    }
    char text[24];
    std::snprintf(text, sizeof(text), format, v * scale);
    return text;
}

std::string seconds_text(double s) {
    char text[32];
    if (s < 0) {
        return "-";
    }
    if (s < 60) {
        std::snprintf(text, sizeof(text), s < 10 ? "%.1f s" : "%.0f s", s);
    } else {
        const int whole = (int) (s + 0.5);
        std::snprintf(text, sizeof(text), "%d:%02d", whole / 60, whole % 60);
    }
    return text;
}

// sd.cpp's log lines start "[INFO ] stable-diffusion.cpp:2340 - "; on a
// small tile the words matter more.
std::string log_text(const std::string & line) {
    if (!line.empty() && line[0] == '[') {
        const std::size_t sep = line.find(" - ");
        if (sep != std::string::npos && sep < 64) {
            return line.substr(sep + 3);
        }
    }
    return line;
}

// A rectangle of the aspect w:h fitted (contained) in `area`.
Rect contain(const Rect & area, int w, int h) {
    if (w <= 0 || h <= 0) {
        return area;
    }
    const float scale = std::min(area.w / (float) w, area.h / (float) h);
    const float fw    = (float) w * scale;
    const float fh    = (float) h * scale;
    return { area.cx() - fw * 0.5f, area.cy() - fh * 0.5f, fw, fh };
}

hui::ui::Theme make_theme() {
    hui::ui::Theme t = hui::ui::default_theme();
    t.id             = "machete_studio";
    t.name           = "Machete Studio Pro";
    t.family         = "Obsidian Glass";
    t.summary        = "Next-Gen Commercial AI Studio UI";
    t.page           = Color::rgb(0x070913);
    t.surface        = Color::rgb(0x101626);
    t.surface_high   = Color::rgb(0x1d273e);
    t.text           = kInk;
    t.text_muted     = with_a(kInk, kMuted);
    t.primary        = kCyan;
    t.on_primary     = Color::rgb(0x070913);
    t.secondary      = t.surface_high;
    t.on_secondary   = kInk;
    t.accent         = kCyan;
    t.outline        = with_a(kCyan, 0.20f);
    t.focus          = kCyan;
    t.shadow         = with_a(kBlack, 0.6f);
    t.light          = with_a(kCyan, 0.25f);
    t.danger         = kRose;
    t.success        = kLime;
    t.warning        = kAmber;
    t.style          = hui::ui::SurfaceStyle::flat;
    t.corner         = hui::ui::Corner::round;
    t.radius         = 12.0f;
    t.radius_card    = kRadius;
    t.border         = 1.5f;
    t.shadow_offset  = 0.0f;
    t.shadow_blur    = 0.0f;
    t.pill_chips     = true;
    t.heading        = hui::ui::FontRole::display;
    t.label          = hui::ui::FontRole::semibold;
    t.caps           = false;
    t.tracking       = 0.0f;
    t.omega          = 16.0f;
    t.damping        = 1.0f;
    t.dark           = true;
    return t;
}

}  // namespace

Dashboard::Dashboard() = default;

Dashboard::~Dashboard() {
    if (loaded_) {
        release(preview_);
        for (Picture & p : pictures_) {
            release(p.thumb);
            release(p.large);
        }
    }
}

bool Dashboard::load_font(const std::string & path, hui::gfx::Font & font, hui::ui::FontRef & ref) {
    std::string data;
    if (!hui::save::read_file(path, &data) || !font.load(data)) {
        std::printf("ps5sd: font %s failed\n", path.c_str());
        return false;
    }
    ref.font    = &font;
    ref.texture = renderer_->batch().create_font_texture(font);
    return true;
}

bool Dashboard::load(hui::gfx::Renderer & renderer, const std::string & font_path) {
    renderer_ = &renderer;
    const std::size_t slash = font_path.find_last_of('/');
    const std::string dir   = slash == std::string::npos ? std::string(".") : font_path.substr(0, slash);
    if (!load_font(dir + "/inter-regular.huifont", regular_font_, fonts_.regular) ||
        !load_font(dir + "/inter-semibold.huifont", semibold_font_, fonts_.semibold) ||
        !load_font(dir + "/montserrat-medium.huifont", display_font_, fonts_.display) ||
        !load_font(font_path, mono_font_, fonts_.mono)) {
        return false;
    }
    // The components look their fonts up by role; the two faces the dashboard
    // does not use stand in for a loaded one so no role is ever empty.
    fonts_.pixel = fonts_.regular;
    fonts_.hand  = fonts_.regular;

    theme_ = make_theme();
    backdrop_.mode      = hui::gfx::BackdropMode::dots;
    backdrop_.colors[0] = Color::rgb(0x04060d);
    backdrop_.colors[1] = Color::rgb(0x080d1c);
    backdrop_.colors[2] = Color::rgb(0x18243c);

    state_chip_.style.theme       = theme_;
    state_chip_.style.leading_dot = true;
    state_chip_.style.height      = 46;
    state_chip_.style.text_size   = 19;
    state_chip_.style.padding     = 22;
    state_chip_.style.align       = Align::right;
    state_chip_.set_bounds({ 1824 - 420, 70, 420, 46 });

    progress_.style.theme     = theme_;
    progress_.style.height    = 22;
    progress_.style.text_size = 20;
    progress_.style.text_gap  = 12;
    progress_.style.sheen     = true;

    // The memory: a bar of what is free, coloured by how much is left.
    memory_.style.theme        = theme_;
    memory_.style.height       = 14;
    memory_.style.percent      = false;
    memory_.style.placement    = hui::ui::LabelPlacement::none;
    memory_.style.finish_flash = false;

    spinner_.style.theme = theme_;
    spinner_.style.kind  = hui::ui::SpinnerKind::arc;

    empty_.style.theme          = theme_;
    empty_.style.icon_size      = 96;
    empty_.style.title_size     = 32;
    empty_.style.body_size      = 22;
    empty_.style.max_text_width = 560;
    empty_.style.max_lines      = 3;
    empty_.style.panel          = false;
    empty_.icon = [this](hui::ui::Canvas & canvas, const Rect & area) {
        const bool busy = snap_.state == State::starting || snap_.state == State::loading ||
                          snap_.state == State::generating;
        if (busy) {
            hui::ui::Spinner spinner = spinner_;
            spinner.style.color      = with_a(accent_.value(), 1.0f);
            spinner.style.status     = hui::ui::Status::accent;
            spinner.set_bounds(area);
            spinner.draw(canvas);
            return;
        }
        DrawList & list   = canvas.list;
        const Color line  = with_a(kCyan, 0.45f);
        const Rect  frame = area.inset(area.w * 0.12f);
        list.bordered_rect(frame, 20, kClear, 3.0f, line);
        list.circle(frame.cx(), frame.cy(), frame.w * 0.22f, with_a(kViolet, 0.6f));
        list.circle(frame.cx(), frame.cy(), frame.w * 0.10f, with_a(kCyan, 0.9f));
    };
                      with_a(kInk, 0.16f));
    };

    accent_.snap(state_color(State::starting));
    accent_set_ = true;
    reveal_.snap(1.0f);
    loaded_ = true;
    return true;
}

// ---- state to look -------------------------------------------------------------

Color Dashboard::state_color(State state) const {
    switch (state) {
        case State::ready:      return kLime;
        case State::generating: return kCyan;
        case State::error:      return kRose;
        case State::stopped:    return kSteel;
        case State::starting:
        case State::loading:
        default:                return kAmber;
    }
}

// ---- textures ------------------------------------------------------------------

void Dashboard::upload(Texture & texture, int width, int height, const uint8_t * rgba) {
    release(texture);
    texture.id     = renderer_->batch().create_texture(width, height, rgba);
    texture.width  = width;
    texture.height = height;
}

void Dashboard::release(Texture & texture) {
    if (texture.id != 0) {
        const GLuint id = texture.id;
        glDeleteTextures(1, &id);
    }
    texture = Texture{};
}

void Dashboard::sync_preview(const AppStatus::Snapshot & s) {
    if (!s.preview) {
        release(preview_);
        preview_serial_ = 0;
        return;
    }
    if (s.preview_serial == preview_serial_ && preview_.id != 0) {
        return;
    }
    const AppStatus::Frame & f = *s.preview;
    const std::size_t        n = (std::size_t) f.width * (std::size_t) f.height;
    if (f.rgb.size() < n * 3) {
        return;
    }
    scratch_.resize(n * 4);
    for (std::size_t i = 0; i < n; i++) {
        scratch_[i * 4 + 0] = f.rgb[i * 3 + 0];
        scratch_[i * 4 + 1] = f.rgb[i * 3 + 1];
        scratch_[i * 4 + 2] = f.rgb[i * 3 + 2];
        scratch_[i * 4 + 3] = 255;
    }
    upload(preview_, f.width, f.height, scratch_.data());
    preview_serial_ = s.preview_serial;
}

// The finished images the snapshot lists, newest first: kept in step, and
// decoded one per frame (a 1024 px PNG takes a few tens of milliseconds on
// the console; one a frame keeps a burst of results from stalling it).
void Dashboard::sync_pictures(const AppStatus::Snapshot & s) {
    const std::size_t keep = std::min<std::size_t>(s.recent.size(), kThumbs);
    std::vector<Picture> next;
    for (std::size_t i = 0; i < keep; i++) {
        const AppStatus::Result & wanted = s.recent[i];
        auto it = std::find_if(pictures_.begin(), pictures_.end(), [&](const Picture & p) { return p.path == wanted.path; });
        if (it != pictures_.end()) {
            next.push_back(std::move(*it));
            pictures_.erase(it);
        } else {
            Picture p;
            p.path = wanted.path;
            next.push_back(std::move(p));
        }
        next.back().prompt = wanted.prompt;
    }
    for (Picture & gone : pictures_) {  // what fell out of the list
        release(gone.thumb);
        release(gone.large);
    }
    pictures_ = std::move(next);

    for (std::size_t i = 0; i < pictures_.size(); i++) {
        Picture & p = pictures_[i];
        if (i != 0) {
            release(p.large);  // only the newest is shown large
        }
        if (p.failed) {
            continue;
        }
        const bool need_large = i == 0 && p.large.id == 0;
        if (p.thumb.id != 0 && !need_large) {
            continue;
        }
        Rgba image;
        if (!load_image_rgba(p.path, need_large ? 1024 : 256, image)) {
            std::printf("ps5sd: dashboard: cannot read %s\n", p.path.c_str());
            p.failed = true;
            continue;
        }
        if (need_large) {
            upload(p.large, image.width, image.height, image.pixels.data());
        }
        if (p.thumb.id == 0) {
            const Rgba small = shrink_to(image, 256);
            upload(p.thumb, small.width, small.height, small.pixels.data());
        }
        break;  // one decode a frame
    }
}

// ---- the frame's state ------------------------------------------------------------

void Dashboard::update(const AppStatus::Snapshot & s, float dt) {
    if (!loaded_) {
        return;
    }
    dt    = std::clamp(dt, 0.0f, 0.1f);
    snap_ = s;
    if (!s.tv_images) {
        // The switch in the web UI: no pictures on the TV. Nothing is decoded
        // either, and what was shown is let go.
        snap_.preview.reset();
        snap_.recent.clear();
        screensaver_active_ = false;
        idle_time_ = 0.0f;
    }
    clock_ += dt;
    age_ += dt;

    if (s.state == State::generating) {
        idle_time_ = 0.0f;
        screensaver_active_ = false;
    } else if (s.tv_images && !pictures_.empty()) {
        idle_time_ += dt;
        if (idle_time_ >= 60.0f) {
            screensaver_active_ = true;
        }
    }

    accent_.target(state_color(s.state));
    accent_.update(dt, 6.0f);
    sync_preview(snap_);
    sync_pictures(snap_);
    sync_qr();

    std::string key = "none";
    if (s.state == State::generating && preview_.id != 0) {
        key = "preview";
    } else if (!pictures_.empty() && pictures_[0].large.id != 0) {
        key = "pic:" + pictures_[0].path;
    }
    if (key != shown_key_) {
        shown_key_ = key;
        reveal_.snap(key == "preview" || key == "none" ? 1.0f : 0.0f);
    }
    reveal_.target = 1.0f;
    reveal_.update(dt, 7.0f);

    sync_components(dt);
}

void Dashboard::settle() {
    age_ = 10.0f;
    accent_.snap(state_color(snap_.state));
    reveal_.snap(1.0f);
}

void Dashboard::sync_components(float dt) {
    const AppStatus::Snapshot & s = snap_;
    const Color accent            = accent_.value();

    // The chip in the header.
    static const char * const kNames[] = { "STARTING", "LOADING", "READY", "GENERATING", "ERROR", "STOPPED" };
    state_chip_.label = kNames[(int) s.state];
    switch (s.state) {
        case State::ready:      state_chip_.style.dot = hui::ui::Status::success; break;
        case State::generating: state_chip_.style.dot = hui::ui::Status::accent;  break;
        case State::error:      state_chip_.style.dot = hui::ui::Status::danger;  break;
        case State::stopped:    state_chip_.style.dot = hui::ui::Status::neutral; break;
        default:                state_chip_.style.dot = hui::ui::Status::warning; break;
    }
    state_chip_.update(dt);

    // The bar: loading counts tensors, sampling counts steps.
    const bool sampling = s.state == State::generating;
    const bool loading  = s.state == State::loading || s.state == State::starting;
    const bool show     = sampling || (loading && s.state == State::loading);
    progress_.style.color = with_a(accent, 1.0f);
    if (show) {
        const Rect tile = kActivity;
        progress_.set_bounds({ tile.x + kPad, tile.y + 204, tile.w - 2 * kPad, 52 });
        char label[64];
        if (sampling) {
            if (s.steps > 0 && s.step <= 0) {
                std::snprintf(label, sizeof(label), "Preparing the first step");
                progress_.style.mode = hui::ui::ProgressMode::indeterminate;
            } else if (s.steps > 0 && s.step >= s.steps) {
                std::snprintf(label, sizeof(label), "Decoding the image");
                progress_.style.mode = hui::ui::ProgressMode::determinate;
                progress_.set_value(1.0f, !progress_shown_);
            } else if (s.steps > 0) {
                std::snprintf(label, sizeof(label), "Step %d of %d", s.step, s.steps);
                progress_.style.mode = hui::ui::ProgressMode::determinate;
                progress_.set_value((float) s.step / (float) s.steps, !progress_shown_);
            } else {
                std::snprintf(label, sizeof(label), "Working");
                progress_.style.mode = hui::ui::ProgressMode::indeterminate;
            }
        } else if (s.steps > 0) {
            std::snprintf(label, sizeof(label), "Loading the model");
            progress_.style.mode = hui::ui::ProgressMode::determinate;
            progress_.set_value((float) s.step / (float) s.steps, !progress_shown_);
        } else {
            std::snprintf(label, sizeof(label), "Loading the model");
            progress_.style.mode = hui::ui::ProgressMode::indeterminate;
        }
        progress_.label = label;
    }
    progress_shown_ = show;
    progress_.update(dt);

    // Memory: a bar of what is free, turning amber and then red as it runs out.
    memory_.set_bounds({ kMemory.x + 24, kMemory.y + 70, kMemory.w - 48, 14 });
    if (s.free_gib >= 0 && s.pool_gib > 0) {
        const float free = (float) std::clamp(s.free_gib / s.pool_gib, 0.0, 1.0);
        memory_.set_value(free);
        memory_.style.color = free > 0.3f ? kLime : (free > 0.12f ? kAmber : kRose);
    } else {
        memory_.set_value(0.0f, true);
    }
    memory_.update(dt);

    spinner_.update(dt);

    // The empty picture area.
    switch (s.tv_images ? s.state : State::stopped) {
        case State::starting:
        case State::loading:
            empty_.title = "Initializing Studio Engine";
            empty_.body  = "Loading model weights into APU memory. Your renders appear here.";
            break;
        case State::generating:
            empty_.title = "Generating Masterpiece";
            empty_.body  = "This request is processing on the GPU. The render will display here.";
            break;
        case State::error:
            empty_.title = "Engine Alert";
            empty_.body  = "Check system logs or /data/PS5SD/app.log for details.";
            break;
        case State::stopped:
            if (!s.tv_images) {
                empty_.title = "Studio Privacy Mode";
                empty_.body  = "Renders are hidden on TV. Toggle in Web UI Settings to show.";
                break;
            }
            empty_.title = "Studio Standby";
            empty_.body.clear();
            break;
        default:
            empty_.title = "Machete Cinema Studio";
            empty_.body  = "Scan the QR code or open Machete Web UI to begin generating on PS5 GPU.";
            break;
    }
    empty_.set_bounds({ kImage.x + 20, kImage.y + 54, kImage.w - 40, kImage.h - 74 });
    empty_.update(dt);
}

// ---- drawing ---------------------------------------------------------------------

hui::ui::Canvas Dashboard::canvas(DrawList & list) const {
    return hui::ui::Canvas{ list, fonts_, 0, clock_ };
}

float Dashboard::caps(DrawList & list, const std::string & text, float x, float baseline, float size, Color color,
                      Align align) const {
    return hui::ui::text(list, fonts_.semibold, hui::ui::upper(text), x, baseline, size, color, align, 3.0f);
}

void Dashboard::draw_panel(DrawList & list, const Rect & r, float radius, Color accent, float lit) const {
    if (r.w <= 0 || r.h <= 0) return;
    list.shadow({ r.x, r.y + 10, r.w, r.h }, radius, 32, with_a(kBlack, 0.55f));
    if (lit > 0.01f) {
        list.glow(r, radius, 28, with_a(accent, 0.22f * lit));
    }
    list.gradient_rect(r, radius, hui::gfx::mix(kPanelTop, accent, 0.06f * (0.3f + lit)).with_alpha(0.92f),
                       hui::gfx::mix(kPanelBot, accent, 0.03f * (0.3f + lit)).with_alpha(0.95f));
    list.bordered_rect(r, radius, kClear, 1.5f, hui::gfx::mix(with_a(kInk, 0.12f), with_a(accent, 0.65f), lit));
}

void Dashboard::compose(hui::gfx::Renderer & renderer, const char * version, const char * build) {
    scene_.clear();
    if (loaded_) {
        draw(scene_, version, build);
    }
    backdrop_.time = clock_;
    renderer.begin();
    renderer.backdrop(backdrop_);
    renderer.draw(scene_);
}

void Dashboard::draw(DrawList & list, const char * version, const char * build) const {
    if (screensaver_active_ && !pictures_.empty() && pictures_[0].large.id != 0) {
        const Texture & t = pictures_[0].large;
        const Rect full{ 0, 0, 1920, 1080 };
        list.image(t.id, contain(full, t.width, t.height), hui::gfx::kFullUv, Color{ 1, 1, 1, 1 }, 0);

        const Rect caption{ 120, 930, 1680, 96 };
        list.rounded_rect(caption, 24, Color::rgb(0x0e1424, 0.88f));
        list.bordered_rect(caption, 24, kClear, 1.5f, with_a(kAmber, 0.5f));
        caps(list, "MACHETE AMBIENT GALLERY", caption.x + 32, caption.y + 36, 15, kAmber);
        const std::string pr = pictures_[0].prompt.empty() ? "Machete Studio Creation" : pictures_[0].prompt;
        hui::ui::text(list, fonts_.regular, fonts_.regular.font->fit(pr, 20, 1200), caption.x + 32, caption.y + 68, 20, with_a(kInk, 0.9f));
        draw_qr(list, caption.x + caption.w - 16, caption.cy());
        return;
    }

    // The parts arrive one after the other, each rising a little.
    int         index = 0;
    const auto  begin = [&] {
        const float in = hui::tween::stagger(age_, index++, 0.07f, 0.5f);
        list.push_opacity(in);
        list.push_transform(1.0f, 0.0f, 0.0f, 0.0f, 24.0f * (1.0f - in));
    };
    const auto end = [&] {
        list.pop_transform();
        list.pop_opacity();
    };
    begin();
    draw_header(list, version, build);
    end();
    begin();
    draw_server(list, kServer);
    end();
    begin();
    draw_activity(list, kActivity);
    end();
    begin();
    draw_memory(list, kMemory);
    end();
    begin();
    draw_image(list, kImage);
    end();
    begin();
    draw_recent(list, kRecent);
    end();
}

void Dashboard::draw_header(DrawList & list, const char * version, const char * build) const {
    const float w = hui::ui::text(list, fonts_.display, "MACHETE AI STUDIO PRO", 96, 112, 54, kInk);
    char sub[256];
    if (build != nullptr && build[0] != '\0') {
        std::snprintf(sub, sizeof(sub), "v%s (%s)  %s  GPU DIRECT ACCELERATION  %s  NEXT-GEN LOCAL INFERENCE",
                      version, build, kDot, kDot);
    } else {
        std::snprintf(sub, sizeof(sub), "v%s  %s  GPU DIRECT ACCELERATION  %s  NEXT-GEN LOCAL INFERENCE",
                      version, kDot, kDot);
    }
    hui::ui::text(list, fonts_.semibold, sub, 96 + w + 28, 112, 20, with_a(kCyan, 0.75f));
    hui::ui::Canvas c = canvas(list);
    state_chip_.draw(c);
}

void Dashboard::draw_server(DrawList & list, const Rect & r) const {
    draw_panel(list, r, kRadius, kCyan, 0.0f);
    caps(list, "Studio Wireless Connect", r.x + kPad, r.y + 36, 15, kCyan);

    // The QR code on the right: a phone's camera opens the web UI from it.
    const float qr   = draw_qr(list, r.x + r.w - 18, r.y + r.h * 0.5f);
    const float room = r.w - 2 * kPad - (qr > 0 ? qr + 24 : 0);

    // http://<address>:<port>
    float x = r.x + kPad;
    x += hui::ui::text(list, fonts_.semibold, "http://", x, r.y + 92, 36, with_a(kInk, 0.5f));
    if (snap_.address.empty()) {
        x += hui::ui::text(list, fonts_.semibold, "console-ip", x, r.y + 92, 36, with_a(kInk, 0.8f));
    } else {
        x += hui::ui::text(list, fonts_.semibold, snap_.address, x, r.y + 92, 36, kInk);
    }
    x += hui::ui::text(list, fonts_.semibold, ":", x, r.y + 92, 36, with_a(kInk, 0.5f));
    hui::ui::text(list, fonts_.semibold, std::to_string(snap_.port), x, r.y + 92, 36, kCyan);

    if (!snap_.api_key.empty()) {
        const float w    = caps(list, "API key", r.x + kPad, r.y + 138, 14, with_a(kInk, 0.5f));
        const float full = fonts_.mono.font->measure(snap_.api_key, 20);
        const float size = full > room - w - 16 ? std::max(15.0f, 20 * (room - w - 16) / full) : 20.0f;
        hui::ui::text(list, fonts_.mono, fonts_.mono.font->fit(snap_.api_key, size, room - w - 16),
                      r.x + kPad + w + 16, r.y + 138, size, kCyan);
    } else {
        const std::string hint = qr > 0 ? "Scan with your phone to open Machete Studio."
                                        : "Open in your phone or PC browser.";
        hui::ui::text(list, fonts_.regular, fonts_.regular.font->fit(hint, 20, room), r.x + kPad, r.y + 138, 20,
                      with_a(kInk, kMuted));
    }
    caps(list, "LOCAL WI-FI / ETHERNET", r.x + kPad, r.y + 184, 12, with_a(kCyan, 0.6f));
}

// The web UI's address, with the API key when the TV shows it anyway (the
// page signs in with a "#key=" in its address). After Machete PS5SD's QR code
// (github.com/VenezuelanBiggie24/Machete-PS5SD-IMG-and-VID).
void Dashboard::sync_qr() {
    std::string text;
    if (!snap_.address.empty()) {
        text = "http://" + snap_.address + ":" + std::to_string(snap_.port) + "/";
        std::string key;
        for (const char c : snap_.api_key) {
            if (c != ' ') {
                key += c;  // the TV shows it in groups of four
            }
        }
        if (!key.empty()) {
            text += "#key=" + key;
        }
    }
    if (text == qr_text_) {
        return;
    }
    qr_text_ = text;
    qr_size_ = 0;
    qr_.clear();
    if (text.empty()) {
        return;
    }
    try {
        const qrcodegen::QrCode code = qrcodegen::QrCode::encodeText(text.c_str(), qrcodegen::QrCode::Ecc::LOW);
        qr_size_ = code.getSize();
        qr_.resize((std::size_t) qr_size_ * (std::size_t) qr_size_);
        for (int y = 0; y < qr_size_; y++) {
            for (int x = 0; x < qr_size_; x++) {
                qr_[(std::size_t) y * (std::size_t) qr_size_ + (std::size_t) x] = code.getModule(x, y) ? 1 : 0;
            }
        }
    } catch (const std::exception &) {
        qr_size_ = 0;
        qr_.clear();
    }
}

float Dashboard::draw_qr(DrawList & list, float right, float cy) const {
    if (qr_size_ <= 0) {
        return 0;
    }
    // Whole pixels a module, with a quiet zone of two modules on white.
    const int   quiet = 2;
    const int   cells = qr_size_ + 2 * quiet;
    const float cell  = std::max(2.0f, std::floor(132.0f / (float) cells));
    const float side  = cell * (float) cells;
    const float x0    = std::floor(right - side);
    const float y0    = std::floor(cy - side * 0.5f);
    list.rounded_rect({ x0, y0, side, side }, 8, Color::rgb(0xffffff));
    const Color dark = Color::rgb(0x0a0f1d);
    for (int y = 0; y < qr_size_; y++) {
        int run = -1;  // runs of dark modules along a row, one rectangle each
        for (int x = 0; x <= qr_size_; x++) {
            const bool on = x < qr_size_ && qr_[(std::size_t) y * (std::size_t) qr_size_ + (std::size_t) x] != 0;
            if (on && run < 0) {
                run = x;
            } else if (!on && run >= 0) {
                list.rounded_rect({ x0 + (float) (quiet + run) * cell, y0 + (float) (quiet + y) * cell,
                                    (float) (x - run) * cell, cell },
                                  0, dark);
                run = -1;
            }
        }
    }
    return side;
}

void Dashboard::draw_stat(DrawList & list, const char * name, const std::string & value, float x,
                          float baseline) const {
    const float w = caps(list, name, x, baseline, 15, with_a(kInk, 0.5f));
    hui::ui::text(list, fonts_.mono, value, x + w + 12, baseline, 24, kInk);
}

void Dashboard::draw_activity(DrawList & list, const Rect & r) const {
    const AppStatus::Snapshot & s = snap_;
    const Color accent            = accent_.value();
    const bool  generating        = s.state == State::generating;
    const bool  busy              = generating || s.state == State::loading || s.state == State::starting;
    draw_panel(list, r, kRadius, accent, generating ? 1.0f : (s.state == State::error ? 0.6f : 0.25f));
    caps(list, "Studio Engine & Job", r.x + kPad, r.y + 36, 15, accent);

    // Active Model Name
    const std::string model = s.model.empty() ? "No Model Loaded" : s.model;
    hui::ui::text(list, fonts_.semibold, fonts_.semibold.font->fit(model, 26, r.w - 2 * kPad), r.x + kPad, r.y + 76, 26,
                  s.model.empty() ? with_a(kInk, kFaint) : kInk);

    // The mark / spinner, then state in words
    const Rect mark{ r.x + kPad, r.y + 98, 38, 38 };
    if (busy) {
        hui::ui::Spinner spinner = spinner_;
        spinner.style.color      = with_a(accent, 1.0f);
        spinner.set_bounds(mark);
        hui::ui::Canvas c = canvas(list);
        spinner.draw(c);
    } else {
        list.glow({ mark.cx() - 10, mark.cy() - 10, 20, 20 }, 10, 12, with_a(accent, 0.40f));
        list.circle(mark.cx(), mark.cy(), 10, accent);
    }
    const char * title = "Starting up";
    switch (s.state) {
        case State::loading:    title = "Loading model tensors"; break;
        case State::ready:      title = "Ready for prompts"; break;
        case State::generating: title = "Generating artwork..."; break;
        case State::error:      title = s.phase.empty() ? "Engine error" : s.phase.c_str(); break;
        case State::stopped:    title = "Server stopped"; break;
        default:                break;
    }
    const float tx = r.x + kPad + 52;
    hui::ui::text(list, fonts_.semibold, fonts_.semibold.font->fit(title, 28, r.w - 2 * kPad - 52), tx, r.y + 128, 28, kInk);

    // Prompt or detail line
    std::string line;
    if (generating && !s.prompt.empty()) {
        line = s.prompt;
    } else if (s.state == State::ready) {
        line = s.images > 0 ? "Last output: " + AppStatus::display_name(s.result) + ".png"
                            : "Waiting for prompt from phone or web UI.";
    } else if (s.state == State::loading || s.state == State::starting) {
        line = s.detail.empty() ? "Preparing model weights on unified APU VRAM..." : s.detail;
    } else if (s.state == State::error && !s.detail.empty()) {
        line = s.detail;
    }
    if (!line.empty()) {
        hui::ui::text(list, fonts_.regular, fonts_.regular.font->fit(line, 20, r.w - 2 * kPad), r.x + kPad, r.y + 168, 20,
                      with_a(kInk, kMuted));
    }

    if (progress_shown_) {
        hui::ui::Canvas c = canvas(list);
        progress_.draw(c);
    }

    // Figures along the bottom
    const float base = r.y + r.h - 26;
    const float x    = r.x + kPad;
    if (generating && s.steps > 0 && s.step > 0) {
        draw_stat(list, "Speed", [&] { char t[32]; std::snprintf(t, sizeof(t), "%.2f s/it", s.step_s); return std::string(t); }(), x, base);
        draw_stat(list, "Elapsed", seconds_text(s.job_s), x + 210, base);
        const double left = s.step_s > 0 ? (double) (s.steps - s.step) * s.step_s : -1;
        draw_stat(list, "Left", left >= 0 ? "~" + seconds_text(left) : "-", x + 410, base);
    } else if (generating) {
        draw_stat(list, "Elapsed", seconds_text(s.job_s), x, base);
    } else if (s.state == State::ready || s.state == State::stopped) {
        float at = x;
        if (s.last_job_s >= 0) {
            draw_stat(list, "Last render", seconds_text(s.last_job_s), at, base);
            at += 320;
        }
        draw_stat(list, "Creations", std::to_string(s.images), at, base);
    }
}

void Dashboard::draw_image(DrawList & list, const Rect & r) const {
    const AppStatus::Snapshot & s = snap_;
    const bool  generating        = s.state == State::generating;
    const bool  live              = generating && preview_.id != 0;
    const Picture * newest        = !pictures_.empty() && pictures_[0].large.id != 0 ? &pictures_[0] : nullptr;
    draw_panel(list, r, kRadius, kViolet, live ? 0.7f : 0.15f);

    std::string name = "Cinema Studio Canvas";
    if (live) {
        name = "Live Generation Preview";
    } else if (newest != nullptr && !generating) {
        name = "Latest Masterpiece";
    }
    caps(list, name, r.x + 28, r.y + 36, 16, kViolet);
    const Texture * tex = live ? &preview_ : (newest != nullptr ? &newest->large : nullptr);
    if (tex != nullptr) {
        char size[48];
        std::snprintf(size, sizeof(size), "%d x %d  %s  Native HDR", tex->width, tex->height, kDot);
        hui::ui::text(list, fonts_.mono, size, r.x + r.w - 28, r.y + 36, 18, with_a(kInk, kMuted), Align::right);
    }

    const Rect area{ r.x + 20, r.y + 54, r.w - 40, r.h - 74 };
    list.rounded_rect(area, 18, with_a(kWell, 0.88f));
    if (tex != nullptr) {
        const float fade = live ? 1.0f : reveal_.value;
        const float dim = generating && !live ? 0.18f : 1.0f;
        list.image(tex->id, contain(area, tex->width, tex->height), hui::gfx::kFullUv,
                   Color{ 1, 1, 1, fade * dim }, 14);
    }
    list.bordered_rect(area, 18, kClear, 1.5f, with_a(kViolet, 0.25f));
    if (live) {
        // Step progress along the bottom of the picture
        if (s.steps > 0) {
            const float frac = std::clamp((float) s.step / (float) s.steps, 0.0f, 1.0f);
            const Rect  track{ area.x + 24, area.y + area.h - 22, area.w - 48, 8 };
            list.rounded_rect(track, 4, with_a(kBlack, 0.6f));
            list.rounded_rect({ track.x, track.y, std::max(track.h, track.w * frac), track.h }, 4, kCyan);
        }
        // LIVE badge with animated breathing indicator
        const Rect pill{ area.x + 20, area.y + 20, 96, 36 };
        list.rounded_rect(pill, 18, with_a(kBlack, 0.65f));
        const float breath = hui::ui::breathe(clock_);
        list.circle(pill.x + 22, pill.cy(), 6 + 1.5f * breath, kCyan);
        hui::ui::text(list, fonts_.semibold, "LIVE", pill.x + 40, pill.cy() + 6, 18, kInk, Align::left, 2.0f);
    }
    if (tex == nullptr || (generating && !live)) {
        hui::ui::Canvas c = canvas(list);
        empty_.draw(c);
    }
}

void Dashboard::draw_recent(DrawList & list, const Rect & r) const {
    const AppStatus::Snapshot & s = snap_;
    draw_panel(list, r, kRadius, kLime, 0.0f);
    caps(list, log_open_ ? "Console Engine Log" : "Recent Creations Filmstrip", r.x + kPad, r.y + 32, 15, log_open_ ? kSteel : kLime);

    if (log_open_) {
        const std::size_t shown = std::min<std::size_t>(s.lines.size(), 6);
        float baseline          = r.y + 68;
        for (std::size_t i = s.lines.size() - shown; i < s.lines.size(); i++) {
            const std::string text = log_text(s.lines[i]);
            hui::ui::text(list, fonts_.mono, fonts_.mono.font->fit(text, 18, r.w - 2 * kPad), r.x + kPad, baseline, 18,
                          with_a(kInk, 0.82f));
            baseline += 24;
        }
        return;
    }

    char count[48];
    if (s.images > 0) {
        std::snprintf(count, sizeof(count), "%d creations this session", s.images);
    } else {
        std::snprintf(count, sizeof(count), "none yet");
    }
    hui::ui::text(list, fonts_.mono, count, r.x + r.w - kPad, r.y + 32, 18, with_a(kInk, kMuted), Align::right);

    const float size = 140.0f;
    const float gap  = (r.w - 2.0f * kPad - (float) kThumbs * size) / (float) (kThumbs - 1);
    if (pictures_.empty()) {
        const Rect well{ r.x + kPad, r.y + 50, r.w - 2 * kPad, size };
        list.rounded_rect(well, 16, with_a(kWell, 0.50f));
        list.bordered_rect(well, 16, kClear, 1.5f, with_a(kInk, 0.08f));
        hui::ui::text(list, fonts_.regular, s.tv_images ? "Finished creations and videos collect here" : "Hidden on this screen",
                      well.cx(), well.cy() + 8, 22,
                      with_a(kInk, kFaint), Align::center);
        return;
    }
    for (int i = 0; i < kThumbs; i++) {
        const Rect slot{ r.x + kPad + (size + gap) * (float) i, r.y + 50, size, size };
        const bool have = (std::size_t) i < pictures_.size();
        list.rounded_rect(slot, 16, with_a(kWell, have ? 0.85f : 0.45f));
        if (have && pictures_[(std::size_t) i].thumb.id != 0) {
            const Texture & t = pictures_[(std::size_t) i].thumb;
            list.image(t.id, contain(slot, t.width, t.height), hui::gfx::kFullUv, Color{ 1, 1, 1, 1 }, 14);
        } else if (have && pictures_[(std::size_t) i].failed) {
            hui::ui::text(list, fonts_.semibold, "?", slot.cx(), slot.cy() + 12, 36, with_a(kInk, kFaint), Align::center);
        }
        list.bordered_rect(slot, 16, kClear, i == 0 && have ? 2.5f : 1.5f,
                           i == 0 && have ? with_a(kLime, 0.85f) : with_a(kInk, have ? 0.15f : 0.06f));
    }
}

void Dashboard::draw_memory(DrawList & list, const Rect & r) const {
    const AppStatus::Snapshot & s = snap_;
    draw_panel(list, r, kRadius, kRose, 0.0f);
    caps(list, "Console Hardware Telemetry", r.x + 24, r.y + 32, 15, kRose);

    const bool hot = !std::isnan(s.console.soc_c) && s.console.soc_c >= 80.0;
    if (hot) {
        caps(list, "ALERT: SoC >= 80C", r.x + r.w - 24, r.y + 32, 14, kRose, Align::right);
    } else {
        caps(list, "THERMAL GUARDIAN: ACTIVE", r.x + r.w - 24, r.y + 32, 13, kLime, Align::right);
    }

    // VRAM readout & bar
    char text[64];
    if (s.free_gib >= 0 && s.pool_gib > 0) {
        std::snprintf(text, sizeof(text), "%.1f / %.1f GiB Free (APU Unified)", s.free_gib, s.pool_gib);
    } else {
        std::snprintf(text, sizeof(text), "Unified Memory Initializing");
    }
    hui::ui::text(list, fonts_.mono, text, r.x + 24, r.y + 60, 16, with_a(kInk, 0.85f));

    hui::ui::Canvas c = canvas(list);
    memory_.draw(c);

    // 4 Digital HUD Telemetry Cards below the bar
    const AppStatus::Console & k = s.console;
    const float card_y = r.y + 98;
    const float card_h = 112;
    const float gap    = 10.0f;
    const float card_w = (r.w - 48 - 3 * gap) / 4.0f;

    struct Gauge {
        const char * label;
        std::string  val;
        Color        accent;
        const char * sub;
    };

    Color soc_color = kLime;
    const char * soc_sub = "NOMINAL";
    if (!std::isnan(k.soc_c)) {
        if (k.soc_c >= 80.0) { soc_color = kRose; soc_sub = "CRITICAL"; }
        else if (k.soc_c >= 70.0) { soc_color = kAmber; soc_sub = "ELEVATED"; }
    }

    const Gauge gauges[4] = {
        { "APU TEMP", reading(k.soc_c, "%.0f\xC2\xB0" "C"), soc_color, soc_sub },
        { "FAN SPEED", reading(k.fan, "%.0f%%", 100), kCyan, "ACTIVE" },
        { "POWER", reading(k.watts, "%.0f W"), kAmber, "DRAW" },
        { "GPU LOAD", reading(k.job, "%.0f%%", 100), kViolet, "RENDER" },
    };

    for (int i = 0; i < 4; i++) {
        const Rect box{ r.x + 24 + i * (card_w + gap), card_y, card_w, card_h };
        list.rounded_rect(box, 12, with_a(kWell, 0.70f));
        list.bordered_rect(box, 12, kClear, 1.0f, with_a(gauges[i].accent, 0.35f));
        caps(list, gauges[i].label, box.cx(), box.y + 24, 12, with_a(kInk, 0.6f), Align::center);
        hui::ui::text(list, fonts_.mono, gauges[i].val, box.cx(), box.y + 64, 26, gauges[i].accent, Align::center);
        caps(list, gauges[i].sub, box.cx(), box.y + 92, 10, with_a(gauges[i].accent, 0.8f), Align::center);
    }
}

void Dashboard::draw_log_tile(DrawList & list, const Rect & r) const {
    if (r.w <= 0 || r.h <= 0) return;
    const AppStatus::Snapshot & s = snap_;
    draw_panel(list, r, 24, kSteel, 0.0f);
    caps(list, "Log", r.x + 24, r.y + 36, 17, kSteel);
    const std::size_t shown = std::min<std::size_t>(s.lines.size(), 2);
    float             y     = r.y + 62;
    for (std::size_t i = s.lines.size() - shown; i < s.lines.size(); i++) {
        const std::string text = log_text(s.lines[i]);
        hui::ui::text(list, fonts_.mono, fonts_.mono.font->fit(text, 17, r.w - 48), r.x + 24, y, 17,
                      with_a(kInk, 0.72f));
        y += 24;
    }
    if (s.lines.empty()) {
        hui::ui::text(list, fonts_.regular, "nothing logged yet", r.x + 24, y, 18, with_a(kInk, kFaint));
    }
}

}  // namespace ps5sd
