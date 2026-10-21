Machete PS5SD 0.5.1 fixes the video models, which did not start in 0.5.0.

## Fixed

- **Video models start again**: LTX-Video 2B, Wan 2.2 TI2V 5B and Wan 2.1 T2V 1.3B failed to load with `unknown argument: 0.0.0.0`, and the app went back to the previous model. The flash attention option in their presets was missing its value. If you installed a video model with 0.5.0, delete its preset on the Models tab and add the model again from the catalog, or open its preset file in `/data/PS5SD/presets/` and change the line `--flash-attn` to two lines: `--flash-attn` and `on`.

## Install

Install from the homebrew store, or unpack `PPSA99699.zip` into `/data/homebrew/` on the console and let ShadowMountPlus scan. Requirements: etaHEN + kstuff-lite, ShadowMountPlus, elfldr; ftpsrv on 2121 optional.

Credits: [PS5SD](https://github.com/Darkatek7/PS5SD) by Darkatek7 (Heel Tobias), [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp).

----

Machete PS5SD 0.5.1 corrige los modelos de video, que no arrancaban en 0.5.0.

## Corregido

- **Los modelos de video arrancan de nuevo**: LTX-Video 2B, Wan 2.2 TI2V 5B y Wan 2.1 T2V 1.3B no cargaban y daban `unknown argument: 0.0.0.0`, y la app volvía al modelo anterior. A la opción de flash attention de sus presets le faltaba el valor. Si instalaste un modelo de video con 0.5.0, borra su preset en la pestaña Models y agrega el modelo otra vez desde el catálogo, o abre su archivo en `/data/PS5SD/presets/` y cambia la línea `--flash-attn` por dos líneas: `--flash-attn` y `on`.

## Instalación

Instala desde la tienda homebrew, o descomprime `PPSA99699.zip` en `/data/homebrew/` en la consola y deja que ShadowMountPlus lo detecte. Requisitos: etaHEN + kstuff-lite, ShadowMountPlus, elfldr; ftpsrv en 2121 opcional.

Créditos: [PS5SD](https://github.com/Darkatek7/PS5SD) de Darkatek7 (Heel Tobias), [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp).
