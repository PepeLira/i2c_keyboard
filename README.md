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

### Arquitectura de software

El firmware está dividido en módulos auto-contenidos que facilitan su mantenimiento y pruebas:

* `matrix_scan.c`: escaneo y *debounce* de la matriz 7×6 mediante *callbacks*.
* `discrete_keys.c`: gestión de las 11 teclas con lógica de anti-rebote y mapeo a índices lógicos.
* `fifo.c`: FIFO circular de 64 entradas para eventos normalizados.
* `i2c_slave.c`: controlador I²C en modo esclavo que expone el banco de registros, despacha la FIFO y gobierna la línea INT opcional.
* `neopixel.c` + `pio/neopixel.pio`: controlador PIO dedicado al WS2812/APA106.
* `layout_default.c`: mapeos físico→lógico y paleta de colores por defecto.
* `main.c`: ciclo principal (1 kHz) que integra escáneres, actualiza modificadores, cursor, LED y publica eventos en la FIFO.

Cada módulo expone un encabezado en `include/` para mantener bajo acoplamiento. `main.c` es el único responsable de combinar eventos físicos con el layout lógico y actualizar el `register_bank_t` consumido por `i2c_slave.c`. Los modificadores se representan en una máscara HID de 8 bits, y el LED cambia automáticamente entre los colores por defecto, modificador activo y error.

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
| `0x00`    | `DEV_ID`        | Identificador (ej. `0xB0`)                                        |
| `0x01`    | `FW_VERSION`    | Mayor.menor (ej. `0x01 0x00`)                                     |
| `0x02`    | `STATUS`        | Bits: `FIFO_EMPTY`, `FIFO_FULL`, `MOD_MASK_VALID`, `ERR`          |
| `0x03`    | `MOD_MASK`      | Máscara de modificadores activos (Ctrl/Shift/Alt/Meta/Func, etc.) |
| `0x04`    | `FIFO_COUNT`    | Cantidad de eventos en FIFO                                       |
| `0x05`    | `FIFO_POP` (R)  | Leer = extrae un evento (hasta 8 bytes)                           |
| `0x06`    | `CFG_FLAGS`     | Flags de configuración (debounce, diodos, modo INT, etc.)         |
| `0x07`    | `CURSOR_STATE`  | Estado de las 5 teclas de cursor (bits UP/DOWN/LEFT/RIGHT/CENTER) |
| `0x08`    | `LED_STATE` (W) | Escribir color directo (GRB) o *preset*                           |
| `0x09`    | `SCAN_RATE`     | Tasa de escaneo en Hz (ej. 1–2 kHz)                               |

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

> **Liberación de cursor:** al soltar una tecla de cursor el código del evento se entrega como `CURSOR_x | 0x80` para distinguirlo de la pulsación. El registro `CURSOR_STATE` mantiene los bits activos en todo momento.

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

El firmware mantiene automáticamente el estado del NeoPixel según la condición del teclado:

* **Verde tenue (`LED_COLOR_DEFAULT`)** → funcionamiento normal sin modificadores activos.
* **Rojo (`LED_COLOR_ACTIVE`)** → al menos un modificador activo (Ctrl, Shift, Alt, GUI).
* **Amarillo (`LED_COLOR_ERROR`)** → condición de error (actualmente desbordes de FIFO).

El host puede sobrescribir el color escribiendo 3 bytes GRB consecutivos en el registro `LED_STATE` (`0x08`).

---

## Construcción

1. **Requisitos:** CMake ≥ 3.13, toolchain ARM (`arm-none-eabi-gcc`), `ninja` o `make`, Git.
2. **Pico SDK:** el repositorio espera el SDK en `3rd_party/pico-sdk`. Inicialícelo con:

   ```bash
   git submodule update --init --recursive
   ```

   Si no hay acceso a Internet, descargue manualmente el SDK y colóquelo en esa ruta.

3. **Configurar y compilar:**

   ```bash
   mkdir -p build && cd build
   cmake -G Ninja -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=Release ..
   cmake --build .
   ```

4. **Flasheo:** copie `i2c_keyboard.uf2` al RP2040 montado en modo **BOOTSEL**.

---

## Estructura del repositorio

```
.
├── CMakeLists.txt
├── 3rd_party/
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

## Herramientas de host

El directorio `test/host_tools/` contiene `i2c_dump.py`, un script en Python que emplea `smbus2` para leer los registros expuestos y vaciar la FIFO de eventos desde un PC Linux. Es útil para automatizar pruebas de integración y depuración de layouts.

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
