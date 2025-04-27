# ESP32 Bluetooth Media Controller

A customizable Bluetooth media controller based on Waveshare ESP32-S3-Zero, inspired by the Sony RM-MC25C controller design with modern enhancements.

![IMG_6219](https://github.com/user-attachments/assets/ef3c9c69-658e-4597-95cd-5974ae673c5a)


## Project Overview

This project transforms a Waveshare ESP32-S3-Zero into a powerful Bluetooth media controller with:
- Media control (play/pause, next/previous track, volume)
- Deep sleep for power saving (2-minute timeout)
- Visual feedback via RGB LEDs
- Modified RM-MC25C functionality with additional features
- Customizable button actions (including modifier key combinations)
- Smart power management using USB-C LiPo charger module

## Hardware Requirements

### Components
- Waveshare ESP32-S3-Zero (Main controller)
- LiPo battery (3.7V, 500mAh+ recommended)
- USB-C LiPo Charger/Booster Module (like https://www.amazon.com.mx/dp/B09YD5C9QC)
- Tactile buttons (x5)
- NeoPixel LED (optional)
- Resistors (10kΩ for pull-ups)
- JST PH 2.0 connector (for battery)

### Power Setup
The USB-C LiPo Charger Module provides:
- Built-in charging circuit (no separate TP4056 needed)
- 5V boost output (powers ESP32 directly)
- On/off button functionality

Special wiring:
- ESP32 5V/GND connected to module's boosted 5V output
- DOWN button wired to charger module's power button pin
  - Single press: Turns power on/off
  - Still functions as modifier button when powered on

## Wiring Diagram

### Power Connections
LiPo (+) → Charger Module "BAT+"
LiPo (-) → Charger Module "BAT-"

Charger Module "5V OUT" → ESP32 "5V"
Charger Module "GND" → ESP32 "GND"
DOWN button → Charger Module "PWR" pin

### Button Connections (All using INPUT_PULLUP)
| Button | GPIO | Function               |
|--------|------|------------------------|
| DOWN   | 7    | Modifier/Power         |
| PLAY   | 9    | Play/Pause             |
| BACK   | 10   | Previous Track/Vol-    |
| NEXT   | 11   | Next Track/Vol+        |
| STOP   | 12   | Stop/Wake-up           |

### RGB LED Connections
- Red LED: GPIO 6
- Blue LED: GPIO 5
- Green LED: GPIO 4
- NeoPixel: GPIO 21

### Troubleshooting: False Wake-Ups from Deep Sleep

If your ESP32 randomly wakes from deep sleep shortly after entering it (especially when using GPIO12 as the wake-up trigger), follow these steps to resolve electrical noise issues:

Recommended Fix
Hardware Modifications (Required for GPIO12):

Add a 10kΩ pull-up resistor between GPIO12 and 3.3V.

Add a 100nF ceramic capacitor between GPIO12 and GND (for debouncing).

Circuit Diagram:
```
[3.3V] ----[10kΩ]----.
                     |
GPIO12 --------------+
                     |
                    [ ] STOP button (to GND)
                     |
                   [100nF]
                     |
                   [GND]
```

Software Configuration:
In setup(), add these lines to stabilize the wake-up pin:

```
void setup() {
...
  pinMode(STOP, INPUT_PULLUP);  // Enable internal pull-up
  gpio_pullup_en(GPIO_NUM_12);  // Force strong pull-up
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, LOW);  // Wake on LOW (button press)
...
```

Why This Happens
ESP32 wake-up pins are sensitive to noise (floating voltages, EMI, or button bounce).

The 10kΩ resistor ensures a clean HIGH state, while the 100nF capacitor filters noise.

If Issues Persist
Use a multimeter to verify GPIO12 voltage:

Should read ~3.3V when idle (no button press).

Should read 0V when pressed.

Shorten wires between the button and ESP32.

Replace the tactile switch (defective switches can cause floating signals).

## Features

### Core Functionality
- Bluetooth LE keyboard emulation
- Media control shortcuts inspired by RM-MC25C
- Enhanced with modern ESP32-S3 capabilities
- Visual status indicators

## Installation

1. Hardware Setup:
   - Solder components according to wiring diagram
   - Ensure proper polarity for battery connections

2. Software Setup:
   - Install required Arduino libraries:
     - Adafruit_NeoPixel
     - BleKeyboard

3. Upload the Sketch:
   - Open WS-ESP32-Media-Controller-Sleep.ino
   - Select ESP32-S3 board
   - Upload to device

## Usage

1. Power On: Press DOWN button once
2. Pairing: Connect via Bluetooth as "RM-MC25C Media Controller"
3. Controls:
   - Standard button presses for media control
   - DOWN + other buttons for secondary functions
4. Power Off: Press DOWN button twice quickly

## Customization Options

- Change timeout duration in code
- Adjust LED colors
- Modify button mappings
- Create custom enclosure

## Technical Specifications

Bluetooth: BLE 4.2
Operating Voltage: 3.3V (regulated from 5V boost)
Current Draw: ~15mA active, ~5μA sleep
Battery Life: ~20 hours (500mAh LiPo)

## **Features pending to add**
* 3D Printed housing to put all together inside

---

## **License**

This project is licensed under the **GNU GENERAL PUBLIC LICENSE**. See the [LICENSE](LICENSE) file for details.

---

## **Acknowledgments**
This project was made possible thanks to these valuable resources:

- **[ESP32-BLE-Keyboard Library](https://github.com/T-vK/ESP32-BLE-Keyboard)** by T-vK  
  *For enabling Bluetooth HID keyboard functionality on ESP32*

- **[Waveshare ESP32-S3-Zero Documentation](https://www.waveshare.com/wiki/ESP32-S3-Zero)**  
  *For hardware specifications and pinout references*

- **[USB-C LiPo Charger/Booster Module](https://www.amazon.com.mx/dp/B09YD5C9QC)**  
  *For providing integrated battery charging and power management*

Special thanks to the open source community and all contributors whose work helped shape this project.

## 📬 Contact & Support
---

**Project Repository**:  
🔗 [github.com/geekinsanemx/ESP32-Bluetooth-Media-Controller](https://github.com/geekinsanemx/ESP32-Bluetooth-Media-Controller)  

**Creator**:  
👨‍💻 [GeekInsaneMX](https://github.com/geekinsanemx)  

**Need Help?**  
- Open an [Issue](https://github.com/geekinsanemx/ESP32-Bluetooth-Media-Controller/issues) on GitHub  
- Share your project modifications - I'd love to see what you build!  

*Your feedback and contributions are always welcome!* 🚀
---
