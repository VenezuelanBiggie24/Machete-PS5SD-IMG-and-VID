<div align="center">

<img src="assets/machete-icon.png" alt="Machete PS5SD Icon" width="160" height="160" style="border-radius: 28px; box-shadow: 0 8px 24px rgba(0,0,0,0.5);">

# Machete PS5SD: Image & Video Studio

**Local AI-Powered Image and Video Generation on the PlayStation 5 GPU**  
*Built on stable-diffusion.cpp and ggml-vulkan over RADV, featuring a responsive web UI and TV dashboard.*

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Platform: PS5](https://img.shields.io/badge/Platform-PS5%20(7.00--13.60)-purple.svg)](#requirements)
[![Video Generation](https://img.shields.io/badge/AI-Text--to--Video%20%7C%20Image--to--Video-00e5ff.svg)](#video-generation-t2v--i2v)
[![Upstream](https://img.shields.io/badge/Upstream-Darkatek7%2FPS5SD-brightgreen.svg)](https://github.com/Darkatek7/PS5SD)

</div>

---

## Table of Contents

- [Overview](#overview)
- [New Features in v0.9.6](#new-features-in-v096)
- [Video Generation Models](#video-generation-models)
- [Live Console Telemetry](#live-console-telemetry)
- [TV Dashboard and Instant QR Connection](#tv-dashboard-and-instant-qr-connection)
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

Machete PS5SD is a complete generative artificial intelligence suite designed specifically to leverage the unified memory architecture of jailbroken PlayStation 5 consoles.

Building on the foundation created by Darkatek7 / Heel Tobias in [PS5SD](https://github.com/Darkatek7/PS5SD), this Machete release integrates full video generation (Text-to-Video and Image-to-Video), hardware telemetry, memory defragmentation, accelerated decoding, and automated crash diagnostics.

---

## New Features in v0.9.6

- **Video Generation (Wan 2.2, Wan 2.1, LTX-Video)**: Generate video sequences up to 81 frames with Wan 2.2 TI2V 5B, Wan 2.1 T2V 1.3B, and LTX-Video 2B.
- **CausVid Video Acceleration**: Generate a 2-second Wan video (33 frames at 832x480) in approximately 3 minutes and 40 seconds with 4 steps and CFG 1, compared to ~15 minutes standard.
- **Fast Video Decoding (TAEHV)**: Decodes 33 frames in 3.6 seconds instead of 55 seconds using the optimized TAEHV autoencoder.
- **Live Hardware Telemetry**: Real-time display of SoC temperature, total power consumption in watts, cooling fan drive percentage, and CPU/GPU load on the TV dashboard and in the web UI.
- **ControlNet SDXL**: Guide image layout with Canny edge detection directly on the console.
- **Modernized Web UI**: Modular interface featuring desktop sidebar tabs, mobile bottom navigation bar, pinch/drag zoom in gallery, search tags (#tag), Danbooru tag completions, and background queue processing.
- **Diagnostic Logging and Crash Recovery**: Automatic dual-location logs (/data/homebrew/PPSA99699/app.log) and atomic crash reports (/data/homebrew/PPSA99699/crash.log) with POSIX signal handlers (SIGSEGV, SIGBUS, SIGABRT, SIGFPE, SIGILL).

---

## Video Generation Models

| Model | Type | Architecture / Quant | VRAM / RAM | Speed / Steps |
|---|---|---|---|---|
| Wan 2.2 TI2V 5B | Image-to-Video / T2V | QuantStack Q8_0 + UMT5 Q4_K_M + Wan 2.2 VAE | ~10.5 GB | 33 frames at 832x480 in ~6 min (or ~16 min at 1280x704) |
| Wan 2.1 T2V 1.3B (Fast Decoder) | Text-to-Video | FP16 + UMT5 Q4_K_M + TAEHV Decoder | ~6.7 GB | 33 frames in ~1.5 min with CausVid LoRA (3.6s decode) |
| LTX-Video 2B Distilled | Text-to-Video | Distilled Q8_0 + T5-XXL Q4_K_M | ~5.0 GB | 8-12 steps distilled, 33 frames at 768x512 |

---

## Live Console Telemetry

When ps5-exporter is running via the console ELF loader (port 9021), Machete PS5SD monitors system sensors:
- **SoC Temperature**: Highest reading across internal APU thermal sensors.
- **Power**: Total system SoC consumption in watts.
- **Fan**: Pulse-width drive percentage (0 to 100%).
- **CPU / GPU Load**: Percentage of active cycles utilized during diffusion and VAE decoding.

---

## TV Dashboard and Instant QR Connection

The on-screen TV dashboard renders:
- Current routable console IPv4 address and active listening port.
- Hardware-accelerated QR code for pairing mobile devices without manual IP typing.
- Live system status, active model, memory pool utilization, and telemetry metrics.

---

## Hardware Architecture and Memory Budget

The PS5 features 16 GiB of GDDR6 unified memory shared between CPU and GPU. Under jailbreak environments, approximately 11.0 to 11.5 GiB are available for direct memory allocations.

Machete PS5SD manages this budget via:
- Contiguous GPU buffer defragmentation in RADV.
- Block-based spatial and temporal VAE tiling.
- Sequential text encoder offloading prior to diffusion sampling.

---

## Requirements

- PlayStation 5 Console running compatible firmware (7.00 through 13.60).
- Jailbreak environment: etaHEN with kstuff-lite and ShadowMountPlus.
- elfldr (port 9021) active for ps5-exporter telemetry.
- Optional: ftpsrv running on port 2121 or 2122 for remote file access.

---

## Installation Guide

### Option 1: PS5 Homebrew Catalog (Recommended)
Search for **Machete AI Studio** (Title ID: `PPSA99699`) in the PS5 Homebrew Store and install with one click.

### Option 2: Manual Installation via ZIP
1. Download `PPSA99699.zip` from [Releases](https://github.com/VenezuelanBiggie24/Machete-PS5SD-IMG-and-VID/releases).
2. Extract the archive into `/data/homebrew/` on your console:
   `/data/homebrew/PPSA99699/eboot.bin`
3. Trigger a title scan in ShadowMountPlus.

---

## Network Configuration and Environment Variables

Configuration options can be placed in `/data/PS5SD/env.txt`:
- `PS5SD_ADDRESS=<ip>`: Manually override the console IP address displayed on screen.
- `PS5SD_PORT=7860`: Set custom HTTP server port.
- `PS5SD_DL_CONNECTIONS=16`: Parallel download threads for the model catalog.

---

## HTTP REST API and Client Integration

Machete PS5SD exposes standard endpoints compatible with AUTOMATIC1111 WebUI and OpenAI API:
- `GET /ps5sd/v1/app/info`: System version, build hash, and stable-diffusion.cpp revision.
- `POST /sdcpp/v1/img_gen`: Image generation with optional ControlNet and inpainting.
- `POST /sdcpp/v1/vid_gen`: Video generation pipeline (returns WebM format).
- `GET /ps5sd/v1/gallery`: Query and manage generated media files and tags.

---

## Credits and Acknowledgments

- **Darkatek7 / Heel Tobias**: Creator of [PS5SD](https://github.com/Darkatek7/PS5SD).
- **ggerganov & leejet**: Authors of [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp).
- **Marice**: Author of [ps5-exporter](https://github.com/Marice/ps5-exporter).
- **cobanov**: Author of [PS5LM](https://github.com/cobanov/PS5LM).
- **John Tornblom & drakmor**: Authors of [ftpsrv](https://github.com/drakmor/ftpsrv).

---

## License

This project is licensed under the GNU General Public License v3.0 (GPL-3.0). See [LICENSE](LICENSE) for details.

---
---

# Machete PS5SD: Estudio de Imagen y Video

**Generacion Local de Imagen y Video con IA en la GPU de PlayStation 5**  
*Basado en stable-diffusion.cpp y ggml-vulkan sobre RADV, con interfaz web responsiva y dashboard para TV.*

---

## Tabla de Contenidos

- [Resumen](#resumen)
- [Nuevas Caracteristicas en v0.9.6](#nuevas-caracteristicas-en-v096)
- [Modelos de Generacion de Video](#modelos-de-generacion-de-video)
- [Telemetria de Consola en Vivo](#telemetria-de-consola-en-vivo)
- [Dashboard de TV y Conexion Instantanea QR](#dashboard-de-tv-y-conexion-instantanea-qr)
- [Presupuesto de Memoria y Rendimiento](#presupuesto-de-memoria-y-rendimiento)
- [Requisitos](#requisitos-1)
- [Guia de Instalacion](#guia-de-instalacion)
- [Variables de Entorno](#variables-de-entorno)
- [API REST y Clientes](#api-rest-y-clientes)
- [Creditos y Agradecimientos](#creditos-y-agradecimientos)
- [Licencia](#licencia)

---

## Resumen

Machete PS5SD es una suite completa de inteligencia artificial generativa disenada especificamente para aprovechar la arquitectura de memoria unificada en consolas PlayStation 5 liberadas.

Construido sobre las bases creadas por Darkatek7 / Heel Tobias en [PS5SD](https://github.com/Darkatek7/PS5SD), esta version de Machete integra generacion de video completa (Texto a Video e Imagen a Video), telemetria de hardware, desfragmentacion de memoria, decodificacion acelerada y diagnostico automatico de fallos.

---

## Nuevas Caracteristicas en v0.9.6

- **Generacion de Video (Wan 2.2, Wan 2.1, LTX-Video)**: Secuencias de video de hasta 81 fotogramas con Wan 2.2 TI2V 5B, Wan 2.1 T2V 1.3B y LTX-Video 2B.
- **Aceleracion de Video CausVid**: Genera un video Wan de 2 segundos (33 frames a 832x480) en aproximadamente 3 minutos y 40 segundos con 4 pasos y CFG 1, frente a los ~15 minutos habituales.
- **Decodificacion Rapida de Video (TAEHV)**: Decodifica 33 fotogramas en 3.6 segundos en lugar de 55 segundos mediante el autoencoder optimizado TAEHV.
- **Telemetria de Hardware en Vivo**: Monitorizacion en tiempo real de temperatura del SoC, consumo de potencia en vatios, porcentaje del ventilador y uso de CPU/GPU en el dashboard de TV y en la web.
- **ControlNet SDXL**: Guiado visual de composicion mediante deteccion de bordes Canny directamente en la consola.
- **Interfaz Web Renovada**: Diseno modular con pestanas laterales en PC, navegacion inferior en moviles, zoom en galeria, etiquetas (#tag), autocompletado Danbooru y cola de tareas en segundo plano.
- **Registro de Fallos y Diagnostico**: Registro continuo en /data/homebrew/PPSA99699/app.log y generacion atomica de crash.log ante excepciones o senales POSIX (SIGSEGV, SIGBUS, SIGABRT, SIGFPE, SIGILL).

---

## Modelos de Generacion de Video

| Modelo | Tipo | Arquitectura / Cuantizacion | VRAM / RAM | Velocidad / Pasos |
|---|---|---|---|---|
| Wan 2.2 TI2V 5B | Imagen a Video / T2V | QuantStack Q8_0 + UMT5 Q4_K_M + Wan 2.2 VAE | ~10.5 GB | 33 frames a 832x480 en ~6 min (o ~16 min a 1280x704) |
| Wan 2.1 T2V 1.3B (Decodificador Rapido) | Texto a Video | FP16 + UMT5 Q4_K_M + Decodificador TAEHV | ~6.7 GB | 33 frames en ~1.5 min con LoRA CausVid (3.6s decodificacion) |
| LTX-Video 2B Distilled | Texto a Video | Distilled Q8_0 + T5-XXL Q4_K_M | ~5.0 GB | 8-12 pasos destilados, 33 frames a 768x512 |

---

## Telemetria de Consola en Vivo

Cuando ps5-exporter esta activo a traves del cargador ELF (puerto 9021), Machete PS5SD monitoriza:
- **Temperatura SoC**: Registro del sensor termico mas caliente del procesador.
- **Potencia**: Consumo global del chip en vatios.
- **Ventilador**: Porcentaje de modulacion de velocidad (0 a 100%).
- **Carga CPU / GPU**: Porcentaje de tiempo de computo durante difusion y procesado VAE.

---

## Dashboard de TV y Conexion Instantanea QR

El panel en pantalla muestra:
- Direccion IPv4 enrutada real y puerto de escucha activo.
- Codigo QR acelerado por hardware para sincronizar moviles sin escribir la IP manualmente.
- Estado del sistema, modelo cargado, memoria directa aprovechable y telemetria.

---

## Presupuesto de Memoria y Rendimiento

La consola dispone de 16 GiB de memoria GDDR6 unificada compartida entre CPU y GPU. En consolas liberadas, entre 11.0 y 11.5 GiB quedan disponibles para ejecucion directa.

Machete PS5SD gestiona este presupuesto mediante:
- Desfragmentacion de buffers contiguos de GPU en el controlador RADV.
- Division por bloques (tiling) espacial y temporal en el VAE.
- Descarga secuencial de codificadores de texto previa al muestreo.

---

## Requisitos

- Consola PlayStation 5 en firmware compatible (7.00 hasta 13.60).
- Entorno de liberacion: etaHEN con kstuff-lite y ShadowMountPlus.
- elfldr activo en puerto 9021 para telemetria de ps5-exporter.
- Opcional: servidor ftpsrv en puerto 2121 o 2122 para transferencia de archivos.

---

## Guia de Instalacion

### Opcion 1: Tienda Homebrew de PS5 (Recomendado)
Busca **Machete AI Studio** (Title ID: `PPSA99699`) en la tienda oficial de Homebrew e instala en un clic.

### Opcion 2: Instalacion Manual mediante ZIP
1. Descarga `PPSA99699.zip` desde [Releases](https://github.com/VenezuelanBiggie24/Machete-PS5SD-IMG-and-VID/releases).
2. Descomprime el contenido en `/data/homebrew/` en tu consola:
   `/data/homebrew/PPSA99699/eboot.bin`
3. Ejecuta un escaneo de aplicaciones en ShadowMountPlus.

---

## Variables de Entorno

Puedes configurar opciones en `/data/PS5SD/env.txt`:
- `PS5SD_ADDRESS=<ip>`: Forzar manualmente la direccion IP mostrada en pantalla.
- `PS5SD_PORT=7860`: Cambiar el puerto HTTP del servidor.
- `PS5SD_DL_CONNECTIONS=16`: Hilos simultaneos de descarga para el catalogo.

---

## API REST y Clientes

Machete PS5SD expone rutas compatibles con AUTOMATIC1111 y OpenAI:
- `GET /ps5sd/v1/app/info`: Version, identificador de build y version de stable-diffusion.cpp.
- `POST /sdcpp/v1/img_gen`: Generacion de imagenes con soporte para ControlNet e inpainting.
- `POST /sdcpp/v1/vid_gen`: Generacion de video nativo en formato WebM.
- `GET /ps5sd/v1/gallery`: Consulta y gestion de imagenes, videos y etiquetas.

---

## Creditos y Agradecimientos

- **Darkatek7 / Heel Tobias**: Autor y creador de [PS5SD](https://github.com/Darkatek7/PS5SD).
- **ggerganov y leejet**: Desarrolladores de [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp).
- **Marice**: Autor de [ps5-exporter](https://github.com/Marice/ps5-exporter).
- **cobanov**: Creador de [PS5LM](https://github.com/cobanov/PS5LM).
- **John Tornblom y drakmor**: Creadores de [ftpsrv](https://github.com/drakmor/ftpsrv).

---

## Licencia

Este proyecto esta publicado bajo la Licencia Publica General de GNU v3.0 (GPL-3.0). Consulta [LICENSE](LICENSE) para mas detalles.
