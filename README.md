<div align="center">

<img src="assets/machete-icon.png" alt="Machete PS5SD Icon" width="160" height="160" style="border-radius: 28px; box-shadow: 0 8px 24px rgba(0,0,0,0.5);">

# Machete PS5SD: Image & Video Studio

**Local AI-Powered Image and Video Generation on the PlayStation 5 GPU**  
*Built on `stable-diffusion.cpp` and ggml-vulkan over RADV, featuring a responsive web UI and TV dashboard.*

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Platform: PS5](https://img.shields.io/badge/Platform-PS5%20(7.00--13.60)-purple.svg)](#requirements)
[![Video Generation](https://img.shields.io/badge/AI-Text--to--Video%20%7C%20Image--to--Video-00e5ff.svg)](#video-generation-t2v--i2v)
[![Upstream](https://img.shields.io/badge/Upstream-Darkatek7%2FPS5SD-brightgreen.svg)](https://github.com/Darkatek7/PS5SD)

</div>

---

## Table of Contents

- [Overview](#overview)
- [New Features in v0.5.0](#new-features-in-v050)
- [Video Generation Models](#video-generation-models)
- [Intelligent TV Dashboard and Instant QR Connection](#intelligent-tv-dashboard-and-instant-qr-connection)
- [Hardware Architecture and Memory Budget](#hardware-architecture-and-memory-budget)
- [Requirements](#requirements)
- [Installation Guide](#installation-guide)
- [Network Configuration and Environment Variables](#network-configuration-and-environment-variables)
- [HTTP REST API and Client Integration](#http-rest-api-and-client-integration)
- [Credits and Acknowledgments](#credits-and-acknowledgments)
- [License](#license)
- [Version en Espanol](#machete-ps5sd-estudio-de-imagen-y-video)

---

## Overview

Machete PS5SD is a complete, self-contained generative artificial intelligence suite designed specifically to leverage the 11.0 GiB of usable unified memory on jailbroken PlayStation 5 consoles.

Building on the foundation created by Darkatek7 / Heel Tobias in [PS5SD](https://github.com/Darkatek7/PS5SD), this Machete release expands the platform with full Video Generation capabilities (both Text-to-Video and Image-to-Video), real local console IP detection, an on-screen TV QR code for instant mobile device pairing, and comprehensive memory orchestration to prevent system out-of-memory exceptions.

---

## New Features in v0.5.0

- Full Text-to-Video (T2V) and Image-to-Video (I2V) generation pipelines in C/C++ with Web UI integration.
- Full Image-to-Image and Inpainting mask editor (paint, erase, invert, brush sizing, and DualSense cursor support) with seamless edge blending.
- Dedicated inpainting models added to the catalog: SDXL Inpainting 0.1 (Q8_0) and SD 1.5 Inpainting (Q8_0).
- Built-in curated video model catalog with 1-click downloads for Wan 2.2, Wan 2.1, and LTX-Video.
- Direct image animation button ("Animate this image") available in the gallery and lightbox views.
- Strict $4k+1$ frame selection standard (17, 33, 49, and 65 frames) for temporal stability.
- Automatic console LAN IPv4 detection through kernel routing table queries, replacing placeholder addresses.
- GPU-rendered high-contrast TV QR code for instant mobile browser pairing without manual IP entry.
- Automatic authentication pass-through via QR fragment URLs when API keys are enabled.
- Spatial-temporal VAE tiling, sequential text encoder unloading, and flash attention to stay within the 11.0 GiB VRAM limit.
- Native MP4 and WebM video packaging with in-browser playback.

---

## Video Generation Models

Machete PS5SD introduces native temporal diffusion models optimized for the PS5 unified APU architecture:

### 1. Curated Model Catalog

- **Wan 2.2 TI2V 5B (Q4_K_M):** Flagship hybrid model capable of generating both Text-to-Video and Image-to-Video. Optimized for 832x480 (landscape) and 480x832 (portrait). Shares the UMT5-XXL text encoder and Wan VAE.
- **Wan 2.1 T2V/I2V 1.3B (Q8_0):** High-speed, lightweight model ideal for rapid iteration. Reuses the same UMT5-XXL text encoder and VAE weights as Wan 2.2 without redundant storage.
- **LTX-Video 2B (Q8_0):** Ultra-fast distilled transformer model delivering cinematic 24 FPS video sequences in only 8 to 12 inference steps.

### 2. Video Controls and Parameters

- **Image-to-Video (I2V):** Upload any reference image or pick any generated still directly from the gallery.
- **Frame Selection ($4k+1$ Rule):** Built-in presets for 17, 33, 49, and 65 frames conforming to 3D VAE temporal downsampling.
- **Motion and Quality Controls:** Adjustable frame rates (16 fps and 24 fps), Flow Shift (3.0 to 5.0), Motion Strength, CFG Scale, Seed, and dedicated samplers (Euler, UniPC).
- **Video LoRA Support:** Dynamic stacking and strength weighting for compatible video LoRAs.

### 3. Memory Containment Strategies

Video diffusion requires substantially more memory than still image generation. Machete PS5SD employs three critical safeguards:

- **Spatial VAE Tiling:** Decodes latents into pixel frames using overlapping spatial tiles (`--vae-tiling` with `--vae-tile-size 512`), preventing VRAM spikes during high-resolution video decode.
- **Sequential Encoder Offloading:** Loads the UMT5-XXL text encoder, computes prompt embeddings, and promptly frees its memory prior to loading and executing the main diffusion transformer.
- **Optimized Attention:** Employs `--flash-attn` kernel optimizations to minimize intermediate attention matrix storage.
- **Container Output:** Encodes completed frames into lightweight MP4 and WebM containers playable directly in the browser and stored in the gallery.

---

## Intelligent TV Dashboard and Instant QR Connection

### Real Console IP Detection
The TV interface queries the Prospero/FreeBSD kernel routing table using non-blocking UDP probe sockets and `getifaddrs`. The actual IPv4 address assigned by DHCP is immediately displayed on the television screen instead of generic placeholders like `<console ip>`.

### TV QR Code Generation
An embedded C++ QR generation library (Project Nayuki) renders a high-contrast QR code directly to the screen via the 2D hardware-accelerated UI pipeline. Pointing a smartphone or tablet camera at the TV connects the device to `http://<console-ip>:7860/` instantly.

### Seamless Token Pass-Through
When an API key is enabled in settings, the generated QR code embeds the session key as a hash fragment (`#key=...`). The mobile browser automatically reads the fragment, sets the authentication cookie, and opens the studio interface with zero friction.

---

## Hardware Architecture and Memory Budget

The PlayStation 5 features 16 GiB of shared GDDR6 memory across the AMD Zen 2 CPU and RDNA 2 GPU:

- **System and Kernel Allocation:** Approximately 4.5 to 5.0 GiB is reserved for the Prospero kernel, operating system daemons, and system services.
- **Usable Unified VRAM:** The userland title has access to roughly 11.44 GiB through RADV's unified APU heap (`radv_enable_unified_heap_on_apu`).
- **Safety Ceiling:** Machete PS5SD strictly enforces an 11.0 GiB memory budget across model weights, context tensors, KV caches, and VAE decode buffers. Exceeding this boundary triggers kernel OOM termination.

The application runs as a native title fSELF registered via ShadowMountPlus with title ID `PPSA99699`. As a native title rather than a payload, it retains exclusive hardware access to the GPU and direct physical memory mappings.

---

## Requirements

- PlayStation 5 console on jailbreakable firmware **7.00 through 13.60**.
- Homebrew environment:
  - [etaHEN](https://github.com/LightningMods/etaHEN) with kstuff-lite.
  - ShadowMountPlus.
  - elfldr (port 9021).
  - Built-in or external FTP service (`ftpsrv`). A dedicated internal instance runs at `127.0.0.1:2122` for high-speed local file handling without operating system write throttling.
- A computer, smartphone, or tablet on the same local network (Wi-Fi or Ethernet) to access the Web UI.

---

## Installation Guide

1. Download the release package `PPSA99699.zip` from the [Releases](https://github.com/VenezuelanBiggie24/Machete-PS5SD-IMG-and-VID/releases) page.
2. Unpack the `PPSA99699` folder into `/data/homebrew/` on your PS5 console internal storage.
3. Rescan installed applications with ShadowMountPlus or install directly through the Homebrew Store.
4. Launch **Machete PS5SD** from the PlayStation 5 home menu.
5. Scan the QR code on your TV screen using your phone camera, or navigate to `http://<console-ip>:7860` in any web browser.

---

## Network Configuration and Environment Variables

Optional operational parameters can be customized by creating or modifying `/data/PS5SD/env.txt`:

| Variable | Description | Default Value |
|---|---|---|
| `PS5SD_IP` / `PS5SD_ADDRESS` | Override automatic LAN IP detection on the TV dashboard | Auto-detected |
| `PS5SD_PORT` | HTTP port for the Web UI and REST API | `7860` |
| `PS5SD_DL_CONNECTIONS` | Number of parallel HTTP connections for model downloads | `16` |
| `PS5SD_DL_PIECE_MIB` | Chunk size in MiB for segmented file downloads | `8` |

---

## HTTP REST API and Client Integration

The integrated HTTP server on port 7860 serves both the Web UI and multiple industry-standard API endpoints:

- `/`: Interactive Web UI Studio.
- `/sdapi/v1/`: AUTOMATIC1111-compatible WebUI API for external integrations.
- `/v1/`: OpenAI-compatible images API (`/v1/images/generations`).
- `/sdcpp/v1/`: Native `stable-diffusion.cpp` async job execution API.
- `/ps5sd/v1/`: Machete PS5SD endpoints for gallery management, hardware health, and model downloads.

Example generation request via curl:

```sh
curl -X POST http://<console-ip>:7860/sdapi/v1/txt2img \
  -H "Content-Type: application/json" \
  -d '{"prompt": "cinematic neon cyberpunk city at night", "steps": 20, "width": 512, "height": 512}'
```

---

## Credits and Acknowledgments

Machete PS5SD is built upon the dedicated research and open-source contributions of the PlayStation 5 homebrew and generative AI communities:

- **[Darkatek7 / Heel Tobias (PS5SD)](https://github.com/Darkatek7/PS5SD):** Original creator and lead architect of PS5SD. His foundational engineering on the native title runtime, Vulkan backend integration, HTTP server, and memory management made this project possible.
- **[leejet / stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp):** High-performance C/C++ inference implementation for Stable Diffusion, FLUX, and Wan/LTX video models.
- **[ggerganov / ggml](https://github.com/ggml-org/ggml):** Core tensor library, quantization formats, and Vulkan compute backend.
- **[cobanov / PS5LM](https://github.com/cobanov/PS5LM):** Groundbreaking large language model implementation on PlayStation 5, pioneering direct unified memory utilization on the console.
- **[mihawk-99 / PS5_Vulkan and PS5_Mesa](https://github.com/mihawk-99/PS5_Vulkan):** Vulkan driver stack, Mesa port, and RADV implementation for Prospero.
- **[blackbearreloaded and Phononenom / ps5-homebrew-ui](https://github.com/Phononenom/ps5-homebrew-ui):** Hardware-accelerated 2D graphics framework powering the on-screen TV dashboard.
- **[Project Nayuki (QR Code generator)](https://www.nayuki.io/page/qr-code-generator-library):** Embedded C++ QR code generation library.
- **[Machete Team & VenezuelanBiggie24](https://github.com/VenezuelanBiggie24/Machete-PS5SD-IMG-and-VID):** Video generation architecture (T2V/I2V), dynamic network IP detection, TV QR code integration, and studio packaging.

---

## License

Machete PS5SD is distributed under the **GNU General Public License v3.0 or later (GPL-3.0-or-later)**.  
See the [LICENSE](LICENSE) file for complete details. Third-party components and submodules retain their respective upstream licenses.

---

<div align="center">

# Machete PS5SD: Estudio de Imagen y Video

**Generación local de Imágenes y Video impulsada por IA en la GPU de PlayStation 5**  
*Basado en `stable-diffusion.cpp` y ggml-vulkan sobre RADV, con interfaz web responsive y dashboard en TV.*

[![Licencia: GPL v3](https://img.shields.io/badge/Licencia-GPLv3-blue.svg)](LICENSE)
[![Plataforma: PS5](https://img.shields.io/badge/Plataforma-PS5%20(7.00--13.60)-purple.svg)](#requisitos-1)
[![Generación de Video](https://img.shields.io/badge/IA-Text--to--Video%20%7C%20Image--to--Video-00e5ff.svg)](#modelos-de-generación-de-video)
[![Upstream](https://img.shields.io/badge/Upstream-Darkatek7%2FPS5SD-brightgreen.svg)](https://github.com/Darkatek7/PS5SD)

</div>

---

## Tabla de Contenidos

- [Resumen](#resumen)
- [Nuevas Características en v0.5.0](#nuevas-características-en-v050)
- [Modelos de Generación de Video](#modelos-de-generación-de-video)
- [Dashboard Inteligente en TV y Conexión Instantánea por QR](#dashboard-inteligente-en-tv-y-conexión-instantánea-por-qr)
- [Arquitectura de Hardware y Presupuesto de Memoria](#arquitectura-de-hardware-y-presupuesto-de-memoria)
- [Requisitos](#requisitos-1)
- [Guía de Instalación](#guía-de-instalación)
- [Configuración de Red y Variables de Entorno](#configuración-de-red-y-variables-de-entorno)
- [API REST HTTP e Integración de Clientes](#api-rest-http-e-integración-de-clientes)
- [Créditos y Agradecimientos](#créditos-y-agradecimientos)
- [Licencia](#licencia-1)

---

## Resumen

Machete PS5SD es una suite completa y autónoma de inteligencia artificial generativa diseñada específicamente para aprovechar los 11.0 GiB de memoria unificada utilizables en consolas PlayStation 5 liberadas.

A partir del trabajo fundacional de Darkatek7 / Heel Tobias en [PS5SD](https://github.com/Darkatek7/PS5SD), esta versión Machete expande la plataforma con soporte integral para Generación de Video (tanto Text-to-Video como Image-to-Video), detección de la IP local real de la consola, un código QR en pantalla para emparejamiento inmediato con dispositivos móviles, y gestión de memoria optimizada para evitar excepciones por falta de memoria en el sistema.

---

## Nuevas Características en v0.5.0

- Pipeline completo de generación Text-to-Video (T2V) e Image-to-Video (I2V) en C/C++ integrado con la interfaz web.
- Editor completo de Inpainting e Image-to-Image (pintar máscara, borrador, invertir tamaño de pincel y soporte para cursor de DualSense) con fusión suave de bordes.
- Modelos especializados de inpainting añadidos al catálogo: SDXL Inpainting 0.1 (Q8_0) y SD 1.5 Inpainting (Q8_0).
- Catálogo de modelos de video con descarga en 1-clic para Wan 2.2, Wan 2.1 y LTX-Video.
- Botón de animación directa ("Animate this image") disponible en la galería y en la vista lightbox.
- Selección estricta de fotogramas según la regla $4k+1$ (17, 33, 49 y 65 frames) para preservar la coherencia temporal.
- Detección automática de la dirección IPv4 local de la consola mediante consultas a la tabla de enrutamiento del kernel.
- Código QR de alto contraste renderizado por GPU en la TV para conectar navegadores móviles sin escribir la dirección IP.
- Autenticación automática mediante fragmentos hash en el código QR cuando la API key está activada.
- Tiling espacial del VAE, descarga secuencial del codificador de texto y flash attention para no superar el límite de 11.0 GiB de VRAM.
- Empaquetado automático en contenedores MP4 y WebM con reproducción nativa en el navegador.

---

## Modelos de Generación de Video

Machete PS5SD incorpora modelos de difusión temporal optimizados para la arquitectura APU unificada de PS5:

### 1. Catálogo de Modelos Integrado

- **Wan 2.2 TI2V 5B (Q4_K_M):** Modelo híbrido insignia capaz de generar Text-to-Video e Image-to-Video con alta fidelidad. Optimizado para 832x480 (horizontal) y 480x832 (vertical). Comparte el codificador de texto UMT5-XXL y el VAE de Wan.
- **Wan 2.1 T2V/I2V 1.3B (Q8_0):** Modelo ligero y ultraestable diseñado para iteración rápida. Reutiliza los mismos pesos de UMT5-XXL y VAE de Wan 2.2 sin duplicar espacio en disco.
- **LTX-Video 2B (Q8_0):** Transformador destilado de alta velocidad capaz de producir video cinemático a 24 FPS en solo 8 a 12 pasos de inferencia.

### 2. Controles y Parámetros de Video

- **Image-to-Video (I2V):** Permite subir cualquier imagen de referencia o seleccionar directamente una imagen generada desde la galería.
- **Selección de Cuadros (Regla $4k+1$):** Ajustes preestablecidos para 17, 33, 49 y 65 fotogramas en concordancia con el submuestreo temporal del VAE 3D.
- **Controles de Movimiento y Calidad:** Velocidad de fotogramas seleccionable (16 fps y 24 fps), Flow Shift (3.0 a 5.0), Motion Strength, CFG Scale, Seed y muestreadores dedicados (Euler, UniPC).
- **Compatibilidad con LoRAs de Video:** Carga y combinación dinámica de múltiples LoRAs con control de ponderación.

### 3. Estrategias de Contención de Memoria

La generación de video requiere sustancialmente más memoria que la síntesis de imágenes fijas. Machete PS5SD implementa tres medidas indispensables:

- **Tiling Espacial del VAE:** Decodifica los latentes en fotogramas mediante bloques espaciales superpuestos (`--vae-tiling` con `--vae-tile-size 512`), evitando picos de VRAM durante la fase de decodificación.
- **Descarga Secuencial del Codificador:** Carga el codificador UMT5-XXL, calcula los embeddings del prompt y libera inmediatamente su memoria antes de instanciar y ejecutar el modelo de difusión principal.
- **Atención Optimizada:** Aplica optimizaciones de kernel `--flash-attn` para minimizar la memoria temporal requerida por matrices de atención intermedia.
- **Contenedores de Salida:** Empaqueta los fotogramas resultantes en formatos MP4 y WebM reproducibles directamente en la web y almacenados en la galería local.

---

## Dashboard Inteligente en TV y Conexión Instantánea por QR

### Detección de IP Real de la Consola
La interfaz de televisión consulta la tabla de rutas del kernel Prospero/FreeBSD mediante sockets UDP no bloqueantes y sondeo de `getifaddrs`. La dirección IPv4 asignada por DHCP se presenta de inmediato en la pantalla en lugar de marcadores genéricos.

### Generación de Código QR en TV
Una librería embebida de C++ (Project Nayuki) dibuja un código QR de alto contraste mediante el pipeline de interfaz 2D acelerado por hardware. Al apuntar la cámara de un smartphone o tablet al televisor, el dispositivo abre `http://<console-ip>:7860/` al instante.

### Traspaso Automático de Credenciales
Si se habilita una clave de API en la configuración, el código QR generado incluye el token de sesión en el fragmento hash (`#key=...`). El navegador móvil lee el fragmento de forma automática, establece la cookie de sesión y abre la interfaz sin intervención manual.

---

## Arquitectura de Hardware y Presupuesto de Memoria

PlayStation 5 cuenta con 16 GiB de memoria GDDR6 unificada compartida entre la CPU AMD Zen 2 y la GPU RDNA 2:

- **Asignación del Sistema y Kernel:** Entre 4.5 y 5.0 GiB están reservados para el kernel Prospero, daemons de sistema y servicios internos de la consola.
- **VRAM Unificada Disponible:** El espacio de usuario dispone de aproximadamente 11.44 GiB a través del heap unificado de APU en RADV (`radv_enable_unified_heap_on_apu`).
- **Límite de Seguridad:** Machete PS5SD respeta rigurosamente un presupuesto máximo de 11.0 GiB para pesos del modelo, tensores de contexto, cachés KV y búferes de decodificación. Superar este umbral provocaría el cierre de la aplicación por el kernel (OOM).

La aplicación opera como un título nativo fSELF registrado mediante ShadowMountPlus con title ID `PPSA99699`. Al ser un título nativo y no un payload, tiene acceso directo y prioritario al hardware de la GPU y al mapeo de memoria física.

---

## Requisitos

- Consola PlayStation 5 en firmware liberable **7.00 a 13.60**.
- Entorno homebrew:
  - [etaHEN](https://github.com/LightningMods/etaHEN) con kstuff-lite.
  - ShadowMountPlus.
  - elfldr (puerto 9021).
  - Servidor FTP (`ftpsrv`). La suite incluye una instancia interna en `127.0.0.1:2122` para transferencias locales de alta velocidad sin la limitación de escritura del sistema operativo.
- Un ordenador, teléfono móvil o tablet conectado a la misma red local (Wi-Fi o Ethernet) para usar la interfaz web.

---

## Guía de Instalación

1. Descarga el paquete de lanzamiento `PPSA99699.zip` desde la sección de [Releases](https://github.com/VenezuelanBiggie24/Machete-PS5SD-IMG-and-VID/releases).
2. Descomprime la carpeta `PPSA99699` dentro de `/data/homebrew/` en el almacenamiento interno de la consola.
3. Actualiza la lista de aplicaciones con ShadowMountPlus o instálala directamente desde la Homebrew Store.
4. Inicia **Machete PS5SD** desde el menú principal de la consola.
5. Escanea el código QR en la pantalla del televisor con la cámara de tu teléfono, o abre `http://<console-ip>:7860` en cualquier navegador web.

---

## Configuración de Red y Variables de Entorno

Es posible personalizar parámetros operativos creando o modificando el archivo `/data/PS5SD/env.txt`:

| Variable | Descripción | Valor por Defecto |
|---|---|---|
| `PS5SD_IP` / `PS5SD_ADDRESS` | Sobrescribir la dirección IP mostrada en el dashboard de TV | Auto-detectada |
| `PS5SD_PORT` | Puerto HTTP para la interfaz web y la API REST | `7860` |
| `PS5SD_DL_CONNECTIONS` | Número de conexiones HTTP paralelas para descarga de modelos | `16` |
| `PS5SD_DL_PIECE_MIB` | Tamaño del fragmento en MiB para descargas concurrentes | `8` |

---

## API REST HTTP e Integración de Clientes

El servidor HTTP en el puerto 7860 proporciona tanto la interfaz de usuario como varios endpoints compatibles con estándares de la industria:

- `/`: Estudio Web interactivo.
- `/sdapi/v1/`: API compatible con AUTOMATIC1111 para integraciones de terceros.
- `/v1/`: API compatible con el formato de imágenes de OpenAI (`/v1/images/generations`).
- `/sdcpp/v1/`: API nativa de ejecución asíncrona de `stable-diffusion.cpp`.
- `/ps5sd/v1/`: Endpoints propios de Machete PS5SD para administración de galería, estado del sistema y descargas.

Ejemplo de petición de generación mediante curl:

```sh
curl -X POST http://<console-ip>:7860/sdapi/v1/txt2img \
  -H "Content-Type: application/json" \
  -d '{"prompt": "cinematic neon cyberpunk city at night", "steps": 20, "width": 512, "height": 512}'
```

---

## Créditos y Agradecimientos

Machete PS5SD es el resultado del trabajo acumulado y la colaboración de las comunidades de homebrew y de inteligencia artificial:

- **[Darkatek7 / Heel Tobias (PS5SD)](https://github.com/Darkatek7/PS5SD):** Creador original y arquitecto principal de PS5SD. Su labor pionera en el runtime nativo de PS5, la integración del backend Vulkan, el servidor HTTP y el esquema de memoria directa constituye la base de este proyecto.
- **[leejet / stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp):** Motor de inferencia en C/C++ de alto rendimiento para Stable Diffusion, FLUX y modelos de video Wan/LTX.
- **[ggerganov / ggml](https://github.com/ggml-org/ggml):** Librería fundamental de tensores, esquemas de cuantización y backend de cómputo Vulkan.
- **[cobanov / PS5LM](https://github.com/cobanov/PS5LM):** Proyecto pionero de modelos de lenguaje en PlayStation 5, abriendo el camino para el aprovechamiento de la memoria unificada directa en la consola.
- **[mihawk-99 / PS5_Vulkan y PS5_Mesa](https://github.com/mihawk-99/PS5_Vulkan):** Stack de controladores Vulkan, port de Mesa y capa RADV para Prospero.
- **[blackbearreloaded y Phononenom / ps5-homebrew-ui](https://github.com/Phononenom/ps5-homebrew-ui):** Framework gráfico 2D acelerado por hardware para los menús en TV.
- **[Project Nayuki (QR Code generator)](https://www.nayuki.io/page/qr-code-generator-library):** Librería embebida de generación de códigos QR en C++.
- **[Equipo Machete & VenezuelanBiggie24](https://github.com/VenezuelanBiggie24/Machete-PS5SD-IMG-and-VID):** Arquitectura de generación de video (T2V/I2V), detección dinámica de red, integración de código QR en TV y empaquetado de la suite.

---

## Licencia

Machete PS5SD se distribuye bajo los términos de la **GNU General Public License v3.0 or later (GPL-3.0-or-later)**.  
Consulte el archivo [LICENSE](LICENSE) para más detalles. Los componentes y submódulos de terceros preservan sus respectivas licencias originales.
