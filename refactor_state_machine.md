# Refactor Guide – FSM + Event-Driven Architecture (RP2040 Power + NeoPixel)

This document describes **how to refactor the current firmware** into:

1. A clear **Finite State Machine (FSM)** for power and latch control.
2. An **event-driven (Observer)** architecture where modules publish and subscribe to events instead of directly calling each other.

La idea es dejar el código **fácil de leer**, con **archivos bien separados por responsabilidad**, y preparado para crecer (teclado I²C, FIFO, etc.) sin transformar `main.c` en un monstruo.

---

## 1. Objetivos de la refactorización

* Modelar la lógica del botón de encendido y el soft latch como una **máquina de estados explícita**.
* Dejar de “cablear” el `power_button.c` con el `neopixel.c` mediante llamadas directas, y en cambio:

  * Generar **eventos** (`POWER_BUTTON_PRESSED`, `POWER_HOLD_1S`, `POWER_HOLD_3S`, `LATCH_OPENED`, etc.).
  * Tener **observadores** que reaccionen a esos eventos (NeoPixel, futuro módulo de Luckfox, teclado, etc.).
* Encapsular:

  * **Drivers de bajo nivel** (GPIO, PIO, I²C) en módulos de HAL.
  * **Lógica de alto nivel** (FSM de energía, política de apagado/encendido) en módulos de “servicio”.

---

## 2. Comportamiento funcional a modelar

### 2.1 Lógica del botón + soft latch

Requisitos funcionales que **la FSM debe reflejar**:

1. **Con USB conectado**

   * Se enciende solo el **RP2040** (para mostrar estado de carga).
   * La **Luckfox** se enciende solo si el botón se presiona **por más de 1 segundo**.

2. **Presión de 3 segundos**

   * A los **3 segundos** de mantener el botón presionado:

     * Se **cierra** el latch y la Luckfox se apaga.
     * Si además no hay USB, también se apaga el RP2040 (se corta la alimentación).

3. **LED RGB**

   * Estado *default*: sólido **verde**.
   * Mientras el botón está siendo presionado:

     * El LED parpadea **rojo / apagado** cada segundo de presión (ritmo 1 Hz).
   * Reservar otros colores para cuando se presionen modificadores del teclado (futuro).

### 2.2 `latch_status` (subestado de alimentación)

Tres estados relacionados al soft latch:

* **Abierto**

  * Antes de que pase 1 segundo de botón presionado.
  * O después de que pasen los 3 segundos para apagar (latch abierto → corte de energía).
* **Cerrado**

  * Luego de que el botón esté presionado 1 segundo (es decir, cuando “engancha” para encender la Luckfox).
* **En transición**

  * Mientras el botón está siendo presionado **después** del primer segundo y **antes** de llegar a los 3 segundos.
  * Solo aplica a la pulsación larga que lleva al apagado.

Relación con el **GPIO del botón** (ejemplo: GPIO 29):

* **Cerrado**: `gpio 29` como **input pull-up**.
* **Abierto**: `gpio 29` como **input con pull null / reset**.
* **En transición**: `gpio 29` como **input pull-up**.

> Nota: esto es un ejemplo de cómo el diseño actual piensa el latch. En la FSM vamos a representar este `latch_status` como un enum y concentrar su manejo en un módulo dedicado.

---

## 3. Diseño de la Arquitectura FSM + Event-Driven

### 3.1 Nuevo layout de archivos (propuesta)

```txt
include/
  config.h
  event.h
  fsm.h
  power_fsm.h
  latch.h
  button.h
  neopixel.h

src/
  main.c

  event.c        // Event bus / dispatcher (Observer)
  fsm.c          // Helpers genéricos de FSM (opcional)

  button.c       // Driver GPIO del botón → genera eventos de input
  latch.c        // Manejo del latch físico (GPIO mode según latch_status)

  power_fsm.c    // Lógica de alto nivel de energía (estados ON/OFF, tiempos)
  neopixel.c     // Observer del estado de energía / botón
```

Más adelante puedes sumar:

```txt
  keyboard_fsm.c
  i2c_service.c
  fifo_service.c
```

### 3.2 Máquina de estados de energía

Define una FSM de **alto nivel** que modele “qué está encendido” y “qué se está intentando hacer”:

```c
typedef enum {
    POWER_MODE_OFF_USB_ONLY,   // RP2040 encendido solo por USB, Luckfox apagada
    POWER_MODE_RP_ONLY,        // RP2040 encendido, Luckfox apagada (sin USB)
    POWER_MODE_ALL_ON,         // RP2040 + Luckfox encendidos
    POWER_MODE_SHUTDOWN_ARMED, // Se ha detectado intención de apagado (>= 3s)
} power_mode_t;

typedef enum {
    LATCH_STATUS_OPEN,
    LATCH_STATUS_CLOSED,
    LATCH_STATUS_TRANSITION,
} latch_status_t;
```

