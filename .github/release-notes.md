Machete AI Studio 1.2.3 brings critical layout, color contrast, and scroll fixes to the Web Studio, introduces native app icon serving, enables direct Telegram sharing for all media formats (images and videos), wires DualSense Cross and Triangle controls on the TV dashboard, and provides in-app speed guidance for video generation.

## Fixes & Enhancements

- **Web Studio Layout, Contrast & Scroll Polishing**:
  - Restored complete desktop modal styling and z-index layering for the Lightbox viewer (`.lb`), eliminating rendering glitches and element overlapping.
  - Resolved theme color clashes in light mode by replacing hardcoded dark backgrounds with dynamic CSS variables (`--sel-bg`, `--raised`, `--hover-weak`), ensuring high-contrast readability.
  - Fixed mobile generation dock (`.gorow`) margin collisions and sticky positioning to prevent overlapping underlying parameter controls.
  - Corrected sidebar collapse breakpoint so medium tablet viewports maintain full horizontal tab navigation.
  - Upgraded scrollbars to 8px width with accessible contrast thumbs across dark and light themes.

- **App Brand Icon & Favicon Integration**:
  - Implemented `/ps5sd/v1/app/icon` endpoint serving the official `icon0.png` asset with PNG headers and inline SVG fallback.
  - Added official high-resolution application branding to the studio header bar and browser tab favicon.

- **Telegram Companion Sharing for All Media**:
  - Moved the Telegram send action directly to the primary Lightbox toolbar (`.lb-acts`), making it available and functional for both videos and images regardless of metadata presence.
  - Added a one-click Telegram send button to the Gallery multi-selection bar (`#gselbar`).

- **TV Dashboard DualSense Controller Inputs**:
  - Wired `Cross` button input: triggers immediate asynchronous image/video generation with DualSense haptic rumble.
  - Wired `Triangle` button input: tags the active picture as `#favorite` in `.labels.json` with haptic confirmation and HUD notification.

- **Video Generation Acceleration Guidance**:
  - Added an integrated Speed Tip in the Video settings group explaining that Wan 2.1 video generation takes ~2 minutes with the CausVid LoRA (4 to 8 steps, CFG 1), compared to ~14 minutes at 20 steps.

## Install

Install from the homebrew store (Machete AI Studio, PPSA99699), or unpack PPSA99699.zip into /data/homebrew/ on the console and let ShadowMountPlus scan. Requirements: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv on 2121 optional.

Credits: PS5SD by Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice), Machete Studio additions by VenezuelanBiggie24.

----

Machete AI Studio 1.2.3 aporta correcciones criticas de diseno, contraste de color y desplazamiento en el Web Studio, incorpora el logo nativo de la aplicacion en el servidor web, habilita el envio directo a Telegram de todos los formatos (imagenes y videos), activa los controles Cross y Triangle del DualSense en el panel de TV y anade guia de aceleracion para generacion de video.

## Correcciones y Mejoras

- **Diseno, Contraste y Desplazamiento del Web Studio**:
  - Restauracion completa de los estilos de escritorio y capas z-index para el visor Lightbox (`.lb`), eliminando elementos superpuestos y bloqueos graficos.
  - Correccion de discrepancias de color en el tema claro al sustituir fondos oscuros fijos por variables CSS adaptables (`--sel-bg`, `--raised`, `--hover-weak`), garantizando legibilidad y armonia visual.
  - Ajuste de margenes y fijacion adhesiva de la barra de generacion movil (`.gorow`), evitando que tape controles o entorpezca el scroll.
  - Correccion de puntos de ruptura para que dispositivos medianos mantengan la barra horizontal de pestanas sin cortes laterales indeseados.
  - Barras de desplazamiento ampliadas a 8px con alto contraste tanto en modo oscuro como en modo claro.

- **Icono de Aplicacion y Favicon en el Servidor**:
  - Implementacion de la ruta `/ps5sd/v1/app/icon` que entrega `icon0.png` con encabezados PNG y respaldo vectorial SVG.
  - Visualizacion del logotipo oficial en la cabecera de la interfaz web y como favicon en la pestana del navegador.

- **Envio a Telegram para Todo Tipo de Archivos**:
  - El boton de envio a Telegram se integro de forma permanente en la barra de herramientas principal del Lightbox (`.lb-acts`), permitiendo enviar tanto videos como imagenes previas con o sin metadatos.
  - Incorporacion de boton de envio a Telegram en la barra de seleccion multiple de la galeria (`#gselbar`).

- **Controles DualSense en el Panel de TV**:
  - Implementacion del boton `Cruz` (Cross): inicia la generacion inmediata en segundo plano con vibracion haptica en el mando.
  - Implementacion del boton `Triangulo` (Triangle): marca la imagen seleccionada como favorita (`#favorite`) en `.labels.json` con vibracion y notificacion en pantalla.

- **Guia de Optimizacion de Velocidad en Video**:
  - Se anadio un aviso instructivo en la seccion de video indicando que Wan 2.1 tarda ~2 minutos utilizando el LoRA CausVid (4 a 8 pasos, CFG 1), frente a los ~14 minutos que toma a 20 pasos convencionales.

## Instalacion

Instale desde la tienda homebrew (Machete AI Studio, PPSA99699), o descomprima PPSA99699.zip en /data/homebrew/ en la consola y permita que ShadowMountPlus escanee. Requisitos: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv en 2121 opcional.

Creditos: PS5SD por Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice), adiciones de Machete Studio por VenezuelanBiggie24.
