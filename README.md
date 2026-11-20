# Brou Crusher (VST3)

Bitcrusher con interfaz propia. Hecho con JUCE.

## Perillas

- **Drive**: ganancia antes del crusher. Satura suave por encima de 0.5; en modo Wrap provoca overflow.
- **Bits**: profundidad de bits de 1 a 16, con decimales (6.5 bits, por ejemplo).
- **Rate**: frecuencia de muestreo reducida (500 Hz a 48 kHz). Por encima de la del proyecto no hace nada.
- **Jitter**: el reloj de muestreo se vuelve inestable, como un convertidor barato.
- **Dither**: ruido TPDF antes de cuantizar. Suaviza la distorsion a pocos bits.
- **Tone**: filtro pasa-bajos al final para domar los agudos.
- **Mix** y **Output**.
- **Mode**:
  - Round: cuantizacion normal.
  - Truncate: redondea siempre hacia abajo, suena mas sucio y asimetrico.
  - Wrap: overflow tipo entero, lo que pasa de +1 reaparece desde -1. Muy agresivo.
- **Anti-alias**: filtro de 4to orden antes del muestreo. Apagado = aliasing crudo (el sonido clasico). Encendido = crunch mas limpio.

## Ideas de sonido

- Lo-fi clasico: Bits 8, Rate 12000, Anti-alias apagado.
- Sampler de los 90: Bits 12, Rate 26000, Anti-alias encendido, Dither 0.3.
- Destruccion total: Drive 18 dB, Bits 4, Rate 4000, Mode Wrap, Tone 6000 Hz.
- Maquina rota: Bits 6, Rate 9000, Jitter 0.6.

## Compilar en Windows

Visual Studio 2022 (C++), CMake (con "Add to PATH") y Git. Luego doble clic a `build.bat`.
O sube la carpeta a un repo de GitHub y descarga el resultado desde Actions > Artifacts.
