Machete AI Studio 1.2.1 is an urgent hotfix release addressing an unhandled JavaScript startup exception in the Web UI that prevented event listeners from attaching to interactive buttons.

## Fixes & Enhancements

- **Web UI Critical Startup Hotfix**:
  - Restored missing `js/22c-fan.js` and `js/22d-appearance.js` script inclusions in `page.html`, resolving `ReferenceError: initFan is not defined` and `initAppearance` startup exceptions that halted event listener binding.
  - Hardened `start()` lifecycle with resilient `try/catch` boundaries around auxiliary modules, guaranteeing all core button event listeners (`go`, tabs, presets, randomize, prompt clear) attach under any condition.
  - Replaced DOM Level 4 `new Event()` instantiations with backwards-compatible `fireEvent()` helper supporting legacy WebKit and PS5 embedded browsers without ES6 Event constructor support.
  - Added null safety checks for `document.documentElement.className` and `$('sidefold')` in appearance controller.
- **Architecture (Preserved from v1.2.0)**:
  - Generative models: Sana 1.6B linear attention, Sana 0.6B drafts, SD 3.5 Medium (Q8_0) MMDiT, Kokoro 82M audio synthesis.
  - GFX1030 Packed FP16 dual-issue compute shaders (~20.6 TFLOPS arithmetic throughput).
  - DualSense controller polling in TV main loop, dynamic lightbar feedback, and haptics.
  - 3D Director Camera controls for video models.
  - Active hardware fan control (`ps5sd-fan.elf`) with port 9021 loader and port 9075 watchdog.

## Install

Install from the homebrew store (Machete AI Studio, PPSA99699), or unpack PPSA99699.zip into /data/homebrew/ on the console and let ShadowMountPlus scan. Requirements: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv on 2121 optional.

Credits: PS5SD by Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice), Machete Studio additions by VenezuelanBiggie24.

----

Machete AI Studio 1.2.1 es una actualizacion de correccion urgente que soluciona una excepcion de JavaScript no controlada en el inicio de la interfaz web que impedia que los botones interactivos respondieran.

## Correcciones y Mejoras

- **Correccion Critica de Inicio en Web UI**:
  - Se restauraron las inclusiones de scripts faltantes `js/22c-fan.js` y `js/22d-appearance.js` en `page.html`, solucionando los errores `ReferenceError: initFan is not defined` e `initAppearance` que detenian la asignacion de eventos.
  - Se reforzo el ciclo de vida `start()` con bloques `try/catch` de aislamiento, garantizando que todos los botones principales (`go`, pestanas, presets, aleatorio, limpiar prompt) queden activos ante cualquier situacion.
  - Se sustituyeron las llamadas a `new Event()` de DOM Level 4 por una funcion auxiliar `fireEvent()` compatible con WebKit antiguo y navegadores embebidos de PS5.
  - Se anadieron verificaciones de seguridad ante valores nulos en `document.documentElement.className` y `$('sidefold')` en el controlador de apariencia.
- **Arquitectura (Preservada de v1.2.0)**:
  - Modelos generativos: Sana 1.6B con atencion lineal, Sana 0.6B para bocetos rapidos, SD 3.5 Medium (Q8_0) MMDiT, sintesis de voz Kokoro 82M.
  - Shaders de computo Packed FP16 en GFX1030 (~20.6 TFLOPS de rendimiento aritmetico).
  - Sondeo nativo del mando DualSense en el renderizador de TV, barra de luz dinamica y vibracion haptica.
  - Controles 3D de Camara de Director para modelos de video.
  - Control activo de ventilador (`ps5sd-fan.elf`) con cargador en puerto 9021 y vigilancia en puerto 9075.

## Instalacion

Instale desde la tienda homebrew (Machete AI Studio, PPSA99699), o descomprima PPSA99699.zip en /data/homebrew/ en la consola y permita que ShadowMountPlus escanee. Requisitos: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv en 2121 opcional.

Creditos: PS5SD por Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice), agregados de Machete Studio por VenezuelanBiggie24.
