#!/usr/bin/env python3
"""Dev helper: serve the web UI at / and forward every other path to a PS5SD server.

    python scripts/webui-dev.py [--port 8787] [--target http://10.0.0.101:7860] [--mock-downloads] [--mock-models]

The page is put together from ps5/webui/src on every request (as scripts/bundle-webui.sh does
for the build), so an edit shows after a reload.

--mock-downloads answers the download routes of the app (catalog, downloads, Hugging Face
search) in this process, with simulated progress, so the Models tab can be tried before the
console has them. It also turns on --mock-models: the model library (list, switch, delete)
is then an in-memory fake too and the console is not asked about models at all, so a delete or
a switch cannot touch it. --mock-models alone fakes only the library. Everything else is still forwarded. Without the flag the real console answers
(an old app answers 404 and the page hides the download parts). Words in a URL steer the mock:
"fail" makes the download fail at 30 percent, "big" makes it 8 GB, "slow" makes it slow.
In a search, "err" gives an error and "none" gives no results.

--mock-standby starts like a fresh install: no model is loaded, the library answers
"standby": true, and every route outside /ps5sd/ answers 503 as sd-server --standby does,
until a switch (Use) has loaded a model. It implies --mock-downloads.
"""
import argparse
import base64
import http.client
import json
import os
import re
import struct
import threading
import time
import urllib.parse
import zlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "ps5", "webui", "src")
VERSION_HPP = os.path.join(HERE, "..", "ps5", "app", "version.hpp")


def bundle():
    """page.html with each "@@include <file>" line replaced by that file, as bytes."""
    out = []
    with open(os.path.join(SRC, "page.html"), "rb") as f:
        for line in f.read().split(b"\n"):
            if line.startswith(b"@@include "):
                with open(os.path.join(SRC, line[10:].decode()), "rb") as part:
                    text = part.read()
                    out.append(text[:-1] if text.endswith(b"\n") else text)
            else:
                out.append(line)
    return b"\n".join(out)


APP = "/ps5sd/v1/app/"
MB = 1000 * 1000
GB = 1000 * MB


