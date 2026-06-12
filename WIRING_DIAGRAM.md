# 🐙 ARACHNOBEEST WALKING OCTOPOD — Wiring Diagram
## Evil Bombe Control System — ESP32-S3-CAM + TB6612FNG × 2 + 2× DC Motors

---

## 📋 Overview

```
ESP32-S3-CAM (OV2640)
    ↓
[GPIO pins] → TB6612FNG Motor Driver #1 → LEFT DC Motor
           → TB6612FNG Motor Driver #2 → RIGHT DC Motor
           ↓
        BATTERY (6-12V LiPo or AA)
```

---

## 🔌 Pin Mapping

### ESP32-S3 GPIO Assignments

| Function | GPIO | Purpose |
|----------|------|----------|
| **Left Motor AIN1** | GPIO 40 | Direction control (forward) |
| **Left Motor AIN2** | GPIO 41 | Direction control (backward) |
| **Left Motor PWMA** | GPIO 42 | Speed control (PWM) |
| **Right Motor BIN1** | GPIO 48 | Direction control (forward) |
| **Right Motor BIN2** | GPIO 45 | Direction control (backward) |
| **Right Motor PWMB** | GPIO 45 | Speed control (PWM) |
| **Motor STBY** | GPIO 38 | Standby enable (both drivers) |

### Camera Pins (Built-in OV2640)
All camera pins are **hardwired on ESP32-S3-CAM board** — no additional wiring needed.

---

## ⚙️ TB6612FNG Motor Driver Connections

### Motor Driver #1 (LEFT Motor)

```
TB6612FNG Pin    →    ESP32-S3 GPIO
─────────────────────────────────
AIN1             →    GPIO 40
AIN2             →    GPIO 41
PWMA             →    GPIO 42
AO1 + AO2        →    LEFT DC Motor (A & B)
VM               →    BATTERY + (6-12V)
VCC              →    3V3 Rail (logic power)
GND              →    GND Rail
STBY             →    GPIO 38
```

### Motor Driver #2 (RIGHT Motor)

```
TB6612FNG Pin    →    ESP32-S3 GPIO
─────────────────────────────────
BIN1             →    GPIO 48
BIN2             →    GPIO 45
PWMB             →    GPIO 45
BO1 + BO2        →    RIGHT DC Motor (A & B)
VM               →    BATTERY + (6-12V)
VCC              →    3V3 Rail (logic power)
GND              →    GND Rail
STBY             →    GPIO 38 (shared)
```

---

## 🔋 Power Distribution

```
┌─────────────────────────────────────────┐
│        BATTERY 6-12V (LiPo / AA)        │
│              + ─────── -                │
└─────┬───────────────────────┬───────────┘
      │                       │
      ↓                       ↓
   [VM Pins]             [GND Rail]
   TB6612 #1, #2         (all GND tied together)
   ↓
   Motor Power
   
   
┌─────────────────────────────┐
│ 3V3 Rail (from ESP32)       │
│  VCC pins on TB6612 #1, #2  │
└─────────────────────────────┘
```

### Power Budget
- **Motor current draw:** ~200-500mA per motor (load dependent)
- **TB6612FNG logic:** ~100mA
- **ESP32-S3:** ~80mA
- **Camera:** ~80-150mA
- **Total peak:** ~1.2A @ 6-12V
- **Recommended battery:** 2S LiPo (7.4V) or 8× AA NiMH (9.6V) with 1500+ mAh capacity

---

## 🎮 Gamepad Input Mapping

### Movement Controls

| Button / Input | Action | Effect |
|---|---|---|
| **D-Pad Up** | Drive forward | Both motors forward at current speed |
| **D-Pad Down** | Drive backward | Both motors backward at current speed |
| **D-Pad Left** | Turn left | Left motor slower, right motor faster |
| **D-Pad Right** | Turn right | Right motor slower, left motor faster |
| **Left Stick** | Analog movement | Push forward/back to drive, left/right to turn |
| **A Button** | STOP | All motors immediately stop |
| **B Button** | Spin right (clockwise) | Left motor backward, right motor forward |
| **X Button** | Spin left (counter-clockwise) | Left motor forward, right motor backward |
| **Y Button** | 360° auto-spin | Continuous spin for 3 seconds, then stop |
| **LB / L1** | Speed down | Decrease motor speed (min 50/255) |
| **RB / R1** | Speed up | Increase motor speed (max 255/255) |

### Current Speed Display
Speed indicator appears in serial monitor (0-255):
```
[INPUT] Speed up → 220/255
```

---

## 🔧 Motor Behavior Reference

| Scenario | Left Motor | Right Motor | Result |
|----------|-----------|------------|--------|
| Forward | +speed | +speed | Both move forward equally |
| Backward | -speed | -speed | Both move backward equally |
| Turn Left | speed/2 | +speed | Pivot left around right motor |
| Turn Right | +speed | speed/2 | Pivot right around left motor |
| Spin CW | -speed | +speed | Rotate clockwise in place |
| Spin CCW | +speed | -speed | Rotate counter-clockwise in place |
| Stop | 0 | 0 | All motors disabled |

---

## 📡 Bluepad32 Gamepad Compatibility

**Supported Gamepads:**
- Xbox 360 Controller ✓
- Xbox One Controller ✓
- PlayStation 4 DualShock 4 ✓
- Nintendo Switch Pro Controller ✓
- Generic Bluetooth gamepads (HID compatible) ✓

**Connection:**
1. Power on ESP32-S3
2. Activate gamepad pairing mode
3. ESP32 will auto-detect and connect
4. Serial output: `[GAMEPAD] Controller #0 connected`

---

## 📺 Camera Live View

The OV2640 camera **runs simultaneously** with motor control:
- **Resolution:** VGA (640×480)
- **Format:** JPEG
- **Frame rate:** ~10 FPS at quality level 10
- **View:** Access via WebSerial or custom web interface (future enhancement)

---

## ⚡ Upload & Test

### Flash to ESP32-S3:
```bash
pio run --target upload
```

### Serial Monitor (115200 baud):
```bash
pio device monitor --baud 115200
```

### Expected startup output:
```
╔════════════════════════════════════════╗
║         🐙 EVIL BOMBE v1.0 🐙          ║
║    Arachnobeest Walking Octopod        ║
╚════════════════════════════════════════╝

[MOTOR] TB6612FNG initialized
[CAMERA] OV2640 initialized successfully
[GAMEPAD] Bluepad32 initialized - waiting for controller...
[SETUP] All systems ready!
```

---

## 🛠️ Troubleshooting

| Problem | Solution |
|---------|----------|
| Motors not moving | Check battery voltage, verify STBY pin is HIGH |
| Only one motor works | Check GPIO/direction pin connections for silent motor |
| Gamepad not connecting | Ensure Bluepad32 library is installed, restart gamepad pairing |
| Camera not initializing | Verify OV2640 is soldered correctly, check PSRAM is enabled |
| Motor speed is weak | Increase battery voltage or reduce load on motors |
| Erratic movement | Verify ground connections are solid (common ground) |

---

## 📚 References

- **Bluepad32:** https://github.com/ricardoquesada/bluepad32
- **TB6612FNG Datasheet:** https://www.sparkfun.com/datasheets/Robotics/TB6612FNG.pdf
- **ESP32-S3 Pinout:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3_devkitc_1_v1.1_pinout.csv
- **OV2640 Camera:** https://www.alastairs-lofty-blog.co.uk/2023/09/15/ov2640-camera-setup-on-esp32/

---

**Last Updated:** 2026-06-12  
**Controller:** Evil Bombe Arachnobeest v1.0
