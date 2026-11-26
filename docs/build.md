# ESP32 build verification

The sanitized sketch compiled locally on 26 November 2026 with Arduino CLI bundled in Arduino IDE, targeting `esp32:esp32:esp32` (generic ESP32 Dev Module).

| Dependency | Local version |
| --- | --- |
| ESP32 Arduino core | 3.3.12 |
| Adafruit MPU6050 | 2.2.9 |
| Adafruit Unified Sensor | 1.1.15 |
| Adafruit BusIO | 1.17.4 |
| SparkFun MAX3010x Pulse and Proximity Sensor Library | 1.1.2 |
| TinyGPSPlus | 1.0.3 |

```text
Sketch uses 314084 bytes (23%) of program storage space.
Maximum is 1310720 bytes.
Global variables use 24148 bytes (7%) of dynamic memory,
leaving 303532 bytes for local variables. Maximum is 327680 bytes.
```

Command:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/smart_helmet
```

This check did not upload firmware or operate attached hardware. Use the actual board's settings before deployment.