class MockDownloads:
    """In-memory stand-in for the app's download routes. One download runs at a time."""

    CATALOG = [
        {"id": "sdxl-base", "name": "SDXL Base 1.0", "preset": "sdxl-base", "featured": True,
         "description": "The classic, works with LoRAs and finetunes.",
         "bytes": 6938 * MB + 335 * MB, "installed": False,
         "files": [{"url": "https://huggingface.co/stabilityai/stable-diffusion-xl-base-1.0/resolve/main/sd_xl_base_1.0.safetensors",
                    "dest": "sd_xl_base_1.0.safetensors", "bytes": 6938 * MB},
                   {"url": "https://huggingface.co/madebyollin/sdxl-vae-fp16-fix/resolve/main/sdxl_vae.safetensors",
                    "dest": "vae/sdxl_vae_fp16_fix.safetensors", "bytes": 335 * MB}]},
        {"id": "flux-klein", "name": "FLUX.3 Image Klein", "preset": "flux-klein", "featured": True,
         "description": "Best quality, writes text, slower.",
         "bytes": 3510 * MB + 2500 * MB + 340 * MB, "installed": True,
         "files": [{"url": "https://huggingface.co/example/flux3-klein/resolve/main/flux3-klein-int8.safetensors",
                    "dest": "flux3-klein-int8.safetensors", "bytes": 3510 * MB},
                   {"url": "https://huggingface.co/example/flux3-klein/resolve/main/text_encoder.safetensors",
                    "dest": "flux3/text_encoder.safetensors", "bytes": 2500 * MB},
                   {"url": "https://huggingface.co/example/flux3-klein/resolve/main/ae.safetensors",
                    "dest": "flux3/ae.safetensors", "bytes": 340 * MB}]},
        {"id": "z-image-turbo", "name": "Z-Image Turbo", "preset": "z-image-turbo", "featured": True,
         "description": "Fast photoreal, 8 steps.",
         "bytes": 3860 * MB + 2500 * MB + 330 * MB, "installed": False,
         "files": [{"url": "https://huggingface.co/example/z-image-turbo/resolve/main/z_image_turbo.safetensors",
                    "dest": "z_image_turbo.safetensors", "bytes": 3860 * MB},
                   {"url": "https://huggingface.co/example/z-image-turbo/resolve/main/text_encoder.safetensors",
                    "dest": "z-image/text_encoder.safetensors", "bytes": 2500 * MB},
                   {"url": "https://huggingface.co/example/z-image-turbo/resolve/main/ae.safetensors",
                    "dest": "z-image/ae.safetensors", "bytes": 330 * MB}]},
        {"id": "sd-turbo", "name": "SD Turbo (Q8_0)", "preset": "sd-turbo",
         "description": "One step, 512x512. Fast, good for trying prompts.", "bytes": 2023745376, "installed": False,
         "files": [{"url": "https://huggingface.co/example/sd-turbo-gguf/resolve/main/sd_turbo-f16-q8_0.gguf",
                    "dest": "sd_turbo-f16-q8_0.gguf", "bytes": 2023745376}]},
        {"id": "sdxl-lightning", "name": "SDXL Lightning 4-step (Q8_0)", "preset": "sdxl-lightning",
         "description": "SDXL quality in four steps, with the fp16 VAE.", "bytes": 5435 * MB, "installed": False,
         "files": [{"url": "https://huggingface.co/example/sdxl-lightning-gguf/resolve/main/sdxl_lightning_4step-Q8_0.gguf",
                    "dest": "sdxl_lightning_4step-Q8_0.gguf", "bytes": 5100 * MB},
                   {"url": "https://huggingface.co/madebyollin/sdxl-vae-fp16-fix/resolve/main/sdxl_vae.safetensors",
                    "dest": "vae/sdxl_vae_fp16_fix.safetensors", "bytes": 335 * MB}]},
        {"id": "sdxs", "name": "SDXS 512 (Q8_0)", "preset": "sdxs",
         "description": "Tiny and very fast, for quick drafts.", "bytes": 683 * MB, "installed": True,
         "files": [{"url": "https://huggingface.co/example/sdxs-gguf/resolve/main/sdxs-512-tinySDdistilled_Q8_0.gguf",
                    "dest": "sdxs-512-tinySDdistilled_Q8_0.gguf", "bytes": 683 * MB}]},
        {"id": "sana-1.6b", "name": "Sana 1.6B (1024px)", "preset": "sana-1.6b", "featured": True,
         "description": "Linear attention diffusion model, fast 1024x1024 generation in 14-20 steps.",
         "bytes": 1720 * MB + 1600 * MB + 100 * MB, "installed": False,
         "files": [{"url": "https://huggingface.co/Efficient-Large-Model/Sana_1600M_1024px/resolve/main/checkpoints/Sana_1600M_1024px.safetensors",
                    "dest": "diffusion_models/sana_1600M_1024px.safetensors", "bytes": 1720 * MB},
                   {"url": "https://huggingface.co/google/gemma-2-2b-it-GGUF/resolve/main/gemma-2-2b-it-Q4_K_M.gguf",
                    "dest": "text_encoders/gemma-2-2b-it-Q4_K_M.gguf", "bytes": 1600 * MB},
                   {"url": "https://huggingface.co/mit-han-lab/dc-ae-f32c32-sana-1.0/resolve/main/model.safetensors",
                    "dest": "vae/dc-ae-f32c32-sana-1.0.safetensors", "bytes": 100 * MB}]},
        {"id": "sana-0.6b", "name": "Sana 0.6B (512px)", "preset": "sana-0.6b", "featured": False,
         "description": "Ultra-lightweight linear attention model for fast 512x512 drafts.",
         "bytes": 650 * MB + 1600 * MB + 100 * MB, "installed": False,
         "files": [{"url": "https://huggingface.co/Efficient-Large-Model/Sana_600M_512px/resolve/main/checkpoints/Sana_600M_512px.safetensors",
                    "dest": "diffusion_models/sana_600M_512px.safetensors", "bytes": 650 * MB},
                   {"url": "https://huggingface.co/google/gemma-2-2b-it-GGUF/resolve/main/gemma-2-2b-it-Q4_K_M.gguf",
                    "dest": "text_encoders/gemma-2-2b-it-Q4_K_M.gguf", "bytes": 1600 * MB},
                   {"url": "https://huggingface.co/mit-han-lab/dc-ae-f32c32-sana-1.0/resolve/main/model.safetensors",
                    "dest": "vae/dc-ae-f32c32-sana-1.0.safetensors", "bytes": 100 * MB}]},
        {"id": "sd35-medium", "name": "SD 3.5 Medium (Q8_0)", "preset": "sd35-medium", "featured": True,
         "description": "Stable Diffusion 3.5 Medium with MMDiT architecture and improved prompt following.",
         "bytes": 2600 * MB + 3655 * MB + 160 * MB, "installed": False,
         "files": [{"url": "https://huggingface.co/city96/StableDiffusion3.5-Medium-gguf/resolve/main/sd3.5_medium-Q8_0.gguf",
                    "dest": "diffusion_models/sd3.5_medium-Q8_0.gguf", "bytes": 2600 * MB},
                   {"url": "https://huggingface.co/city96/t5-v1_1-xxl-encoder-gguf/resolve/main/t5-v1_1-xxl-encoder-Q4_K_M.gguf",
                    "dest": "text_encoders/t5-v1_1-xxl-encoder-Q4_K_M.gguf", "bytes": 3655 * MB},
                   {"url": "https://huggingface.co/stabilityai/stable-diffusion-3.5-medium/resolve/main/vae/diffusion_pytorch_model.safetensors",
                    "dest": "vae/sd3.5_vae.safetensors", "bytes": 160 * MB}]},
        {"id": "kokoro-82m", "name": "Kokoro 82M (Audio / TTS)", "preset": "kokoro-82m", "featured": True,
         "description": "Compact high-quality speech and audio generation model (82M parameters).",
         "bytes": 340 * MB, "installed": False,
         "files": [{"url": "https://huggingface.co/hexgrad/Kokoro-82M/resolve/main/kokoro-v0_19.safetensors",
                    "dest": "audio_models/kokoro-v0_19.safetensors", "bytes": 340 * MB}]},
    ]

    def __init__(self):
        self.lock = threading.Lock()
        self.items = []
        self.next_id = 1
        self.last = time.time()

    @staticmethod
    def _speed(url):
        return 8 * MB if "slow" in url else 30 * MB

    def _next_queued(self):
        for d in self.items:
            if d["state"] in ("queued", "running"):
                return d
        return None

    def _advance(self):
        now = time.time()
        dt = now - self.last
        self.last = now
        run = self._next_queued()
        while run is not None and dt > 0:
            run["state"] = "running"
            speed = self._speed(run["url"])
            run["bytes_per_second"] = int(speed * (0.85 + 0.3 * ((now * 7) % 1)))
            goal = int(run["total"] * 0.3) if "fail" in run["url"] else run["total"]
            need = goal - run["bytes"]
            step = speed * dt
            if step < need:
                run["bytes"] += int(step)
                break
            run["bytes"] += int(need)
            dt -= need / float(speed)
            run["bytes_per_second"] = 0
            if "fail" in run["url"]:
                run["state"] = "failed"
                run["error"] = "HTTP 403 from the server (login required)"
            else:
                run["state"] = "done"
            run = self._next_queued()

    def _installed(self, entry):
        if entry["installed"]:
            return True
        for f in entry["files"]:
            if not any(d["dest"] == f["dest"] and d["state"] == "done" for d in self.items):
                return False
        return True

    def handle(self, method, path, body):
        """Returns (status, object) or None when the path is not a download route."""
        route = path.split("?")[0]
        if not route.startswith(APP):
            return None
        route = route[len(APP):]
        with self.lock:
            self._advance()
            if method == "GET" and route == "catalog":
                out = []
                for e in self.CATALOG:
                    c = dict(e)
                    c["installed"] = self._installed(e)
                    out.append(c)
                return 200, {"entries": out}
            if method == "GET" and route == "downloads":
                return 200, {"downloads": [dict(d) for d in self.items]}
            if method == "POST" and route == "downloads":
                try:
                    req = json.loads(body or b"{}")
                except ValueError:
                    return 400, {"error": "body is not JSON"}
                url = str(req.get("url") or "")
                if not re.match(r"^https?://", url):
                    return 400, {"error": "url must start with http:// or https://"}
                for d in self.items:
                    if d["url"] == url and d["state"] in ("queued", "running"):
                        return 409, {"error": "already downloading"}
                dest = req.get("dest") or urllib.parse.unquote(url.split("?")[0].split("/")[-1]) or "download.bin"
                total = 8 * GB if "big" in url else 400 * MB
                for e in self.CATALOG:
                    for f in e["files"]:
                        if f["url"] == url:
                            total = f["bytes"]
                item = {"id": str(self.next_id), "url": url, "dest": dest, "bytes": 0, "total": total,
                        "state": "queued", "error": "", "bytes_per_second": 0, "catalog": req.get("catalog") or ""}
                self.next_id += 1
                self.items.append(item)
                self._advance()
                return 202, dict(item)
            if method == "DELETE" and route.startswith("downloads/"):
                did = route.split("/", 1)[1]
                for d in self.items:
                    if d["id"] == did:
                        if d["state"] in ("queued", "running"):
                            d["state"] = "cancelled"
                            d["bytes_per_second"] = 0
                        return 200, {"ok": True}
                return 404, {"error": "no such download"}
            if method == "POST" and route == "hf/search":
                try:
                    q = str(json.loads(body or b"{}").get("q") or "")
                except ValueError:
                    return 400, {"error": "body is not JSON"}
                if "err" in q:
                    return 502, {"error": "Hugging Face is not reachable from the console"}
                if "none" in q:
                    return 200, {"results": []}
                many = [{"path": "variant-%d/model-Q%d_0.gguf" % (i, i), "bytes": (1 + i) * 700 * MB} for i in range(9)]
                return 200, {"results": [
                    {"repo": "stabilityai/stable-diffusion-xl-base-1.0", "downloads": 1840211, "likes": 6500,
                     "files": [{"path": "README.md", "bytes": 9000},
                               {"path": "sd_xl_base_1.0.safetensors", "bytes": 6938078334},
                               {"path": "vae/diffusion_pytorch_model.safetensors", "bytes": 334643276}]},
                    {"repo": "someone/%s-gguf" % (re.sub(r"[^A-Za-z0-9_.-]+", "-", q)[:24] or "model"),
                     "downloads": 12400, "likes": 88,
                     "files": [{"path": "config.json", "bytes": 800}] + many},
                    {"repo": "nobody/tiny-docs", "downloads": 3, "likes": 0,
                     "files": [{"path": "README.md", "bytes": 100}]},
                ]}
        return 404, {"error": "not found"}


