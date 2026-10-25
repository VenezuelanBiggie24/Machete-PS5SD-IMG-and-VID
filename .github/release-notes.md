Machete AI Studio 1.1.1 delivers a commercial-grade generative AI web studio interface comparable to top-tier commercial AI suites, integrates generation duration tracking in saved image metadata, and incorporates upstream model defaults and per-model preset memory, while preserving 100% of Machete's Cinema Studio TV Dashboard, Face Studio, and Telegram Companion Bot.

## Enhancements & Architecture

- **Commercial-Grade Studio Web UI**:
  - Obsidian Space dark theme with luminous cyan and violet accents, frosted glassmorphism (`backdrop-filter: blur(20px)`), and porcelain light mode support.
  - 8 Creative Style Chips: Cinematic 8K, Photorealistic, Cyberpunk Neon, Anime Aesthetic, Studio Product, Dark Fantasy, Retro 90s, Concept Art.
  - Quick Negative Filter Chips: Instant 1-tap insertion of quality boosters, anatomical safeguards, and watermark suppression.
  - Visual Aspect Ratio Selector: 6 proportion presets (1:1 Square, 16:9 Wide, 9:16 Story, 4:3 Classic, 3:2 Photo, 21:9 Cinema) with interactive proportion glyphs.
  - Mobile Ergonomics & Docked CTA: Bottom docked generation controls on mobile devices for seamless one-handed generation without scrolling.
  - Clean TV Cursor Separation: Distinct font scaling isolating TV joystick pointer navigation (`html.tv`) from mobile and desktop views (`html`).
  - Prompt Clear Tool: Dedicated quick clear control for prompt authoring.
- **Generation Duration Metadata (Patch 0074)**:
  - Exact generation duration recorded in saved PNG tEXt parameters (`Time: 52.3 s`) and sdcpp JSON payload.
  - Gallery viewer displays elapsed generation duration (`took 52.3 s`).
  - Automatic model naming resolution from diffusion model component for multi-file architectures (FLUX, Z-Image, Wan).
- **Per-Model Default Preset Memory**:
  - Console settings (`default_presets`) now persist user-selected default presets per model file across reboots.
- **FLUX.2 Klein 9B Optimized Defaults**:
  - Tuned defaults for Klein 9B (8 steps, dpm2 sampler, CFG 1) delivering enhanced detail and high fidelity.
- **Hardware & Engine Foundations (Preserved)**:
  - Active hardware fan control payload (`ps5sd-fan.elf`) with port 9021 ELF loader and port 9075 thermal watchdog.
  - 8 MB chunked tensor streaming reducing peak CPU heap allocation by 45%.
  - Single-retry OOM recovery and ThreadGuard exception boundaries.
  - Cinema Studio TV Dashboard (1064px canvas, 4-dial HUD telemetry).
  - Face Studio (PhotoMaker identity preservation).
  - Telegram Companion Bot with remote prompt generation.

## Install

Install from the homebrew store (Machete AI Studio, PPSA99699), or unpack PPSA99699.zip into /data/homebrew/ on the console and let ShadowMountPlus scan. Requirements: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv on 2121 optional.

Credits: PS5SD by Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice), Machete Studio additions by VenezuelanBiggie24.

----

Machete AI Studio 1.1.1 ofrece una interfaz web de estudio de IA generativa de nivel comercial comparable a las herramientas lideres del mercado, incorpora registro de tiempo de generacion en los metadatos de las imagenes guardadas, y anade persistencia de ajustes predeterminados por modelo y sincronizacion de novedades upstream, manteniendo intactos el panel Cinema Studio TV, Face Studio y el Bot Acompanante de Telegram.

## Mejoras y Arquitectura

- **Interfaz Web de Estudio Profesional**:
  - Tema oscuro Obsidian Space con acentos luminosos en cian y violeta, paneles de cristal esmerilado (`backdrop-filter: blur(20px)`) y soporte para modo claro porcelana.
  - 8 Presets Creativos de Estilo: Cinematic 8K, Photorealistic, Cyberpunk Neon, Anime Aesthetic, Studio Product, Dark Fantasy, Retro 90s, Concept Art.
  - Filtros Rapidos de Prompt Negativo: Insercion en 1 toque de mejoras de calidad, correccion anatomica y eliminacion de marcas de agua.
  - Selector Visual de Relacion de Aspecto: 6 proporciones (1:1 Cuadrado, 16:9 Panoramico, 9:16 Historia, 4:3 Clasico, 3:2 Foto, 21:9 Cine) con glifos interactivos.
  - Ergonomia Movil y Boton Flotante: Controles de generacion anclados en la parte inferior en moviles para generacion comoda con una sola mano sin necesidad de deslizar.
  - Separacion de Escala para TV: Tipografia adaptativa que aisla la navegacion con cursor de joystick en TV (`html.tv`) de las pantallas moviles y de escritorio (`html`).
  - Boton Limpiar Prompt: Control directo para limpiar el texto del prompt con un toque.
- **Metadatos de Tiempo de Generacion (Parche 0074)**:
  - Registro exacto de la duracion de cada generacion en los parametros tEXt del PNG (`Time: 52.3 s`) y en el JSON sdcpp.
  - El visor de la galeria muestra el tiempo transcurrido (`took 52.3 s`).
  - Resolucion automatica del nombre del modelo desde el componente diffusion model en arquitecturas multifile (FLUX, Z-Image, Wan).
- **Memoria de Ajustes Predeterminados por Modelo**:
  - Los ajustes de la consola (`default_presets`) ahora guardan la configuracion predeterminada elegida por el usuario para cada archivo de modelo.
- **Valores Optimizados para FLUX.2 Klein 9B**:
  - Ajustes predeterminados afinados para Klein 9B (8 pasos, muestreador dpm2, CFG 1) proporcionando mayor nivel de detalle y nitidez.
- **Bases de Hardware y Motor (Preservadas)**:
  - Payload de control activo de ventilador (`ps5sd-fan.elf`) con cargador ELF en puerto 9021 y vigilancia termica en puerto 9075.
  - Transmision de tensores en bloques de 8 MB reduciendo el consumo maximo de memoria de CPU un 45%.
  - Reintento automatico ante falta de memoria y proteccion de hilos ThreadGuard.
  - Panel Cinema Studio TV (lienzo de 1064px, telemetria digital HUD de 4 cuadrantes).
  - Face Studio (preservacion de identidad con PhotoMaker).
  - Bot Acompanante de Telegram con generacion remota por comandos y texto.

## Instalacion

Instala desde la tienda homebrew (Machete AI Studio, PPSA99699), o descomprime PPSA99699.zip en /data/homebrew/ en la consola y deja que ShadowMountPlus lo detecte. Requisitos: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv en 2121 opcional.

Creditos: PS5SD de Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice), adaptaciones de Machete Studio por VenezuelanBiggie24.
