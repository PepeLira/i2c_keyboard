# Firmware RP2040 – Teclado Matricial 7×6 + 11 teclas independientes con I²C FIFO y NeoPixel

Este repositorio contiene el firmware para una placa basada en **RP2040** (Raspberry Pi Pico o equivalente) que expone por **I²C** un teclado compuesto por una **matriz 7×6** y **11 teclas independientes** (cableadas a GPIO como *pull-down*). Cinco de esas teclas independientes controlan el movimiento de un **cursor**. Además, el sistema controla un **LED RGB tipo NeoPixel (WS2812/APA106)** para indicar estados y modificadores.

El diseño se inspira en el proyecto **i2c_puppet** de Solder Party (`git@github.com:solderparty/i2c_puppet.git`), pero introduce:

* Tamaño de matriz distinto (7×6).
* 11 teclas discretas adicionales con *pull-down* (5 reservadas para cursor).
* Protocolo FIFO propio con **layout configurable** y señalización mediante **NeoPixel** de modificadores/estados.

---

## Tabla de contenidos

* [Arquitectura](#arquitectura)
* [Mapa de hardware](#mapa-de-hardware)
* [Protocolo I²C y FIFO](#protocolo-i²c-y-fifo)
* [Layouts y keycodes](#layouts-y-keycodes)
* [Indicadores LED (NeoPixel)](#indicadores-led-neopixel)
* [Construcción](#construcción)
* [Estructura del repositorio](#estructura-del-repositorio)
* [Configuración](#configuración)
* [Pruebas](#pruebas)
* [Rendimiento y *debouncing*](#rendimiento-y-debouncing)
* [Checklist del proyecto](#checklist-del-proyecto)

---

## Arquitectura

* **SoC:** RP2040, doble núcleo.
* **SDK:** Pico SDK (TinyUSB deshabilitable; este firmware no requiere USB para operación, solo para *logging* si se desea).
* **Interfaz host:** I²C esclavo con FIFO de eventos (key down/up, modificadores, especiales).
* **Escaneo:** controlador de matriz con prevención de *ghosting* por configuración (opcional diodos; ver sección de *debouncing*).
* **Teclas discretas:** 11 líneas dedicadas (GPIO con *pull-down*), de las cuales 5 están mapeadas a `CURSOR_UP/DOWN/LEFT/RIGHT/CENTER`.
* **Indicadores:** 1 LED NeoPixel direccionado vía PIO o bit-bang (PIO recomendado).

---

## Mapa de hardware

> Ajuste los GPIO a su diseño. Los nombres son ejemplos por defecto.

| Función                         | GPIO (ejemplo) | Notas                                                   |
| ------------------------------- | -------------- | ------------------------------------------------------- |
| I²C SDA                         | GP0            | Pull-up externo 4.7–10 kΩ                               |
| I²C SCL                         | GP1            | Pull-up externo 4.7–10 kΩ                               |
| Filas matriz (7)                | GP2–GP8        | Configuradas como salidas (o entradas según escaneo)    |
| Columnas matriz (6)             | GP9–GP14       | Configuradas como entradas con *pull-down*              |
| Teclas independientes (11)      | GP15–GP25      | Entradas con *pull-down*                                |
| NeoPixel DIN                    | GP26           | Un solo LED (WS2812/APA106)                             |
| INT (opcional)                  | GP27           | Línea de interrupción al host al colocar evento en FIFO |
| Address pins (opcional, 2 bits) | GP28, GP29     | Selección de dirección I²C                              |

> **Nota:** Configure `CMakeLists.txt`/`config.h` para reflejar su *pinout* definitivo.

---

## Protocolo I²C y FIFO

El dispositivo se presenta como un esclavo I²C con una página de registros simples y una **FIFO de eventos** de tamaño fijo.

### Dirección I²C

* Base: `0x32` (ejemplo).
* Bits A0–A1 opcionales desde pines `ADDR0/ADDR1` → `0x32 | (ADDR1<<1) | ADDR0`.

### Registros

| Dirección | Nombre          | Descripción                                                       |
| --------- | --------------- | ----------------------------------------------------------------- |
| `0x00`    | `DEV_ID`            | Identificador (ej. `0xB0`)                                        |
| `0x01`    | `FW_VERSION_MAJOR` | Byte mayor de versión (ej. `0x01`)                               |
| `0x02`    | `FW_VERSION_MINOR` | Byte menor de versión (ej. `0x00`)                               |
| `0x03`    | `STATUS`            | Bits: `FIFO_EMPTY`, `FIFO_FULL`, `MOD_MASK_VALID`, `ERR`          |
| `0x04`    | `MOD_MASK`          | Máscara de modificadores activos (Ctrl/Shift/Alt/Meta/Fn)        |
| `0x05`    | `FIFO_COUNT`        | Cantidad de eventos en FIFO                                       |
| `0x06`    | `FIFO_POP` (R)      | Leer = extrae evento (4 bytes rotativos)                          |
| `0x07`    | `CFG_FLAGS`         | Flags de configuración (debounce, diodos, modo INT, etc.)         |
| `0x08`    | `CURSOR_STATE`      | Estado de las 5 teclas de cursor (bits UP/DOWN/LEFT/RIGHT/CENTER) |
| `0x09-0x0B` | `LED_STATE` (W)   | Escribir color directo en formato GRB                             |
| `0x0C`    | `SCAN_RATE`         | Tasa de escaneo en Hz (ej. 1–2 kHz)                               |

> La lectura de `FIFO_POP` devuelve un **paquete de evento**. Si la FIFO está vacía, devuelve un evento `NOP` o `0x00`.

### Formato de evento (FIFO)

Tamaño fijo de 4 bytes por evento (recomendado por simplicidad):

```
Byte 0: TYPE      (0x01=KEY_DOWN, 0x02=KEY_UP, 0x03=MOD_CHANGE, 0x04=CURSOR, 0x05=SPECIAL, 0x00=NOP)
Byte 1: CODE      (keycode lógico o cursor code)
Byte 2: MOD_MASK  (estado de modificadores tras el evento)
Byte 3: TS_LO     (timestamp relativo en ticks; opcionalmente combine con un registro TS_HI)
```

* **KEY_DOWN/KEY_UP:** `CODE` es keycode lógico (layout-aware).
* **MOD_CHANGE:** `CODE` = máscara de bits que cambió.
* **CURSOR:** `CODE` en {UP=1, DOWN=2, LEFT=3, RIGHT=4, CENTER=5}.
* **SPECIAL:** reservado (p. ej., *layer toggle*, *macro trigger*).

> **INT opcional:** al *pushear* un evento válido en FIFO se pulsa `INT` (activo-bajo), el host limpia leyendo `FIFO_POP`.

### Tamaño de FIFO

* Por defecto **64 eventos** (configurable por `#define FIFO_SIZE`).

---

## Layouts y keycodes

* **Layout separable de hardware.** Un archivo `layout_<name>.h` define el mapeo:

  * Matriz 7×6 → keycodes lógicos (p. ej., valores tipo HID o códigos propios).
  * 11 teclas discretas → keycodes lógicos.
  * Máscaras de modificadores (Ctrl, Shift, Alt, Meta, Fn).
* **Capas (layers):** opcional. Un modificador o tecla especial puede alternar capa para remapear.

Ejemplo (esquema conceptual):

```c
// layout_default.h (extracto)
#define MOD_CTRL   (1u<<0)
#define MOD_SHIFT  (1u<<1)
#define MOD_ALT    (1u<<2)
#define MOD_META   (1u<<3)
#define MOD_FN     (1u<<4)

static const uint16_t matrix_map[7][6] = {
  // keycodes lógicos (propios o HID usage IDs)
  { KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y },
  { KC_A, KC_S, KC_D, KC_F, KC_G, KC_H },
  // ...
};

static const uint16_t discrete_map[11] = {
  KC_UP, KC_DOWN, KC_LEFT, KC_RIGHT, KC_ENTER, /* … otras 6 … */
};
```

---

## Indicadores LED (NeoPixel)

* **Estados por defecto:**

  * **Sin modificadores:** NeoPixel tenue blanco.
  * **Shift/Ctrl/Alt/Meta/Fn:** color por bit activo (combinaciones → mezcla).
  * **Error (FIFO FULL):** rojo intermitente.
  * **Actividad (key event):** *blink* corto.

* **Control:** escribir en `LED_STATE`:

  * `0x00 0xGG 0xRR 0xBB` → color directo (GRB típico).
  * `0x01 idx` → preset predefinido.

---

## Construcción

### 1) Requisitos

* **Toolchain ARM** (`arm-none-eabi-gcc`), **CMake** ≥ 3.13, **Git**.
* **Pico SDK** como submódulo.

### 2) Clonar y agregar Pico SDK

```bash
# Clonar este repo
git clone <URL_DE_ESTE_REPO>.git rp2040-keyboard-i2c
cd rp2040-keyboard-i2c

# Agregar Pico SDK como submódulo (rama estable recomendada)
git submodule add https://github.com/raspberrypi/pico-sdk external/pico-sdk
git -C external/pico-sdk submodule update --init  # trae tinyusb, etc.
```

> Si el repo ya existía: `git submodule update --init --recursive`.

### 3) Configurar entorno

Definir `PICO_SDK_PATH` (si no se usa ruta relativa en `CMakeLists.txt`):

```bash
export PICO_SDK_PATH=$(pwd)/external/pico-sdk
```

### 4) Compilar

```bash
mkdir -p build && cd build
cmake -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=Release ..
make -j
```

El artefacto `.uf2` quedará en `build/`. Para cargarlo, monte el dispositivo en modo **BOOTSEL** y copie el `.uf2`.

---

## Estructura del repositorio

```
.
├── CMakeLists.txt
├── external/
│   └── pico-sdk/          # submódulo
├── include/
│   ├── config.h           # pines, dirección I²C, FIFO_SIZE, flags
│   ├── layout_default.h   # mapeo de teclas
│   └── protocol.h         # registros, tipos de evento, máscaras
├── src/
│   ├── main.c             # init, loop de escaneo, I²C slave, FIFO
│   ├── matrix_scan.c      # driver de matriz 7×6
│   ├── discrete_keys.c    # 11 teclas pull-down
│   ├── fifo.c             # implementación FIFO
│   ├── i2c_slave.c        # registro y transacciones
│   └── neopixel.c         # control de LED (PIO)
├── test/
│   └── host_tools/        # script(s) para leer FIFO por I²C
└── README.md
```

---

## Configuración

Edite `include/config.h`:

```c
#define I2C_ADDR_BASE   0x32
#define FIFO_SIZE       64
#define SCAN_RATE_HZ    1000
#define USE_INT_PIN     1
#define INT_GPIO        27

// Pines I2C
#define SDA_GPIO        0
#define SCL_GPIO        1

// Matriz 7×6 (ejemplo)
static const uint8_t ROW_PINS[7] = {2,3,4,5,6,7,8};
static const uint8_t COL_PINS[6] = {9,10,11,12,13,14};

// Teclas discretas (11)
static const uint8_t DISCRETE_PINS[11] = {15,16,17,18,19,20,21,22,23,24,25};

// NeoPixel
#define NEOPIXEL_GPIO   26
#define NEOPIXEL_COUNT  1
```

---

## Pruebas

* **Herramienta de host** (`test/host_tools`): lectura de registros y *polling* de FIFO.
* **INT line** (si se usa): confirme que al presionar una tecla el host recibe la interrupción y puede leer `FIFO_POP`.
* **Anti-rebote:** validar que no se generan eventos duplicados.
* **Cursor:** verificar codificación `UP/DOWN/LEFT/RIGHT/CENTER`.

---

## Rendimiento y *debouncing*

* **Tasa de escaneo** objetivo: 1–2 kHz para baja latencia.
* **Debounce** por software: 5–10 ms por tecla es usual; ajustable con `CFG_FLAGS`.
* **Ghosting:** si el hardware carece de diodos en la matriz, se sugiere habilitar mitigaciones por software (no n-key rollover completo).

---

## Checklist del proyecto

**Planificación y fundamentos**

* [ ] Cerrar *pinout* definitivo (I²C, filas/columnas, 11 discretas, NeoPixel, INT, address pins).(de momento uno genérico)
* [ ] Especificar keycodes y capas (layout por defecto + variantes).

**Infraestructura**

* [ ] Inicializar repo y agregar **Pico SDK** como submódulo.
* [ ] Escribir `CMakeLists.txt` (target, PIO, linker, opciones).
* [ ] Configurar `include/config.h` y `protocol.h`.

**Implementación**

* [ ] Driver de matriz 7×6 (escaneo, debounce, ghosting guard).
* [ ] Lector de 11 teclas discretas (debounce).
* [ ] FIFO circular (push/pop, *overflow handling*).
* [ ] Esclavo I²C (registro de página, `FIFO_POP`, `STATUS`, `MOD_MASK`, etc.).
* [ ] Línea **INT** al host (opcional).
* [ ] Control **NeoPixel** con PIO y presets de estado.
* [ ] Manejador de **modificadores** y **cursor** (5 teclas dedicadas).

**Configuración y layouts**

* [ ] `layout_default.h` con mapeo completo de matriz + discretas.
* [ ] Soporte de capas / función (Fn) si aplica.
* [ ] Mapeo de colores de NeoPixel por modificador/estado.

**Pruebas y validación**

* [ ] Scripts de host (I²C) para leer/escribir registros y vaciar FIFO.
* [ ] Pruebas de latencia y tasa de escaneo (ajuste fino).
* [ ] Ensayo de *FIFO FULL* y recuperación.
* [ ] Validar comportamiento de INT y *polling*.

**Documentación**

* [ ] Completar README con pinout real, diagramas y ejemplos de host.
* [ ] Añadir guía de solución de problemas (pull-ups I²C, polaridad, etc.).
* [ ] Publicar ejemplos de layouts alternativos.

**Entrega**

* [ ] *Release* inicial con binarios `.uf2`.
* [ ] Etiquetar versión `v1.0.0` y changelog.
* [ ] (Opcional) CI para *builds* de prueba.

---

> **Sugerencia:** mantenga separado el **layout** del **hardware**. Así podrá iterar sobre el mapeo sin tocar el código de escaneo ni el protocolo I²C. También considere documentar en el repo del host un *driver* mínimo que consuma la FIFO y traduzca los keycodes a su sistema destino.