class MockModels:
    """In-memory stand-in for GET /models, POST /models/active and DELETE /models/<id>."""

    DIR = "/data/PS5SD/models/"
    EXT = (".gguf", ".safetensors", ".ckpt", ".sft")
    FILES = {
        "stable-diffusion-xl-base-1.0-Q8_0.gguf": 5040957760,
        "vae/sdxl_vae_fp16_fix.safetensors": 334641162,
        "raehoshiIllustXL_v110.safetensors": 6938063182,
        "flux3-klein-int8.safetensors": 3510 * MB,
        "flux3/text_encoder.safetensors": 2500 * MB,
        "sd_turbo-f16-q8_0.gguf": 2023745376,
        "sdxs-512-tinySDdistilled_Q8_0.gguf": 682847200,
        "sana_1600M_1024px.safetensors": 1720 * MB,
        "text_encoders/gemma-2-2b-it-Q4_K_M.gguf": 1600 * MB,
        "vae/dc-ae-f32c32-sana-1.0.safetensors": 100 * MB,
        "diffusion_models/sd3.5_medium-Q8_0.gguf": 2600 * MB,
        "text_encoders/t5-v1_1-xxl-encoder-Q4_K_M.gguf": 3655 * MB,
        "vae/sd3.5_vae.safetensors": 160 * MB,
        "audio_models/kokoro-v0_19.safetensors": 340 * MB,
    }
    SDXL_DEFAULTS = {"steps": "25", "cfg": "6", "width": "1024", "height": "1024", "sampler": "euler_a"}

    def __init__(self, downloads=None):
        self.lock = threading.Lock()
        self.downloads = downloads
        self.files = dict(self.FILES)
        self.presets = [
            {"id": "sdxl-base", "name": "SDXL Base 1.0 (Q8_0)", "defaults": self.SDXL_DEFAULTS,
             "files": ["stable-diffusion-xl-base-1.0-Q8_0.gguf", "vae/sdxl_vae_fp16_fix.safetensors"]},
            {"id": "sdxl-fast", "name": "SDXL Base 1.0 (Q8_0), fast", "defaults": dict(self.SDXL_DEFAULTS, steps="12"),
             "files": ["stable-diffusion-xl-base-1.0-Q8_0.gguf", "vae/sdxl_vae_fp16_fix.safetensors"]},
            {"id": "illustxl", "name": "Raehoshi IllustXL 1.1 (Q8_0)", "defaults": dict(self.SDXL_DEFAULTS, steps="28"),
             "files": ["raehoshiIllustXL_v110.safetensors", "vae/sdxl_vae_fp16_fix.safetensors"]},
            # one file is missing on purpose: a preset that is incomplete
            {"id": "flux-klein", "name": "FLUX.3 Image Klein (int8)",
             "defaults": {"steps": "16", "cfg": "1", "width": "1024", "height": "1024", "sampler": "euler"},
             "files": ["flux3-klein-int8.safetensors", "flux3/text_encoder.safetensors", "flux3/ae.safetensors"]},
            {"id": "sana-1.6b", "name": "Sana 1.6B (1024px)",
             "defaults": {"steps": "18", "cfg": "4.5", "width": "1024", "height": "1024", "sampler": "euler"},
             "files": ["sana_1600M_1024px.safetensors", "text_encoders/gemma-2-2b-it-Q4_K_M.gguf", "vae/dc-ae-f32c32-sana-1.0.safetensors"]},
            {"id": "sana-0.6b", "name": "Sana 0.6B (512px)",
             "defaults": {"steps": "14", "cfg": "4.5", "width": "512", "height": "512", "sampler": "euler"},
             "files": ["sana_1600M_1024px.safetensors", "text_encoders/gemma-2-2b-it-Q4_K_M.gguf", "vae/dc-ae-f32c32-sana-1.0.safetensors"]},
            {"id": "sd35-medium", "name": "SD 3.5 Medium (Q8_0)",
             "defaults": {"steps": "24", "cfg": "4.5", "width": "1024", "height": "1024", "sampler": "euler"},
             "files": ["diffusion_models/sd3.5_medium-Q8_0.gguf", "text_encoders/t5-v1_1-xxl-encoder-Q4_K_M.gguf", "vae/sd3.5_vae.safetensors"]},
            {"id": "kokoro-82m", "name": "Kokoro 82M (Audio)",
             "defaults": {"voice": "af_heart", "speed": "1.0"},
             "files": ["audio_models/kokoro-v0_19.safetensors"]},
        ]
        self.active = "sdxl-base"
        self.loaded = "sdxl-base"
        self.until = 0.0
        self.standby = False

    def _entries(self):
        out = []
        for p in self.presets:
            out.append(dict(p, kind="preset"))
        for path in sorted(self.files):
            if "/" not in path and path.lower().endswith(self.EXT):
                out.append({"id": path, "name": path.rsplit(".", 1)[0], "kind": "file", "files": [path], "defaults": {}})
        return out

    def _find(self, mid):
        for e in self._entries():
            if e["id"] == mid:
                return e
        return None

    def _json(self):
        if self.until and time.time() >= self.until:
            self.until = 0.0
            self.loaded = self.active
            self.standby = False
        models = []
        for e in self._entries():
            models.append({"id": e["id"], "name": e["name"], "kind": e["kind"], "defaults": e["defaults"],
                           "bytes": sum(self.files[f] for f in e["files"] if f in self.files),
                           "files": [self.DIR + f for f in e["files"]],
                           "missing": [self.DIR + f for f in e["files"] if f not in self.files]})
        return {"active": self.active, "loaded": "" if self.until else self.loaded,
                "switching": bool(self.until), "standby": self.standby and not self.until, "models": models}

    def _sync_catalog(self, mid, gone):
        if self.downloads is None:
            return
        with self.downloads.lock:
            for e in self.downloads.CATALOG:
                if e.get("preset") == mid:
                    e["installed"] = False
            self.downloads.items = [d for d in self.downloads.items
                                    if not (d["state"] == "done" and d["dest"] in gone)]

    def handle(self, method, path, body):
        route = path.split("?")[0]
        if not route.startswith(APP + "models"):
            return None
        rest = route[len(APP + "models"):]
        with self.lock:
            if method == "GET" and rest == "":
                return 200, self._json()
            if method == "POST" and rest == "/active":
                try:
                    mid = str(json.loads(body or b"{}").get("id") or "")
                except ValueError:
                    mid = ""
                if not mid:
                    return 400, {"error": "body needs an id"}
                e = self._find(mid)
                if e is None or any(f not in self.files for f in e["files"]):
                    return 404, {"error": "unknown model, or files missing: " + mid}
                cur = self._json()
                if cur["switching"]:
                    return 409, {"error": "a model switch is already running"}
                if cur["loaded"] == mid:
                    return 200, cur
                self.active = mid
                self.until = time.time() + 3
                return 202, self._json()
            if method == "DELETE" and rest.startswith("/"):
                mid = urllib.parse.unquote(rest[1:])
                e = self._find(mid)
                if e is None:
                    return 404, {"error": "unknown model: " + mid}
                cur = self._json()
                if cur["switching"]:
                    return 409, {"error": "a model switch is running"}
                if mid in (self.active, cur["loaded"]):
                    return 409, {"error": e["name"] + " is in use; switch to another model first"}
                if e["kind"] == "file":
                    for p in self.presets:
                        if e["files"][0] in p["files"]:
                            return 409, {"error": e["name"] + " is part of preset " + p["id"] + "; delete that preset instead"}
                busy = set(self._find(self.active)["files"]) if self._find(self.active) else set()
                others = set()
                for p in self.presets:
                    if p["id"] != mid:
                        others.update(p["files"])
                deleted, kept = [], []
                for f in e["files"]:
                    if f not in self.files:
                        continue
                    if f in busy or f in others:
                        kept.append(self.DIR + f)
                    else:
                        del self.files[f]
                        deleted.append(self.DIR + f)
                if e["kind"] == "preset":
                    self.presets = [p for p in self.presets if p["id"] != mid]
                self._sync_catalog(mid, [d[len(self.DIR):] for d in deleted])
                out = self._json()
                out["deleted"] = deleted
                out["kept"] = kept
                return 200, out
        return 404, {"error": "not found"}


