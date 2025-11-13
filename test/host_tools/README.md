# Herramientas de host

Este directorio contiene utilidades de referencia para interactuar con el teclado I²C desde un equipo host.

* `i2c_dump.py`: script en Python que utiliza `smbus2` para leer registros y vaciar la FIFO de eventos.

## Dependencias

```bash
pip install smbus2
```

## Uso

```bash
python i2c_dump.py --bus 1 --address 0x32
```

El script imprimirá el estado actual de los registros principales y, opcionalmente, vaciará la FIFO para mostrar los eventos pendientes.
