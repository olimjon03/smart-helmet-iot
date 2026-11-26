# Firmware pin map

These connections are derived from the submitted Arduino sketch. Photos show the physical assembly but do not establish electrical ratings or continuity.

| ESP32 pin | Connection | Firmware configuration |
| --- | --- | --- |
| GPIO32 | MPU6050 and MAX30102 SDA | Shared I2C |
| GPIO33 | MPU6050 and MAX30102 SCL | Shared I2C |
| GPIO34 RX | NEO-6M TX | UART2, 9600 baud, 8N1 |
| GPIO13 TX | NEO-6M RX | UART2, 9600 baud, 8N1 |
| GPIO16 RX | SIM7600 TX | UART1, 115200 baud, 8N1 |
| GPIO17 TX | SIM7600 RX | UART1, 115200 baud, 8N1 |
| GPIO27 | Button to ground | `INPUT_PULLUP`, active low |
| GPIO26 | Red LED control | Active high |
| GPIO25 | Green LED control | Active high |
| GPIO14 | Buzzer control | Digital high/low |

MPU6050 is addressed at `0x68`, configured for ±8 g acceleration, ±500°/s rotation and a 21 Hz filter. MAX30102 uses the SparkFun `MAX30105` class and is configured but never sampled by the main loop.

## Assembly checks

- Share a reference ground between the controller and connected modules.
- Check the exact breakout's power input and UART/I2C logic requirements before wiring. The sketch does not specify a complete power circuit.
- Confirm LED current limiting and buzzer drive requirements for the installed components.
- Verify the SIM7600 carrier's supply and power-on arrangement independently; the sketch has no power-key control.
- Validate UART TX/RX orientation and the physical board variant before upload.

No battery rating, power budget, runtime estimate or complete electrical schematic has been verified.