Reglas típicas:

* **Al boot**:

  * Detectar si hay USB:

    * Si hay USB: `POWER_MODE_OFF_USB_ONLY`, `LATCH_STATUS_OPEN`.
    * Si no hay USB: probablemente empezar en `POWER_MODE_RP_ONLY` o un estado similar según hardware.
* **Botón presionado ≥ 1s**:

  * Cerrar latch: `LATCH_STATUS_CLOSED`.
  * Si había USB y estabas mostrando solo RP2040, ahora puedes ir a `POWER_MODE_ALL_ON` (enciendes la Luckfox).
* **Botón presionado ≥ 3s**:

  * `POWER_MODE_SHUTDOWN_ARMED` → abrir latch: `LATCH_STATUS_OPEN`.
  * Si no hay USB, eventualmente todo se queda sin alimentación (RP2040 se caerá).
* **Botón liberado**:

  * En función del modo, vuelves a `POWER_MODE_OFF_USB_ONLY` o `POWER_MODE_RP_ONLY`.

### 3.3 Modelo de eventos (Observer)

Define un **bus de eventos** simple y tipos de eventos:

```c
typedef enum {
    EVENT_TICK_1MS,
    EVENT_TICK_10MS,
    EVENT_BUTTON_PRESSED,
    EVENT_BUTTON_RELEASED,
    EVENT_BUTTON_HOLD_1S,
    EVENT_BUTTON_HOLD_3S,
    EVENT_USB_PLUGGED,
    EVENT_USB_UNPLUGGED,
    EVENT_LATCH_CHANGED,
    EVENT_POWER_MODE_CHANGED,
} event_type_t;

typedef struct {
    event_type_t type;
    uint32_t timestamp_ms;
    union {
        struct {
            bool pressed;
        } button;
        struct {
            latch_status_t status;
        } latch;
        struct {
            power_mode_t old_mode;
            power_mode_t new_mode;
        } power_mode;
    } data;
} event_t;
```

API del **event bus**:

```c
typedef void (*event_handler_t)(const event_t *ev);

void event_init(void);
void event_subscribe(event_handler_t handler);
void event_publish(const event_t *ev);
void event_process_pending(void);
```

* `button.c` **publica** eventos de `EVENT_BUTTON_PRESSED`, `EVENT_BUTTON_HOLD_1S`, etc.
* `power_fsm.c` se **suscribe** y:

  * Actualiza su estado interno.
  * Cuando cambian `power_mode` o `latch_status`, publica `EVENT_POWER_MODE_CHANGED` y `EVENT_LATCH_CHANGED`.
* `neopixel.c` se **suscribe** y ajusta el color según:

  * Estado del botón.
  * `POWER_MODE_*`.
  * Eventos de modificadores del teclado (más adelante).

---

## 4. Rol de cada módulo después de la refactorización

### 4.1 `button.c` – Driver del botón

Responsabilidad: leer el GPIO, aplicar **debounce**, contar tiempo de pulsación y emitir eventos de nivel alto.

```c
// button.h
void button_init(void);
void button_on_tick_1ms(void); // llamado por main para generar lógica interna
```

Internamente:

* Usa `POWER_BUTTON` de `config.h`.
* Mantiene un contador de milisegundos mientras el botón está presionado.
* Cuando detecta:

  * Flanco de bajada: `EVENT_BUTTON_PRESSED`.
  * Flanco de subida: `EVENT_BUTTON_RELEASED`.
  * Tiempo >= 1000 ms: `EVENT_BUTTON_HOLD_1S`.
  * Tiempo >= 3000 ms: `EVENT_BUTTON_HOLD_3S`.

Todos esos se generan vía `event_publish()`.

### 4.2 `latch.c` – Control del soft latch

Responsabilidad: **mapear `latch_status_t` → configuración de GPIO** y exponer una API simple:

```c
// latch.h
void latch_init(void);
void latch_set_status(latch_status_t status);
latch_status_t latch_get_status(void);
```

Implementación:

* `latch_set_status(LATCH_STATUS_OPEN)`:

  * `gpio_set_input(POWER_BUTTON);`
  * `gpio_disable_pulls(POWER_BUTTON);`
* `latch_set_status(LATCH_STATUS_CLOSED)`:

  * `gpio_set_input(POWER_BUTTON);`
  * `gpio_pull_up(POWER_BUTTON);`