def make_png(width=128, height=128, color=(45, 125, 230)):
    """Generate a clean valid PNG binary buffer without external dependencies."""
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for x in range(width):
            r = (color[0] + x) % 256
            g = (color[1] + y) % 256
            b = color[2]
            raw.extend((r, g, b))
    compressed = zlib.compress(bytes(raw))

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", compressed)
        + chunk(b"IEND", b"")
    )


class MockBackend:
    """Full offline backend answering samplers, schedulers, console stats, settings, jobs and gallery."""

    def __init__(self, models=None):
        self.lock = threading.Lock()
        self.models = models
        self.jobs = {}
        self.gallery = []
        self.labels = {}
        self.next_job_id = 1
        self.settings = {
            "tv_images": True,
            "tv_key": False,
            "fan": "balanced",
            "fan_target": 65,
            "telegram_enabled": False,
            "telegram_token": "",
            "telegram_admin_id": 0,
            "telegram_paired": False,
        }

    def handle(self, method, path, body):
        route = path.split("?")[0]
        with self.lock:
            # 1. Server info & capabilities
            if method == "GET" and route == "/sdapi/v1/samplers":
                return 200, [
                    {"name": "euler_a"},
                    {"name": "euler"},
                    {"name": "dpm++2m"},
                    {"name": "dpm++2s_a"},
                    {"name": "dpm2"},
                    {"name": "lcm"},
                    {"name": "ddim"},
                ]
            if method == "GET" and route == "/sdapi/v1/schedulers":
                return 200, [
                    {"name": "normal"},
                    {"name": "karras"},
                    {"name": "exponential"},
                    {"name": "ays"},
                    {"name": "sgm_uniform"},
                    {"name": "simple"},
                ]
            if method == "GET" and route == "/sdcpp/v1/capabilities":
                return 200, {
                    "supported_modes": ["img_gen", "vid_gen"],
                    "features": {"cancel_generating": True},
                    "upscalers": ["RealESRGAN_x4plus", "ESRGAN_4x"],
                    "limits": {"max_repeat": 10},
                }
            if method == "GET" and route == "/sdapi/v1/options":
                active = self.models.active if self.models else "sdxl-base"
                return 200, {"sd_model_checkpoint": active}
            if method == "GET" and route == "/sdapi/v1/sd-models":
                active = self.models.active if self.models else "sdxl-base"
                return 200, [{"title": active, "model_name": active}]
            if method == "GET" and route == "/v1/models":
                active = self.models.active if self.models else "sdxl-base"
                return 200, {"data": [{"id": active}]}
            if method == "GET" and route == "/sdapi/v1/progress":
                return 200, {
                    "progress": 0.0,
                    "eta_relative": 0.0,
                    "state": {"job_count": len(self.jobs), "sampling_steps": 0},
                }

            # 2. App console, settings, auth, storage, companion
            if method == "GET" and route == APP + "settings":
                return 200, dict(self.settings)
            if method == "POST" and route == APP + "settings":
                try:
                    patch = json.loads(body or b"{}")
                    self.settings.update(patch)
                except ValueError:
                    return 400, {"error": "invalid json"}
                return 200, dict(self.settings)
            if method == "GET" and route == APP + "console":
                return 200, {
                    "soc_temp_c": 56,
                    "fan_rpm": 1620,
                    "fan_level": 52,
                    "fan_mode": self.settings.get("fan", "balanced"),
                    "mem_used_bytes": 4400000000,
                    "mem_total_bytes": 16000000000,
                    "vram_used_bytes": 3400000000,
                    "vram_total_bytes": 12884901888,
                }
            if method == "GET" and route in (APP + "auth", "/ps5sd/v1/auth"):
                return 200, {"required": False, "authenticated": True, "enabled": False, "key": "", "active": False}
            if method == "POST" and route in (APP + "auth/login", "/ps5sd/v1/auth/login"):
                return 200, {"ok": True}
            if method == "POST" and route in (APP + "auth/logout", "/ps5sd/v1/auth/logout"):
                return 200, {"ok": True}
            if method == "GET" and route in (APP + "health", "/ps5sd/v1/health"):
                return 200, {"status": "ok", "app": "PS5SD"}
            if method == "GET" and route == APP + "loras":
                return 200, {"loras": []}
            if method == "GET" and route == APP + "upscalers":
                return 200, {"upscalers": ["RealESRGAN_x4plus", "ESRGAN_4x"]}
            if method == "GET" and route == APP + "storage/devices":
                return 200, [
                    {
                        "mount": "/data/PS5SD",
                        "label": "Internal NVMe",
                        "total_bytes": 1000000000000,
                        "free_bytes": 450000000000,
                        "is_active": True,
                        "is_usb": False,
                        "has_models": True,
                    }
                ]
            if method == "POST" and route == APP + "storage/select":
                return 200, {"ok": True, "base_dir": "/data/PS5SD"}
            if method == "GET" and route == APP + "face/status":
                return 200, {"has_face": False, "face_name": ""}
            if method == "GET" and route == APP + "companion/status":
                return 200, {
                    "enabled": self.settings.get("telegram_enabled", False),
                    "has_token": bool(self.settings.get("telegram_token")),
                    "running": False,
                }
            if method == "POST" and route == APP + "companion/send":
                return 200, {"ok": True, "message": "Dispatched to Telegram companion bot."}
            if method == "GET" and route == APP + "tags":
                return 200, {"count": 0, "tags": []}
            if method == "POST" and route == APP + "tags":
                return 200, {"tags": []}
            if method == "GET" and route == APP + "gallery/labels":
                return 200, dict(self.labels)
            if method == "POST" and route == APP + "gallery/labels":
                try:
                    j = json.loads(body or b"{}")
                    name = j.get("name")
                    if name:
                        self.labels[name] = {"title": j.get("title", ""), "tags": j.get("tags", [])}
                except ValueError:
                    return 400, {"error": "invalid json"}
                return 200, dict(self.labels)
            if method == "POST" and route == APP + "fit":
                return 200, {"fits": True, "headroom_bytes": 4000000000}

            # 3. Gallery & Image Generation
            if method == "GET" and (route == "/ps5sd/v1/gallery" or route == "/gallery"):
                return 200, list(self.gallery)
            if method == "POST" and (route == "/sdcpp/v1/img_gen" or route == "/sdcpp/v1/vid_gen"):
                jid = "job_%d" % self.next_job_id
                self.next_job_id += 1
                img_name = "output_%d.png" % int(time.time())
                self.jobs[jid] = {
                    "id": jid,
                    "created": time.time(),
                    "duration": 1.5,
                    "filename": img_name,
                    "cancelled": False,
                }
                return 200, {"job_id": jid, "status": "queued"}
            if method == "GET" and route.startswith("/sdcpp/v1/jobs/"):
                jid = route.split("/")[-1]
                job = self.jobs.get(jid)
                if not job:
                    return 404, {"error": "job not found"}
                elapsed = time.time() - job["created"]
                if elapsed < job["duration"] and not job["cancelled"]:
                    return 200, {"job_id": jid, "status": "running", "progress": min(0.95, elapsed / job["duration"])}
                if job["cancelled"]:
                    return 200, {"job_id": jid, "status": "cancelled", "progress": 0.0}
                if not any(g.get("name") == job["filename"] for g in self.gallery):
                    self.gallery.insert(0, {
                        "name": job["filename"],
                        "url": "/outputs/" + job["filename"],
                        "bytes": 8862,
                        "created": int(job["created"]),
                    })
                return 200, {"job_id": jid, "status": "completed", "progress": 1.0, "output": [job["filename"]]}
            if method == "POST" and route.endswith("/cancel") and "/sdcpp/v1/jobs/" in route:
                jid = route.split("/")[-2]
                if jid in self.jobs:
                    self.jobs[jid]["cancelled"] = True
                return 200, {"ok": True}
            if method == "POST" and route in ("/sdapi/v1/txt2img", "/sdapi/v1/img2img"):
                png = make_png()
                b64 = base64.b64encode(png).decode("ascii")
                img_name = "output_%d.png" % int(time.time())
                self.gallery.insert(0, {
                    "name": img_name,
                    "url": "/outputs/" + img_name,
                    "bytes": len(png),
                    "created": int(time.time()),
                })
                return 200, {"images": [b64]}
            if method == "POST" and route == "/sdapi/v1/interrupt":
                for j in self.jobs.values():
                    j["cancelled"] = True
                return 200, {"ok": True}

        return None


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    target = None
    mock = None
    models = None
    backend = None

    def log_message(self, fmt, *args):
        pass

    mock_all = False

    def do_HEAD(self):
        clean_path = self.path.split("?")[0]
        if clean_path in ("/", "/index.html"):
            body = bundle()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            return
        if clean_path.startswith("/outputs/") or clean_path.startswith("/data/PS5SD/outputs/"):
            img = make_png()
            self.send_response(200)
            self.send_header("Content-Type", "image/png")
            self.send_header("Content-Length", str(len(img)))
            self.end_headers()
            return
        self._dispatch()

    def _page(self):
        body = bundle()
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _proxy(self, body):
        t = urllib.parse.urlparse(self.target)
        try:
            conn = http.client.HTTPConnection(t.hostname, t.port or 80, timeout=2.5)
            headers = {k: v for k, v in self.headers.items()
                       if k.lower() not in ("host", "connection", "accept-encoding")}
            conn.request(self.command, self.path, body=body, headers=headers)
            r = conn.getresponse()
            data = r.read()
        except Exception as e:
            if self.backend is not None:
                res = self.backend.handle(self.command, self.path, body)
                if res is not None:
                    if self.command == "HEAD":
                        self.send_response(res[0])
                        self.send_header("Content-Type", "application/json")
                        self.send_header("Content-Length", "0")
                        self.end_headers()
                        return
                    self._json(res[0], res[1])
                    return
            msg = ("proxy error: %s" % e).encode()
            self.send_response(502)
            self.send_header("Content-Length", str(len(msg)))
            self.end_headers()
            self.wfile.write(msg)
            return
        self.send_response(r.status)
        for k, v in r.getheaders():
            if k.lower() in ("transfer-encoding", "connection", "content-length", "keep-alive"):
                continue
            self.send_header(k, v)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(data)

    def _json(self, code, obj):
        data = json.dumps(obj).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(data)

    def _dispatch(self):
        clean_path = self.path.split("?")[0]
        if self.command == "GET" and clean_path in ("/", "/index.html"):
            self._page()
            return
        if self.command in ("GET", "HEAD") and (clean_path.startswith("/outputs/") or clean_path.startswith("/data/PS5SD/outputs/")):
            img = make_png()
            self.send_response(200)
            self.send_header("Content-Type", "image/png")
            self.send_header("Content-Length", str(len(img)))
            self.end_headers()
            if self.command == "GET":
                self.wfile.write(img)
            return

        length = int(self.headers.get("Content-Length") or 0)
        body = self.rfile.read(length) if length else None
        res = None
        if self.models is not None and clean_path == APP + "info":
            src = open(VERSION_HPP, encoding="utf-8").read()
            version = re.search(r'PS5SD_VERSION "([^"]*)"', src).group(1)
            self._json(200, {"name": "PS5SD", "version": version, "sd_cpp": {"version": "mock", "commit": "mock"}})
            return
        if self.models is not None and self.models.standby and not self.path.startswith("/ps5sd/"):
            with self.models.lock:
                standby = self.models._json()["standby"]
            if standby:
                self._json(503, {"error": "no model is loaded; pick or download one in the web UI, tab Models",
                                 "standby": True})
                return
        if self.models is not None and re.match(r"^/ps5sd/v1/app/models(/|\?|$)", self.path):
            res = self.models.handle(self.command, self.path, body)
        elif self.mock is not None and re.match(r"^/ps5sd/v1/app/(catalog|downloads|hf/)", self.path):
            res = self.mock.handle(self.command, self.path, body)
        if res is not None:
            self._json(res[0], res[1])
            return

        if self.backend is not None and self.mock_all:
            res = self.backend.handle(self.command, self.path, body)
            if res is not None:
                self._json(res[0], res[1])
                return

        self._proxy(body)

    do_GET = do_POST = do_PUT = do_DELETE = do_OPTIONS = _dispatch


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8787)
    ap.add_argument("--target", default="http://10.0.0.101:7860")
    ap.add_argument("--mock-all", action="store_true",
                    help="run completely standalone with simulated models, downloads, generation and gallery")
    ap.add_argument("--mock-downloads", action="store_true",
                    help="answer catalog, downloads and Hugging Face search here (simulated)")
    ap.add_argument("--mock-models", action="store_true",
                    help="answer the model library (list, switch, delete) here; implied by --mock-downloads")
    ap.add_argument("--mock-standby", action="store_true",
                    help="start without a model, as sd-server --standby does; implies --mock-downloads")
    a = ap.parse_args()
    a.mock_all = a.mock_all or (a.target == "http://10.0.0.101:7860" and not (a.mock_downloads or a.mock_models))
    a.mock_downloads = a.mock_downloads or a.mock_standby or a.mock_all
    a.mock_models = a.mock_models or a.mock_all
    Handler.target = a.target
    Handler.mock_all = a.mock_all
    if a.mock_downloads:
        Handler.mock = MockDownloads()
    if a.mock_downloads or a.mock_models:
        Handler.models = MockModels(Handler.mock)
    Handler.backend = MockBackend(Handler.models)
    if a.mock_standby:
        Handler.models.standby = True
        Handler.models.active = Handler.models.loaded = ""
    print("serving %s on http://127.0.0.1:%d -> %s%s" % (SRC, a.port, a.target,
                                                          (" (mock all)" if a.mock_all else (" (mock downloads)" if a.mock_downloads else "") + (" (mock models)" if Handler.models else ""))))
    ThreadingHTTPServer(("127.0.0.1", a.port), Handler).serve_forever()
