Machete PS5SD 0.5.0 adds video generation on top of everything [PS5SD](https://github.com/Darkatek7/PS5SD) by Darkatek7 already does.

## New

- **Video generation**: Text to Video and Image to Video on the PS5 GPU. Pick a video model and the Create tab shows **Frames** (17 to 65), **FPS** (16 or 24), **Flow Shift** and **Start image**. The result plays in the gallery and downloads as a file.
- **Animate this image**: any image in the gallery or the last result can be sent straight to Image to Video.
- **Video models** in the catalog, one click each: Wan 2.2 TI2V 5B (quality), Wan 2.1 T2V 1.3B (light and fast) and LTX-Video 2B Distilled (8 to 12 steps). Shared files such as the text encoder and VAE are downloaded only once.
- **LoRAs for video**: one or more Wan LoRAs with a strength from 0.0 to 2.0, also added by link from Hugging Face or Civitai.
- **Real console IP and QR code**: the TV shows the console's actual LAN address and a QR code next to it. Scan it with a phone to open the web UI.
- **Version and build** are shown on the TV and in the web UI.
- **Crash log**: `app.log` and, after a crash, `crash.log` are written to `/data/homebrew/PPSA99699/`.

## Install

Install from the homebrew store, or unpack `PPSA99699.zip` into `/data/homebrew/` on the console and let ShadowMountPlus scan. Requirements: etaHEN + kstuff-lite, ShadowMountPlus, elfldr; ftpsrv on 2121 optional.

Credits: [PS5SD](https://github.com/Darkatek7/PS5SD) by Darkatek7 (Heel Tobias), [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp).

----

Machete PS5SD 0.5.0 agrega generación de video sobre todo lo que ya hace [PS5SD](https://github.com/Darkatek7/PS5SD) de Darkatek7.

## Nuevo

- **Generación de video**: Texto a Video e Imagen a Video en la GPU de la PS5. Al elegir un modelo de video, la pestaña Create muestra **Frames** (17 a 65), **FPS** (16 o 24), **Flow Shift** e **Imagen inicial**. El resultado se reproduce en la galería y se descarga como archivo.
- **Animate this image**: cualquier imagen de la galería o el último resultado se puede enviar directo a Imagen a Video.
- **Modelos de video** en el catálogo, con un clic: Wan 2.2 TI2V 5B (calidad), Wan 2.1 T2V 1.3B (ligero y rápido) y LTX-Video 2B Distilled (8 a 12 pasos). Los archivos compartidos, como el codificador de texto y el VAE, se descargan una sola vez.
- **LoRAs de video**: uno o varios LoRAs de Wan con intensidad de 0.0 a 2.0, también agregados por enlace desde Hugging Face o Civitai.
- **IP real de la consola y código QR**: la TV muestra la dirección LAN real de la consola y un código QR al lado. Escanéalo con el teléfono para abrir la interfaz web.
- **Versión y build** visibles en la TV y en la interfaz web.
- **Log de fallos**: `app.log` y, tras un fallo, `crash.log` se guardan en `/data/homebrew/PPSA99699/`.

## Instalación

Instala desde la tienda homebrew, o descomprime `PPSA99699.zip` en `/data/homebrew/` en la consola y deja que ShadowMountPlus lo detecte. Requisitos: etaHEN + kstuff-lite, ShadowMountPlus, elfldr; ftpsrv en 2121 opcional.

Créditos: [PS5SD](https://github.com/Darkatek7/PS5SD) de Darkatek7 (Heel Tobias), [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp).