* `latch_set_status(LATCH_STATUS_TRANSITION)`:

  * Igual que `CLOSED` pero conceptualmente se usa para FSM de apagado.

Cuando `latch_set_status` cambia el estado, publica evento:

```c
event_t ev = {
    .type = EVENT_LATCH_CHANGED,
    .timestamp_ms = system_time_ms(),
    .data.latch.status = status,
};
event_publish(&ev);
```

### 4.3 `power_fsm.c` – FSM de energía

Responsabilidad: **política de encendido/apagado**, en base a eventos de botón y USB.

```c
// power_fsm.h
void power_fsm_init(void);
void power_fsm_handle_event(const event_t *ev);
power_mode_t power_fsm_get_mode(void);
latch_status_t power_fsm_get_latch_status(void);
```

Comportamiento típico:

* `EVENT_BUTTON_HOLD_1S`:

  * `latch_set_status(LATCH_STATUS_CLOSED);`
  * Si estabas en `POWER_MODE_OFF_USB_ONLY`, pasas a `POWER_MODE_ALL_ON`.
* `EVENT_BUTTON_HOLD_3S`:

  * `latch_set_status(LATCH_STATUS_TRANSITION);`
  * Cambias a `POWER_MODE_SHUTDOWN_ARMED`.
* `EVENT_BUTTON_RELEASED`:

  * Si estabas en `POWER_MODE_SHUTDOWN_ARMED`:

    * `latch_set_status(LATCH_STATUS_OPEN);`
    * Según haya USB o no, terminas en `POWER_MODE_OFF_USB_ONLY` o RP apagado.

Cada vez que se modifica `power_mode`:

```c
event_t ev = {
    .type = EVENT_POWER_MODE_CHANGED,
    .timestamp_ms = system_time_ms(),
    .data.power_mode.old_mode = old_mode,
    .data.power_mode.new_mode = new_mode,
};
event_publish(&ev);
```

---

### 4.4 `neopixel.c` – Observador gráfico

Responsabilidad: **escuchar eventos** y cambiar colores/parpadeos del LED.

* Se suscribe al event bus en `neopixel_init()`.
* Mantiene estado local:

```c
typedef struct {
    bool button_pressed;
    uint32_t hold_ms;
    power_mode_t power_mode;
} neopixel_state_t;
```

* Reglas de color:

  * `POWER_MODE_ALL_ON`, sin botón presionado → `DEFAULT_COLOR` (verde sólido).
  * Botón presionado, `hold_ms` < 1000 ms → verde sólido o leve indicación.
  * Entre 1 y 3 segundos → parpadeo rojo/apagado (usa `EVENT_TICK_100MS` o `EVENT_TICK_10MS` para animación).
  * `POWER_MODE_SHUTDOWN_ARMED` → rojo fuerte o efecto especial.
  * Más adelante: si hay un evento tipo `EVENT_MODIFIER_ACTIVE`, usar `MODIFIER_COLOR`.

---

## 5. Integración en `main.c`

Tu `main.c` pasa de “llamar funciones acopladas” a ser un **scheduler sencillo**:

```c
int main(void) {
    stdio_init_all();
    config_board_pins();

    event_init();

    latch_init();
    neopixel_init();
    button_init();
    power_fsm_init();

    // Suscripciones
    event_subscribe(power_fsm_handle_event);
    event_subscribe(neopixel_handle_event); // a implementar

    absolute_time_t next_tick = delayed_by_ms(get_absolute_time(), 1);

    while (true) {
        // Tick de 1 ms
        if (absolute_time_diff_us(get_absolute_time(), next_tick) <= 0) {
            next_tick = delayed_by_ms(next_tick, 1);

            // Lógica de entrada: el botón actualiza su estado y publica eventos
            button_on_tick_1ms();

            // Generar un evento de TICK (opcional, si lo usas para animación)
            event_t ev = {
                .type = EVENT_TICK_1MS,
                .timestamp_ms = system_time_ms(),
            };
            event_publish(&ev);
        }

        event_process_pending();
        tight_loop_contents();
    }
}
```

`main.c` ya no “sabe” cómo se maneja el latch ni el LED; solo orquesta el tiempo y deja que las FSM + observers hagan su trabajo.

---

## 6. Plan paso a paso para refactorizar

### Paso 1 – Separar drivers de lógica

1. Mueve toda lógica de NeoPixel que sea de “animación/interpretación de estados” fuera del módulo actual, dejando:

   * `neopixel_init()`
   * `neopixel_set_rgb(uint8_t r, uint8_t g, uint8_t b)`
   * `neopixel_off()`
