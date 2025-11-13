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

* **Herramienta de host** (`test/host_tools`): lectura de registros y *polling* de FIFO.
* **INT line** (si se usa): confirme que al presionar una tecla el host recibe la interrupción y puede leer `FIFO_POP`.
* **Anti-rebote:** validar que no se generan eventos duplicados.
* **Cursor:** verificar codificación `UP/DOWN/LEFT/RIGHT/CENTER`.

---

## Herramientas de host

El directorio `test/host_tools/` contiene `i2c_dump.py`, un script en Python que emplea `smbus2` para leer los registros expuestos y vaciar la FIFO de eventos desde un PC Linux. Es útil para automatizar pruebas de integración y depuración de layouts.

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
