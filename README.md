# 📺 AnimeNotifier for ESP32-C3

A compact ESP32-C3 powered anime release notifier featuring a monochrome OLED display, Wi-Fi connectivity, and a simple three-button interface. AnimeNotifier automatically checks for newly released anime episodes and displays notifications directly on the device.
It also shows movies on one side.

Initial version only supports 3 animes and refreshes every 15 minutes (9000000)

---

## ✨ Features

- 📡 Wi-Fi connectivity
- 📺 Check for newly released anime episodes
- 🔔 New episode notifications *(If you add the buzzer)*
- 📱 128×64 OLED user interface
- 🎮 Three-button navigation
- ⚡ Fast and lightweight
- 🔄 Automatic refresh
- 🌙 Low power ESP32-C3 platform
- 🔋 Real-time battery level indicator

---

## 📷 Preview

> *(Coming Soon)*



## 🛠 Hardware

### Tested On

- ESP32-C3 SuperMini

### Components

- 128×64 OLED (SH1106/SSD1306 I²C)
- ESP32-C3 SuperMini
- 3 Push Buttons
- 5v Active Buzzer
- J1059 Module 
- 900 mAh battery

---

## 📌 Pin Configuration

| Component | GPIO |
|-----------|-----:|
| OLED SDA | GPIO8 |
| OLED SCL | GPIO9 |
| UP Button | GPIO7 |
| DOWN Button | GPIO3 |
| OK Button | GPIO6 |
| Battery ADC | GPIO01 |
| Buzzer *(Optional)* | Configure in source |

---

## 📦 Software

Built using:

- Arduino Framework
- PlatformIO
- ArduinoJson
- U8g2 Graphics Library

---

## 📥 Installation

### Clone

```bash
git clone https://github.com/yourusername/AnimeNotifier.git
```

### Open

Open the project using **PlatformIO** inside VS Code.

### Install Dependencies

```ini
lib_deps =
    olikraus/U8g2
    bblanchon/ArduinoJson
```


### Build & Upload

```bash
pio run
pio run --target upload
```

---



## 🚀 Planned Features

- [ ] MAL authentication
- [ ] AniList authentication
- [ ] OTA firmware updates
- [ ] Multiple anime tracking
- [ ] Episode countdown timer
- [ ] Timezone selection
- [ ] Deep sleep mode
- [ ] Custom notification sounds
- [ ] Brightness control
- [ ] Watch history

---

## 🤝 Contributing

Pull requests are welcome.

For major changes, please open an issue first to discuss what you would like to improve.

---

## 🐞 Bug Reports

If you find a bug, please include:

- Board model
- Firmware version
- Serial Monitor logs
- Steps to reproduce

---

## 📜 License

This project is licensed under the MIT License.

---

## ❤️ Acknowledgements

- Espressif
- U8g2
- ArduinoJson
- The anime community ❤️

---

## ⭐ Support

If you like this project, consider giving it a ⭐ on GitHub!

It helps others discover the project and motivates future development.