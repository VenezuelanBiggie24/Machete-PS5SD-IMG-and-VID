Machete AI Studio 1.2.2 resolves model catalog download failures (HTTP 401 and HTTP 404) by updating model repository endpoints to verified un-gated public mirrors and introducing runtime URL canonicalization for download retries.

## Fixes & Enhancements

- **Model Download Mirrors & HTTP 401/404 Resolution**:
  - Corrected repository name casing for SD 3.5 Medium Q8_0 (`city96/stable-diffusion-3.5-medium-gguf`), fixing the HTTP 401 Unauthorized error when downloading without Hugging Face credentials.
  - Mirrored SD 3.5 VAE (`PS5SD_SD35_VAE`) to un-gated mirror (`Shio-Koube/SD-3.5-vae`), eliminating gated agreement blocks.
  - Switched Kokoro 82M audio model endpoint to `prince-canuma/Kokoro-82M` (`kokoro-v1_0.safetensors`), resolving the HTTP 404 Not Found error caused by upstream repository restructuring.
  - Restored PhotoMaker v1 safetensors endpoint via `bssrdf/PhotoMaker` (`photomaker-v1.safetensors`), preventing 404 errors during face lock weight downloads.
  - Replaced gated Gemma 2 LLM weights with public un-gated quant (`bartowski/gemma-2-2b-it-GGUF`).
  - Updated Sana 1.6B and 0.6B to official diffusers safetensors endpoints (`Efficient-Large-Model/Sana_1600M_1024px_diffusers` and `Sana_600M_512px_diffusers`).
- **Automatic Runtime URL Canonicalizer**:
  - Implemented `canonicalize_model_url()` in `download_core.cpp` and `downloader.cpp`.
  - Automatically redirects legacy or failed download URLs stored in the queue to verified working endpoints, enabling seamless one-click Retry from the Web UI without needing to wipe downloads or provide API keys.
- **Web UI & Architecture (Preserved)**:
  - Startup lifecycle hardening and WebKit compatibility from v1.2.1.
  - GFX1030 Packed FP16 dual-issue compute shaders (~20.6 TFLOPS arithmetic throughput).
  - DualSense controller TV navigation, Director camera controls, and active hardware fan control.

## Install

Install from the homebrew store (Machete AI Studio, PPSA99699), or unpack PPSA99699.zip into /data/homebrew/ on the console and let ShadowMountPlus scan. Requirements: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv on 2121 optional.

Credits: PS5SD by Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice), Machete Studio additions by VenezuelanBiggie24.

----

Machete AI Studio 1.2.2 resuelve los fallos de descarga de modelos en el catalogo (errores HTTP 401 y HTTP 404) actualizando los repositorios a espejos publicos no restringidos e incorporando canonicalizacion dinamica de URLs para reintentos.

## Correcciones y Mejoras

- **Espejos de Descarga y Resolucion de Errores HTTP 401/404**:
  - Se corrigio el nombre de repositorio con mayusculas de SD 3.5 Medium Q8_0 (`city96/stable-diffusion-3.5-medium-gguf`), eliminando el error HTTP 401 Unauthorized sin requerir token de Hugging Face.
  - Se redirigio el VAE de SD 3.5 (`PS5SD_SD35_VAE`) a un espejo abierto (`Shio-Koube/SD-3.5-vae`), evitando el bloqueo de repositorio restringido.
  - Se actualizo la ruta de Kokoro 82M a `prince-canuma/Kokoro-82M` (`kokoro-v1_0.safetensors`), solucionando el error HTTP 404 Not Found debido a la reorganizacion del repositorio oficial.
  - Se restauro la descarga de PhotoMaker v1 safetensors a traves de `bssrdf/PhotoMaker` (`photomaker-v1.safetensors`), evitando el error 404 al descargar modelos de fijacion facial.
  - Se sustituyeron las dependencias de Gemma 2 LLM por el cuantil publico no restringido (`bartowski/gemma-2-2b-it-GGUF`).
  - Se actualizaron Sana 1.6B y 0.6B a las versiones safetensors de diffusers (`Efficient-Large-Model/Sana_1600M_1024px_diffusers` y `Sana_600M_512px_diffusers`).
- **Canonicalizador Dinamico de URLs**:
  - Se implemento `canonicalize_model_url()` en `download_core.cpp` y `downloader.cpp`.
  - Reasigna automaticamente las URLs fallidas o en cola hacia los espejos funcionales verificados, permitiendo presionar 'Retry' en la interfaz web y descargar con exito de inmediato sin necesidad de limpiar la cola o ingresar tokens.
- **Web UI y Arquitectura (Preservadas)**:
  - Estabilidad y tolerancia a excepciones en el inicio de la interfaz web introducidas en v1.2.1.
  - Shaders de computo Packed FP16 en GFX1030 (~20.6 TFLOPS de rendimiento aritmetico).
  - Control de navegacion TV con mando DualSense, Camara de Director 3D y control activo de ventilador.

## Instalacion

Instale desde la tienda homebrew (Machete AI Studio, PPSA99699), o descomprima PPSA99699.zip en /data/homebrew/ en la consola y permita que ShadowMountPlus escanee. Requisitos: kstuff-lite, ShadowMountPlus, elfldr; ftpsrv en 2121 opcional.

Creditos: PS5SD por Darkatek7 (Heel Tobias), stable-diffusion.cpp (ggerganov / leejet), ps5-exporter (Marice), agregados de Machete Studio por VenezuelanBiggie24.
