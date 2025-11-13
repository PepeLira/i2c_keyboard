# Firmware RP2040 – Teclado Matricial 7×6 + 11 teclas independientes con HID nativo por I²C, HID USB y NeoPixel

Este repositorio contiene el firmware para una placa basada en **RP2040** (Raspberry Pi Pico o equivalente) que expone un **teclado HID estándar** mediante:

* **HID sobre I²C** (*HID over I2C*), para que el teclado sea reconocido por un sistema Linux/PC sin drivers especiales.
* **HID por USB** (TinyUSB) opcional, para pruebas inmediatas conectando el RP2040 directamente a un PC.

La disposición física incluye:

* Una **matriz 7×6** (7 filas × 6 columnas).
* **11 teclas independientes** definidas como `FN1..FN12` (sin `FN7`), conectadas como *pull-down*.
* **5 teclas de cursor** dentro de las FN.
* **Un NeoPixel** (WS2812/APA106) para indicar estados.

Toda la asignación lógica (teclas, modificadores, capas, caracteres especiales) se define en:

./keyboard_layout.json

Este archivo describe la relación entre posiciones físicas (`A1..G6`, `FN1..FN12`) y los **usages HID**, así como los modificadores y su comportamiento.

---

## Tabla de contenidos

* Arquitectura
* Mapa de hardware
* HID sobre I²C
* HID por USB (modo prueba)
* Layouts y keyboard_layout.json
* Sticky Keys (Shift, Alt, Fn, Ctrl)
* Indicadores LED (NeoPixel)
* Construcción
* Estructura del repositorio
* Configuración
* Pruebas
* Rendimiento, *debouncing* y anti-ghosting
* Checklist del proyecto

---

## Arquitectura

* **SoC:** RP2040, doble núcleo.
* **SDK:** Pico SDK (TinyUSB opcional).
* **Interfaces HID:**

  * HID over I2C (principal).
  * HID USB (pruebas).
* **Entrada física:**

  * Matriz 7×6 con escaneo continuo.
  * 11 teclas discretas (pull-down).
* **Limitaciones de hardware:**

  * La matriz **no tiene diodos**, por lo que se implementa **anti-ghosting por software**.
* **Indicador visual:**

  * 1 NeoPixel controlado por PIO.

---

## Mapa de hardware

Los GPIO deben ajustarse al hardware real y reflejarse en `config.h`.

* **I²C:** SDA, SCL
* **Matriz:** ROW_PINS[7], COL_PINS[6]
* **Teclas discretas (11):** DISCRETE_PINS[11]
* **NeoPixel:** NEOPIXEL_GPIO
* **USB:** Pines nativos del RP2040

---

## HID sobre I²C

El firmware implementa **HID over I²C**, exponiéndose como un teclado estándar.

Incluye:

* Descriptor HID para teclado.
* Report descriptor derivado del contenido de keyboard_layout.json.
* Reportes HID con:

  * Modificadores (Shift, Alt, Ctrl, GUI, Fn).
  * Lista de keycodes activos (limitada por anti-ghosting).

El host interpreta los reportes HID sin requerir configuraciones especiales.

### Anti-ghosting por software

Debido a la ausencia de diodos:

* Se limita la cantidad de teclas simultáneas reportadas (ej. 2 o 3).
* Las combinaciones inseguras se descartan.
* Las teclas modificadoras pueden excluirse del límite.

---

## HID por USB (modo prueba)

Si se habilita TinyUSB, la placa se comporta como un teclado USB estándar.

* Útil para pruebas rápidas en un PC.
* El report descriptor USB es el mismo que el de I²C (misma lógica).
* Permite validar layout, capas y modificadores sin un sistema I²C host.

---

## Layouts y keyboard_layout.json

El archivo JSON define completamente el comportamiento lógico del teclado:

* Mapeo matriz: `"A3": ["a", "A", "á"]`, `"A6": ["1", "!", "F1"]`

  * Índice 0 → sin modificador
  * Índice 1 → Shift
  * Índice 2 → Fn (o el modificador correspondiente según se defina)
* Mapeo teclas FN: `"FN1": ["Escape"]`, `"FN2": ["ArrowUp"]`, etc.
* Modificadores: Shift, Ctrl, Alt, Fn, etc.
* Capas: definidas mediante el índice en la lista, igual que en `i2c_puppet`.

Permite cambiar completamente el idioma del teclado o el layout sin tocar el firmware.

---

## Sticky Keys (Shift, Alt, Fn, Ctrl)

Los **modificadores son sticky keys** para mejorar usabilidad en dispositivos portátiles:

### Comportamiento:

* **Una pulsación:**
  El modificador se activa **solo para la siguiente tecla no modificadora**.
  Tras enviar esa tecla, el modificador vuelve a estado normal.

* **Dos pulsaciones seguidas:**
  El modificador entra en **modo sticky permanente**.
  Permanece activo hasta que el usuario lo presione nuevamente.

