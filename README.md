# pong-game

# equipment
```
#use ARdoino uno r3 or esp32 dev module
#use oled 1.3 inc
#use joystick x2
##physic
```
# ARDUINO PIN
```
OLED SH1106 (I2C)
VCC → 5V
GND → GND
SDA → A4
SCL → A5

Joystick Left (P1)
VRy → A0
SW → 2
VCC → 5V
GND → GND

Joystick Right (P2)
VRy → A1
SW → 3
VCC → 5V
GND → GND
```

# ESP32 PIN
```
OLED SH1106 (I2C)
VCC → 3.3V
GND → GND
SCL → GPIO 22
SDA → GPIO 21

Buzzer
GND → GND
VCC → GPIO 25

Joystick P1 
VRy → GPIO 34
VCC → 3.3V
GND → GND

Joystick P2 
VRy → GPIO 35
VCC → 3.3V
GND → GND

button1 GPIO 13  // Select PvP
button2 GPIO 12   // Select PvAI
button3 GPIO 14   // Select AIvsAI

```
