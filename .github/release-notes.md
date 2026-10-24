Machete PS5SD 0.9.8 (Machete Studio Suite Hotfix) delivers a complete overhaul of the Telegram Companion Bot, enabling interactive touch menu keyboards, live loopback generation on the PS5 GPU, full multipart image/video uploads to mobile devices, and case-insensitive numbered model selection.

## Hotfix & Improvements

- **Interactive Mobile Keyboard**: Native Telegram ReplyKeyboardMarkup provides one-touch buttons on phones: Generate Image, Generate Video, Models, Parameters, Status, and Help.
- **Active Loopback Generation**: Wired Companion Bot directly to the internal sd-server loopback API (/sdapi/v1/txt2img and /sdcpp/v1/vid_gen), eliminating mock placeholder behavior.
- **Multipart Media Upload**: Finished images (.png) and videos (.webm) are now read from disk and uploaded directly to the Telegram chat using multipart form data.
- **Conversational State Machine**: Introduces distinct states for prompt entry, step adjustment, CFG scale, resolution, and video frame count, preventing numerical settings from being mistakenly treated as image prompts.
- **Numbered and Case-Insensitive Model Selection**: Users can switch models by simply typing the index number (e.g. 1, 2) or model name without worrying about mobile keyboard auto-capitalization.
- **Face Studio & Multi-Storage Integration**: Preserves all 0.9.7 features including 1-click facial identity transfer, USB drive support (/mnt/usb0 .. /mnt/usb7), HTTP 206 download resumption, and Thermal Guardian APU safety monitoring.

## Install

Install from the homebrew store (Machete AI Studio, PPSA99699), or unpack PPSA99699.zip into /data/homebrew/ on the console and let ShadowMountPlus scan. Requirements: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv on 2121 optional.

Credits: PS5SD by Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice).

----

Machete PS5SD 0.9.8 (Machete Studio Suite Hotfix) ofrece una renovacion completa del Bot Acompanante de Telegram, incorporando teclados tactiles interactivos, generacion real en bucle local sobre la GPU de PS5, subida directa de imagenes y videos al celular via multipart, y seleccion de modelos numerada e insensible a mayusculas.

## Correcciones y Mejoras

- **Teclado Tactil Interactivo**: Teclado de respuesta nativo de Telegram con botones de un toque en el celular: Generate Image, Generate Video, Models, Parameters, Status y Help.
- **Generacion Real en Bucle Local**: Bot conectado directamente al API interno del servidor (/sdapi/v1/txt2img y /sdcpp/v1/vid_gen), eliminando respuestas simuladas.
- **Subida de Contenido Multimedia**: Las imagenes generadas (.png) y videos (.webm) se leen del almacenamiento de la consola y se transmiten directamente al chat de Telegram mediante multipart form-data.
- **Maquina de Estados Conversacional**: Separa claramente la entrada de prompts de la configuracion de pasos, CFG, resolucion y fotogramas, evitando que numeros o ajustes se interpreten como prompts de imagen.
- **Seleccion de Modelos Numerada e Insensible a Mayusculas**: Permite cambiar de modelo escribiendo simplemente su numero (ej. 1, 2) o nombre, sin importar las mayusculas del teclado movil.
- **Integracion Completa con Face Studio y USB**: Mantiene todas las capacidades de 0.9.7, incluyendo bloqueo de identidad facial en 1 clic, discos USB (/mnt/usb0 .. /mnt/usb7), reanudacion HTTP 206 y alertas termicas de seguridad del APU.

## Instalacion

Instala desde la tienda homebrew (Machete AI Studio, PPSA99699), o descomprime PPSA99699.zip en /data/homebrew/ en la consola y deja que ShadowMountPlus lo detecte. Requisitos: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv en 2121 opcional.

Creditos: PS5SD de Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice).
