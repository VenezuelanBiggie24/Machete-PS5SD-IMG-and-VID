Machete PS5SD 0.9.7 (Machete Studio Suite) introduces Face Studio 1-click identity lock, external USB storage support, HTTP 206 download resumption, Thermal Guardian APU safety alerts, a TV ambient art screensaver, and a remote Telegram companion bot.

## New

- **Face Studio (Identity Lock)**: Upload a portrait once and preserve facial likeness automatically across generations using native PhotoMaker integration or facial reference guidance.
- **Multi-storage engine & USB drives**: Full support for external USB drives (/mnt/usb0 .. /mnt/usb7) and decoupled console storage (/data/MacheteAI). Preserves downloaded models across clean app reinstalls and reuses existing models in /data/PS5SD/models automatically.
- **HTTP Range download resumption**: Large multi-gigabyte model downloads now support HTTP 206 Range headers. Interrupted downloads resume from their last byte instead of starting over.
- **Thermal Guardian alerts**: Configurable SoC temperature safety thresholds (default 80 C) with real-time visual alerts on both TV and Web to safeguard the console APU.
- **TV ambient art screensaver**: Automatically presents generated artworks in full screen after 60 seconds of inactivity on the TV.
- **Remote Telegram companion bot**: Generate images and inspect console telemetry remotely from your phone via Telegram while the PS5 runs in the background.
- **Zero-hardcoded independence**: Eliminates rigid path dependencies across the codebase, allowing full peaceful coexistence with upstream PS5SD (PPSA99576).

## Install

Install from the homebrew store (Machete AI Studio, PPSA99699), or unpack PPSA99699.zip into /data/homebrew/ on the console and let ShadowMountPlus scan. Requirements: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv on 2121 optional.

Credits: PS5SD by Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice).

----

Machete PS5SD 0.9.7 (Machete Studio Suite) incorpora Face Studio para bloqueo de identidad en 1 clic, soporte para memorias USB externas, reanudacion de descargas HTTP 206, alertas de seguridad de Guardian Termico, salvapantallas ambiental en TV y bot acompanante remoto de Telegram.

## Nuevo

- **Face Studio (Bloqueo de identidad)**: Sube una foto de tu cara y genera imagenes conservando tu identidad facial de manera automatica con PhotoMaker o guia facial.
- **Motor multi-almacenamiento y USB**: Soporte completo para memorias USB externas (/mnt/usb0 .. /mnt/usb7) y almacenamiento desacoplado (/data/MacheteAI). Protege los modelos ante reinstalaciones limpias y reutiliza modelos existentes en /data/PS5SD/models automaticamente.
- **Reanudacion de descargas HTTP Range**: Las descargas pesadas de modelos admiten ahora cabeceras HTTP 206 Range. Si la conexion se corta, continua desde el ultimo byte descargado.
- **Alertas de Guardian Termico**: Limites de temperatura de seguridad configurables (80 C por defecto) con avisos visuales en TV y Web para proteger el APU.
- **Salvapantallas ambiental en TV**: Muestra una galeria de arte a pantalla completa con tus creaciones tras 60 segundos de inactividad en la TV.
- **Bot acompanante de Telegram**: Genera contenido y consulta temperaturas y telemetria desde tu celular en Telegram con la consola encendida.
- **Independencia sin rutas fijas**: Elimina dependencias de rutas fijas en el codigo, asegurando la coexistencia pacifica con PS5SD original (PPSA99576).

## Instalacion

Instala desde la tienda homebrew (Machete AI Studio, PPSA99699), o descomprime PPSA99699.zip en /data/homebrew/ en la consola y deja que ShadowMountPlus lo detecte. Requisitos: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv en 2121 opcional.

Creditos: PS5SD de Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice).
