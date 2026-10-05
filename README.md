# 40 kHz ultrasonic rangefinder on LPC1769 (bare-metal C)

[![build](https://github.com/chabanechaouchemohand004-ctrl/lpc1769-ultrasonic-rangefinder/actions/workflows/build.yml/badge.svg)](https://github.com/chabanechaouchemohand004-ctrl/lpc1769-ultrasonic-rangefinder/actions/workflows/build.yml)

Time-of-flight distance measurement for a wire-following mobile robot: 40 kHz burst, analog receive chain, and register-level C firmware on an NXP LPC1769 (Cortex-M3), no HAL. L3 EEA team project (6 people, 15 days), UE 3EE206, Sorbonne Université. **My module: the rangefinder** (analog front end, firmware, simulation).

- **Tools:** arm-none-eabi-gcc, Make, GitHub Actions CI, gcc for PC tests; Keil µVision 5 also supported.
- **Flow:** TIMER1 match → 40 kHz burst → RLC filter → 2 × TL082 (×300) → peak detector → LM311 → TIMER1 hardware capture → distance → UART.
- **Results:** 40.000 kHz burst, **40 ns** timing resolution, echo at 1 m detected at 5.936 ms in simulation (5.83 ms theoretical), firmware ≈ 2 KB flash, PC tests pass.

The firmware here is my post-course version (GCC build, CI, UART FIFO, PC tests), not the exact code that ran on the robot.

<img src="docs/systeme.png" width="520" alt="System overview: robots, base station and supervision">

## 1. Key figures

| Item | Value |
| --- | --- |
| Ultrasound | 40 kHz, 8-period burst (200 µs) |
| Range | 5-250 cm (design target) |
| Timing resolution | 40 ns (TIMER1 at 25 MHz) |
| Measurement rate | 10 / 15 / 20 / 25 Hz (2 switches) |
| Receive filter | Series RLC 100 µH / 160 nF, f0 ≈ 39.8 kHz |
| Gain | ×300 (two TL082 stages, ×10 then ×30) |
| Detection | Peak detector, then LM311 comparator (Vref = 6 V) |
| Debug | UART0 at 115200 baud, 4 modes |

## 2. Architecture

![Measurement chain](docs/chaine.svg)

![Measurement sequence](docs/sequence.svg)

One timer (TIMER1) runs the whole measurement in interrupts, with no busy-wait:

1. **Burst:** match `MR0` toggles P2.13 every 12.5 µs (312 then 313 ticks: exactly 40.000 kHz).
2. **Blind zone (290 µs):** capture disabled to ignore direct TX → RX coupling.
3. **Listen:** capture `CAP1.0` stores the comparator's rising edge in `CR0`. Time of flight = `CR0 − t0`, distance d = c·t/2.
4. **Out of range:** match `MR1` stops listening at 14.5 ms (250 cm).

The echo time is stamped **in hardware** (capture), so it does not depend on interrupt latency.

- **TIMER0** sets the measurement rate.
- **UART0** sends frames without blocking through a 256-byte circular FIFO.
- `DBG0`…`DBG3` received on the UART switch the debug mode.

| DBG switches | Frame sent |
| --- | --- |
| 00 | `T 102 cm` |
| 01 | `T0x0243B0` (raw time of flight, 40 ns ticks) |
| 10 | `T 10 mes/sec` |
| 11 | none (`DBGx` command mode on the UART) |

## 3. Receive-chain simulation

Echo at 1 m through the RLC filter, both gain stages, the peak detector and the comparator. **Detection at 5.936 ms**, against 5.83 ms theoretical at 343 m/s; the 0.1 ms offset is the peak detector's rise time, removable by calibration.

![Receive-chain simulation](docs/simulation-chaine.png)

## 4. Tests on PC (no board)

`firmware/test/` simulates TIMER1 tick by tick and calls the interrupt handlers as the hardware would. It also covers the 32-bit counter overflow during a measurement.

```text
$ cd firmware/test && make
salve : 16 fronts, 7 periodes = 4375 ticks -> 40000.0 Hz
echo a 5,936 ms : tof = 148400 ticks (5.936 ms) -> 1018 mm
couplage a 100/250 us ignore, echo a 1 ms -> 171 mm
pas d'echo -> hors portee apres 14.50 ms
echo a 291 us (distance min) -> 49 mm
UART0 : 115741 bauds (cible 115200)
ALL TESTS PASSED (0 failures)
```

## 5. Pinout (LPC1769)

| Signal | Pin | Role |
| --- | --- | --- |
| Ultrasound burst | P2.13 (GPIO) | IR2304 driver input |
| Echo | P1.18 (CAP1.0) | LM311 comparator output |
| On / off | P0.30 | switch* |
| Measurement rate | P0.28, P0.29 | switches* |
| Debug mode | P1.30, P1.31 | switches (internal pull-up) |
| Measurement LED | P0.22 | on during each measurement |
| UART0 | P0.2 (TX), P0.3 (RX) | debug frames |

\* P0.28 is open-drain and P0.29/P0.30 have no internal pull-up: these three pins need an external pull resistor.

## Build

```bash
sudo apt install gcc-arm-none-eabi
make -C firmware          # -> firmware/build/rangefinder.elf / .bin / .hex
make -C firmware test     # PC tests
```

- The `.bin` gets the LPC17xx boot-ROM checksum automatically (`tools/lpc_checksum.py`).
- CI (`.github/workflows/build.yml`) runs the tests and the build on every push.
- Clocks: CCLK = 100 MHz, PCLK = 25 MHz.

## Repository layout

```text
firmware/src/      main.c, ultrasonic.c (burst + time of flight), uart0.c, board.h (pinout)
firmware/test/     PC tests of ultrasonic.c (simulated registers)
firmware/startup/  startup code, linker script, system_LPC17xx.c
firmware/cmsis/    CMSIS and LPC17xx headers
tools/             flash image checksum
docs/              measurement chain, sequence, simulation
```

## Credits

Team of 6; the other robot modules (induction, DTMF, infrared, base-station link) belong to my teammates and are not in this repo. The rangefinder module is mine.
