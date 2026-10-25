Machete AI Studio 1.1.0 integrates hardware-level active fan control payload execution, 8 MB chunked tensor streaming, automated single-retry memory recovery, thread exception guards, FLUX.2 Klein 9B LoRA mapping, and Web UI appearance and workflow improvements, while keeping 100% of Machete's Cinema Studio TV Dashboard, Face Studio, and Telegram Companion Bot intact.

## Enhancements & Architecture

- **Active Fan Control Payload (ps5sd-fan.elf)**: Hardware cooling integration via port 9021 ELF loader with a 3-second heartbeat on port 9075. Offers Quiet, Balanced (3 deg C below target), Cool (6 deg C below target), and Manual (target down to 70 deg C) modes. Safely steps temperature down 1 degree every 20 seconds, preventing high-temperature throttle states under extended generation runs.
- **8 MB Chunked Tensor Streaming**: Lowers peak CPU heap memory allocation during model loading from 1.33 GiB to 0.72 GiB (45% reduction), avoiding FreeBSD malloc exhaustion on dense models with LoRAs.
- **Graceful OOM Recovery & Auto-Retry**: Automated single-retry recovery for generation jobs that experience memory fragmentation or std::bad_alloc, catching and re-allocating without restarting the application.
- **ThreadGuard Forensics & Safety**: Wraps network downloader, Hugging Face search, console stats, and manager threads with exception boundaries. Integrates std::terminate forensics logging uncaught exception details and direct memory pool statistics to app.log prior to exit.
- **FLUX.2 Klein 9B LoRA Support**: Diffusers layer key mapping support in patch 0070 for PEFT and diffusers-format LoRAs.
- **Web UI Appearance & Ergonomics**:
  - Light and Dark appearance modes with system preference sync.
  - Collapsible compact sidebar mode for tablet and widescreen workflows.
  - Inline 2-tap deletion under the active result ("Delete? Press again") to instantly discard generations.
  - Numeric CLIP skip stepper with model-specific adaptive guidance.
  - Modal scroll lock preventing background scrolling while viewer or settings dialogs are active.
- **Cinema Studio TV Dashboard (Preserved)**: 1064px Hero Artwork Canvas, frosted obsidian glassmorphism, 4-dial digital HUD telemetry, and zero raw log clutter on the TV.
- **Telegram Companion Bot (Preserved)**: Universal resolution parsing, interactive touch keypad, base64 delivery fallback, and direct prompt execution.

## Install

Install from the homebrew store (Machete AI Studio, PPSA99699), or unpack PPSA99699.zip into /data/homebrew/ on the console and let ShadowMountPlus scan. Requirements: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv on 2121 optional.

Credits: PS5SD by Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice), Machete Studio additions by VenezuelanBiggie24.

----

Machete AI Studio 1.1.0 incorpora ejecucion de payload de control activo de ventilador por hardware, carga de tensores en bloques de 8 MB, recuperacion automatica por reintento tras falta de memoria, proteccion de excepciones en hilos, mapeo de LoRAs para FLUX.2 Klein 9B y mejoras de ergonomia en la interfaz web, manteniendo intactos el panel Cinema Studio TV, Face Studio y el Bot Acompanante de Telegram.

## Mejoras y Arquitectura

- **Payload de Control Activo de Ventilador (ps5sd-fan.elf)**: Integracion de refrigeracion por hardware mediante el cargador ELF en puerto 9021 con latido cada 3 segundos en el puerto 9075. Ofrece modos Quiet, Balanced (3 grados C por debajo del objetivo), Cool (6 grados C por debajo) y Manual (objetivo hasta 70 grados C). Ajusta la temperatura progresivamente un grado cada 20 segundos evitando temperaturas excesivas en sesiones prolongadas.
- **Carga de Tensores en Bloques de 8 MB**: Reduce el consumo maximo de memoria heap de la CPU al cargar modelos de 1.33 GiB a 0.72 GiB (reduccion del 45%), previniendo el agotamiento de memoria en modelos complejos con LoRAs.
- **Recuperacion y Reintento Automatico ante OOM**: Reintento automatico unico para trabajos de generacion que experimenten fragmentacion de memoria o std::bad_alloc, resolviendo fallos esporadicos sin reiniciar la aplicacion.
- **Seguridad ThreadGuard y Diagnostico de Cierres**: Proteccion de hilos de descarga, busqueda en Hugging Face, estadisticas y administracion. Incorpora registro en app.log de excepciones no capturadas y balance de memoria unificada previo a cualquier salida de emergencia.
- **Compatibilidad con LoRAs FLUX.2 Klein 9B**: Mapeo de capas diffusers y PEFT en el parche 0070 para modelos Klein 9B.
- **Ergonomia y Apariencia Web UI**:
  - Modo claro y oscuro con deteccion automatica del sistema operativo.
  - Barra lateral colapsable para pantallas compactas o tablets.
  - Boton de borrado en dos toques bajo el resultado activo ("Delete? Press again") para eliminar generaciones no deseadas.
  - Control numerico para CLIP skip con sugerencias adaptadas segun el modelo cargado.
  - Bloqueo de desplazamiento de fondo mientras los visores o paneles emergentes estan abiertos.
- **Panel de TV Cinema Studio (Preservado)**: Lienzo de 1064px, obsidian glassmorphism, 4 cuadrantes HUD de telemetria digital y pantalla libre de texto de depuracion.
- **Bot Acompanante de Telegram (Preservado)**: Deteccion flexible de resoluciones, teclado tactil interactivo, entrega de respaldo en base64 y recepcion directa de prompts.

## Instalacion

Instala desde la tienda homebrew (Machete AI Studio, PPSA99699), o descomprime PPSA99699.zip en /data/homebrew/ en la consola y deja que ShadowMountPlus lo detecte. Requisitos: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv en 2121 opcional.

Creditos: PS5SD de Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice), adaptaciones de Machete Studio por VenezuelanBiggie24.
