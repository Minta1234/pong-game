# 🎮 ESP32 Pong Game

A classic Pong game running on ESP32/Arduino with OLED display, 
joystick controls, buzzer feedback, and a Web UI via WiFi.

---

## 📦 Features
- PvP / PvAI / AIvsAI game modes
- Smart AI with ball prediction
- Web UI via WiFi (ESP32 only)
- OLED SH1106 1.3" display
- Buzzer sound effects
- OLED brightness control

---

## 🛠️ Equipment
- Arduino Uno R3 **or** ESP32 Dev Module **or** ESP32-S3
- OLED SH1106 1.3"
- Joystick x2
- Buzzer x1 (ESP32 only)
- Buttons x3 (ESP32 only)

---

## 📌 Wiring

### Arduino UNO R3
| Component | Pin |
|---|---|
| OLED VCC | 5V |
| OLED GND | GND |
| OLED SDA | A4 |
| OLED SCL | A5 |
| Joystick P1 VRy | A0 |
| Joystick P1 SW | D2 |
| Joystick P2 VRy | A1 |
| Joystick P2 SW | D3 |

### ESP32 / ESP32-S3
| Component | Pin |
|---|---|
| OLED VCC | 3.3V |
| OLED SDA | GPIO 21 |
| OLED SCL | GPIO 22 |
| Buzzer | GPIO 25 |
| Joystick P1 VRy | GPIO 34 |
| Joystick P2 VRy | GPIO 35 |
| Button PvP | GPIO 13 |
| Button PvAI | GPIO 12 |
| Button AIvsAI | GPIO 14 |

---

## 📚 Libraries Required
| Library | Install via |
|---|---|
| U8g2 | Arduino Library Manager |
| ESP32Servo | Arduino Library Manager |
| WiFi | Built-in (ESP32) |
| WebServer | Built-in (ESP32) |

---

## 🚀 How To Upload
1. Install **Arduino IDE**
2. Install libraries above
3. Select correct board:
   - `Arduino Uno` for Arduino
   - `ESP32 Dev Module` for ESP32
   - `ESP32S3 Dev Module` for ESP32-S3
4. Select correct **COM Port**
5. Click **Upload**

---

## 🌐 Web UI (ESP32 Only)
1. Connect WiFi: `ESP32-PONG` / Password: `12345678`
2. Open browser → `http://192.168.4.1`
3. Control game mode, brightness, and view live score

---

## 🎮 How To Play
| Action | Control |
|---|---|
| Move Paddle | Push Joystick Up/Down |
| Select PvP | Button 1 / Web UI |
| Select PvAI | Button 2 / Web UI |
| Select AIvsAI | Button 3 / Web UI |
| Reset | Web UI Reset button |

**First to 10 points wins!**

---

---

## 📄 License
MIT License - Free to use and modify