* **Combinación con layout:**
  Cada modificador corresponde a un **índice** en la lista del JSON.
  Ejemplo: `"A6": ["1", "!", "F1"]`

  * Índice 1 (Shift) → `!`
  * Índice 2 (Fn) → `F1`

### Alt internacional

El modificador **Alt** funciona como en un teclado internacional:

* Alt + a → á
* Alt + e → é
* Alt + i → í
* Alt + o → ó
* Alt + u → ú
* Alt + n → ñ

Estos pares (a→á, n→ñ, etc.) deben definirse explícitamente en keyboard_layout.json.

---

## Indicadores LED (NeoPixel)

El NeoPixel indica estados importantes:

* Sin modificadores → blanco tenue.
* Shift → color dedicado (ej. azul).
* Alt → color especial (ej. amarillo).
* Ctrl → otro color.
* Fn → otro color.
* Sticky permanente → brillo más alto.
* Error o ghosting → rojo parpadeante.
* Tecla presionada → blink corto.

Todo esto puede ajustarse en `neopixel.c`.

---

## Construcción

### Requisitos

* ARM gcc (arm-none-eabi-gcc)
* CMake ≥ 3.13
* Pico SDK
* Git

### Clonar y agregar submódulo Pico SDK

git clone <URL_REPO>.git
cd rp2040-keyboard
git submodule add [https://github.com/raspberrypi/pico-sdk](https://github.com/raspberrypi/pico-sdk) external/pico-sdk
git -C external/pico-sdk submodule update --init --recursive

### Configurar entorno

export PICO_SDK_PATH=$(pwd)/external/pico-sdk

### Compilar

mkdir build
cd build
cmake ..
cmake --build . --target rp2040_keyboard_hid

---

## Estructura del repositorio

.
├── CMakeLists.txt
├── external/
│   └── pico-sdk/
├── keyboard_layout.json
├── include/
│   ├── config.h
│   ├── layout.h
│   ├── hid_reports.h
│   └── neopixel.h
├── src/
│   ├── main.c
│   ├── matrix_scan.c
│   ├── discrete_keys.c
│   ├── hid_i2c.c
│   ├── hid_usb.c
│   ├── keymap.c
│   └── neopixel.c
└── README.md

---

## Configuración

Editar `config.h`:

* Dirección I²C
* SCAN_RATE_HZ
* MAX_SIMULT_KEYS
* Pines I²C
* Pines matriz
* Pines discretos
* NeoPixel GPIO
* Opciones USB

Debe coincidir con la estructura del teclado y con keyboard_layout.json.

---

## Pruebas

### USB HID

1. Habilitar ENABLE_USB_HID
2. Conectar RP2040 al PC
3. Verificar:

   * Escritura correcta
   * Sticky keys
   * Alt internacional
   * Comportamiento de Fn

### HID I²C

* Conectar como HID-over-I2C
* Verificar entradas vía `evtest` o `libinput`
* Probar eventos con combinaciones de modificadores
* Probar sticky keys

### NeoPixel

* Confirmar cambios de color al activar/desactivar modificadores
* Probar blink al presionar teclas
* Simular errores para ver parpadeo rojo

---

## Rendimiento, debounce y anti-ghosting

* Frecuencia recomendada: 1–2 kHz
* Debounce por software: 5–10 ms
* Anti-ghosting: limitar teclas simultáneas y descartar combinaciones conflictivas
* Teclas modificadoras tratadas por separado

---

## Checklist del proyecto

### Planificación

* [ ] Pinout definitivo
* [ ] Definir estructura completa del JSON
* [ ] Diseñar pares Alt internacional

### Infraestructura

* [ ] Pico SDK como submódulo
* [ ] CMakeLists completo
* [ ] Implementar hid_reports.h

### Implementación HID

* [ ] HID I²C
* [ ] HID USB
* [ ] Sticky keys
* [ ] Alt internacional
* [ ] Anti-ghosting
* [ ] Keymap a partir de JSON

### Indicadores

* [ ] NeoPixel con colores por modificador
* [ ] Estado sticky
* [ ] Errores

### Pruebas

* [ ] USB HID
* [ ] I²C HID
* [ ] Sticky keys
* [ ] Alt internacional
* [ ] Ghosting controlado

### Documentación

* [ ] README con pinout real y ejemplos
* [ ] Documentar schema JSON
* [ ] Guía de troubleshooting

### Entrega

* [ ] Release con binarios .uf2
* [ ] Versionado v1.0.0
* [ ] CI opcional

---

### Consejo
Mantén separada de la implementación la layout del teclado. Para luego poder realizar modificaciones de ser necesario. 
De ser necesario para leer los estados de las teclas es posible que sea necesario un arreglo FIFO. Separa y describe la arquitectura de la aplicación para poder ayudar al proceso de desarrollo. 
