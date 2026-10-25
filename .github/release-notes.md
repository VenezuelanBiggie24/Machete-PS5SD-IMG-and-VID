Machete AI Studio 1.2.0 delivers expanded generative model architecture (Sana 1.6B/0.6B linear attention, SD 3.5 Medium MMDiT, Kokoro 82M voice synthesis), validates GFX1030 Packed FP16 dual-issue compute shaders, integrates native DualSense gamepad control and dynamic lightbar/haptic feedback on the TV dashboard, introduces the Director Camera 3D motion suite, and resolves spurious WebUI offline lock issues.

## Enhancements & Architecture

- **Pillar 1: Expanded Generative Models & Kokoro Audio**:
  - Sana 1.6B (1024px) linear attention model for high-resolution generation in 14-20 steps.
  - Sana 0.6B (512px) ultra-lightweight draft model for rapid ideation in 12-14 steps.
  - SD 3.5 Medium (Q8_0) MMDiT model with superior prompt comprehension and typography adherence.
  - Kokoro 82M voice & speech audio synthesis model for prompt/story narration.
  - 1-click installable catalog entries and custom preset profiles (`presets/sana-1.6b.txt`, `presets/sana-0.6b.txt`, `presets/sd3.5-medium.txt`, `presets/kokoro-82m.txt`).
- **Pillar 2: GFX1030 Packed FP16 Compute Shaders**:
  - Validated RADV ACO compiler support for `v_pk_fma_f16` instructions via `shaderFloat16` (`VK_KHR_shader_float16_int8`), unlocking up to ~20.6 TFLOPS arithmetic throughput on PS5 RDNA2 GPU.
  - Compute queue family isolation and multi-add kernel compilation for tensor ops.
- **Pillar 3: DualSense Gamepad & Hardware Mastery**:
  - Native controller polling integrated into the TV main loop via `hui::ps5::Pad`.
  - Direct button shortcuts: Square toggles HUD/log telemetry overlay, Circle toggles Screensaver, L1/Left and R1/Right cycle through gallery images.
  - Dynamic DualSense lightbar states: Amber pulsing during generation, Ambient Purple during screensaver, Cyan/Blue idle.
  - Tactile haptic vibration pulses confirming controller actions.
- **Pillar 4: Creative Studio Director Camera & WebUI Offline Fix**:
  - Camera Director 3D motion controls for video models (Pan Left/Right, Tilt Up/Down, Zoom In/Out, Roll CW/CCW).
  - WebUI Offline Fix: Added missing backend routes (`/ps5sd/v1/auth`, `/ps5sd/v1/health`, `/ps5sd/v1/auth/login`, `/ps5sd/v1/auth/logout`) and HTTP HEAD support to prevent browser timeout lockouts.
  - Guaranteed immediate online state activation (`setOnline(true)`) upon successful authentication check and server load.
- **Foundations Preserved**:
  - Active hardware fan control payload (`ps5sd-fan.elf`) with port 9021 ELF loader and port 9075 thermal watchdog.
  - 8 MB chunked tensor streaming and single-retry OOM recovery.
  - Cinema Studio TV Dashboard, Face Studio (PhotoMaker), and Telegram Companion Bot.

## Install

Install from the homebrew store (Machete AI Studio, PPSA99699), or unpack PPSA99699.zip into /data/homebrew/ on the console and let ShadowMountPlus scan. Requirements: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv on 2121 optional.

Credits: PS5SD by Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice), Machete Studio additions by VenezuelanBiggie24.

----

Machete AI Studio 1.2.0 incorpora arquitectura ampliada de modelos generativos (Sana 1.6B/0.6B con atencion lineal, SD 3.5 Medium MMDiT, sintesis de voz Kokoro 82M), valida shaders de computo Packed FP16 en GFX1030, integra control nativo con mando DualSense y respuesta dinamica de barra de luz y vibracion haptica en la TV, estrena controles de Camara de Director 3D, y soluciona bloqueos por falso estado desconectado en la interfaz web.

## Mejoras y Arquitectura

- **Pilar 1: Modelos Generativos Ampliados y Audio Kokoro**:
  - Modelo Sana 1.6B (1024px) con atencion lineal para generacion de alta resolucion en 14-20 pasos.
  - Modelo ligero Sana 0.6B (512px) para bocetos ultra-rapidos en 12-14 pasos.
  - Modelo SD 3.5 Medium (Q8_0) MMDiT con fidelidad avanzada al prompt y composicion tipografica.
  - Modelo de sintesis de voz Kokoro 82M para narracion y lectura de prompts y relatos.
  - Registros de catalogo con instalacion en un clic y perfiles predeterminados dedicados (`presets/sana-1.6b.txt`, `presets/sana-0.6b.txt`, `presets/sd3.5-medium.txt`, `presets/kokoro-82m.txt`).
- **Pilar 2: Shaders de Computo Packed FP16 en GFX1030**:
  - Soporte validado del compilador ACO en RADV para instrucciones `v_pk_fma_f16` mediante `shaderFloat16` (`VK_KHR_shader_float16_int8`), alcanzando hasta ~20.6 TFLOPS de rendimiento aritmetico en la GPU RDNA2 de PS5.
  - Aislamiento de cola de computo dedicada y compilacion optimizada de kernels de multiplicacion y suma de tensores.
- **Pilar 3: Mando DualSense y Dominio del Hardware**:
  - Bucle de sondeo del mando integrado en el renderizador principal de TV a traves de `hui::ps5::Pad`.
  - Accesos directos con botones: Cuadrado activa/desactiva la telemetria y registros, Circulo activa el salvapantallas, L1/Izquierda y R1/Derecha navegan por la galeria de imagenes.
  - Estados dinamicos en la barra de luz DualSense: Ambar pulsante durante generacion, Purpura ambiental en salvapantallas, Cian/Azul en reposo.
  - Pulsos de vibracion haptica que confirman de forma tactil las acciones del mando.
- **Pilar 4: Camara de Director de Estudio Creativo y Correccion de WebUI**:
  - Controles de movimiento 3D de Camara de Director para modelos de video (Paneo Izquierda/Derecha, Inclinacion Arriba/Abajo, Zoom Acercar/Alejar, Rotacion Horaria/Antihoraria).
  - Correccion de desconexion en WebUI: Incorporacion de rutas de backend faltantes (`/ps5sd/v1/auth`, `/ps5sd/v1/health`, `/ps5sd/v1/auth/login`, `/ps5sd/v1/auth/logout`) y soporte HTTP HEAD para evitar bloqueos por tiempo de espera en el navegador.
  - Activacion inmediata del estado en linea (`setOnline(true)`) tras autenticacion y carga de informacion del servidor.
- **Bases Preservadas**:
  - Payload de control activo de ventilador (`ps5sd-fan.elf`) con cargador ELF en puerto 9021 y vigilancia termica en puerto 9075.
  - Transmision de tensores en bloques de 8 MB y recuperacion automatica ante falta de memoria.
  - Panel Cinema Studio TV, Face Studio (PhotoMaker) y Bot Acompanante de Telegram.

## Instalacion

Instale desde la tienda homebrew (Machete AI Studio, PPSA99699), o descomprima PPSA99699.zip en /data/homebrew/ en la consola y permita que ShadowMountPlus escanee. Requisitos: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv en 2121 opcional.

Creditos: PS5SD por Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice), agregados de Machete Studio por VenezuelanBiggie24.