2. Crea `button.c` con:

   * Lectura del GPIO del botón.
   * Debounce básico, pero aún sin eventos (devuelve solo un `bool button_is_pressed(void)` si quieres mantener compatibilidad momentánea).
3. Crea `latch.c` y traslada ahí toda manipulación de `gpio 29` relacionada al soft latch.

### Paso 2 – Introducir el event bus

1. Implementa `event.c` y `event.h` con:

   * Una pequeña cola circular de eventos (p.ej. 16 o 32 entradas).
   * Registro de manejadores (`event_subscribe`).
   * `event_publish` y `event_process_pending`.
2. Sin cambiar la lógica aún, modifica módulos para usar `event_publish` cuando algo relevante ocurra (aunque inicialmente solo se suscriba `neopixel` o `power_fsm`).

### Paso 3 – Convertir el botón en generador de eventos

1. En `button.c` reemplaza la devolución de `bool` por:

   * Internamente contar milisegundos.
   * Publicar:

     * `EVENT_BUTTON_PRESSED` al detectar flanco de bajada.
     * `EVENT_BUTTON_RELEASED` al detectar flanco de subida.
     * `EVENT_BUTTON_HOLD_1S` cuando se cumple 1000 ms.
     * `EVENT_BUTTON_HOLD_3S` cuando se cumple 3000 ms.
2. Haz que `main.c` llame solo `button_on_tick_1ms()` y no consulte directamente el GPIO.

### Paso 4 – Introducir `power_fsm.c`

1. Implementa el enum `power_mode_t` y `latch_status_t`.
2. Implementa `power_fsm_handle_event` que:

   * Reciba eventos del botón y USB.
   * Use `latch_set_status` para cambiar `latch_status`.
   * Cambie `power_mode` según las reglas de negocio.
   * Publique `EVENT_POWER_MODE_CHANGED` y `EVENT_LATCH_CHANGED`.
3. `main.c` deja de llamar a código de lógica de poder; solo inicializa y suscribe la FSM.

### Paso 5 – Hacer de `neopixel.c` un observer

1. Añade `neopixel_handle_event(const event_t *ev)` que:

   * Actualice un `neopixel_state_t`.
   * Ajuste colores en respuesta a:

     * `EVENT_BUTTON_PRESSED / RELEASED`
     * `EVENT_BUTTON_HOLD_1S / 3S`
     * `EVENT_POWER_MODE_CHANGED`
2. Registra `neopixel_handle_event` con `event_subscribe`.
3. Elimina las llamadas directas desde `power_button.c` (antiguo) hacia `neopixel`.

### Paso 6 – Retirar la lógica vieja

1. Una vez que el FSM + event-driven reproduzcan el comportamiento actual:

   * Elimina `power_button.c` o reduce su rol a “driver de botón” si ya lo migraste.
2. Actualiza la documentación y los encabezados:

   * `power_button.h` → `power_fsm.h` (o deja un stub que redirija a la nueva API).
3. Deja claro en el README el nuevo flujo:

   * Entrada (button) → eventos → `power_fsm` → eventos → `neopixel` / futuro `luckfox_service` / teclado.

---

## 7. Recomendaciones de estilo y legibilidad

* **Nombrar estados y eventos con verbos/frases claras**:

  * Ejemplo: `POWER_MODE_SHUTDOWN_ARMED` es mejor que `SHUTDOWN_1`.
* **Mantener cada archivo con un rol único**:

  * Drivers (GPIO, PIO) no deben conocer la política de negocio.
  * Máquinas de estado no deben llamar directamente a funciones de bajo nivel; siempre a través de módulos de servicio (`latch`, `neopixel_driver`, etc.).
* **Evitar “if gigantes” en `main.c`**:

  * Toda lógica condicional compleja debería vivir dentro de la FSM (`power_fsm.c`) o de observers.

---

Con esta refactorización:

* El **soft latch** y el **botón de encendido** quedan descritos en términos de estados y eventos legibles.
* El **NeoPixel** deja de ser un detalle embebido en la lógica de poder y pasa a ser un **observador reutilizable**.
* Más adelante puedes agregar:

  * Eventos de **modificadores de teclado** (`EVENT_MODIFIER_PRESSED`).
  * Una FSM de **modo de teclado** (normal, Fn, gaming, etc.).
  * Servicios de **I²C/FIFO** que simplemente se suscriben a `EVENT_POWER_MODE_CHANGED` para saber cuándo dormir o despertar.

Si quieres, en el próximo paso puedo escribirte los **headers concretos** (`event.h`, `power_fsm.h`, `button.h`, `latch.h`, `neopixel.h`) ya listos para copiar/pegar al repo.
