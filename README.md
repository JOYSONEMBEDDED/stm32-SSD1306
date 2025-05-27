# STM32 OLED Display with SSD1306 and Custom Glyphs
STM32F401CCU6 BLACK PILL
This project demonstrates interfacing an SSD1306 OLED display with an STM32 microcontroller (STM32F4 series). The display is driven over I2C and showcases:

- Text rendering using custom fonts.
- Animated GIF frame display (Horse animation).
- I2C scanning.
- Custom glyph rendering (Ohm symbol `Ω` in 7x10 format).
- Debugging output via UART2.

## Features

✅ SSD1306 OLED initialization  
✅ GIF-like frame animation using `horsegif1` to `horsegif12`  
✅ I2C bus scanning for devices  
✅ Draw custom 7x10 font character (like `Ω`)  
✅ UART debug messages using `printf`

---

## Hardware Used

- **Microcontroller**: STM32F4xx (e.g., STM32F401/STM32F103)
- **Display**: 0.96" SSD1306 OLED (I2C Interface)
- **Other**:
  - ST-Link V2 (for programming)
  - UART2 for debug output
  - I2C1 for display communication

---

## Directory Structure

```plaintext
├── Core/
│   ├── Inc/
│   └── Src/
├── Drivers/
│   ├── SSD1306/
│   ├── fonts.h
│   ├── gifs.h
├── main.c
├── README.md
How to Build
Open the project in STM32CubeIDE or your preferred STM32 IDE.

Make sure your I2C and UART configurations match the OLED and debugger.

Connect the SSD1306 display to I2C1.

Compile and flash the firmware.

Open a serial terminal (e.g., PuTTY or Tera Term) at the baud rate of 115200 to see debug logs.

GIF Display
The SSD1306_ShowGif() function displays 12 frames in sequence to create a simple animation. These frames (horsegif1, horsegif2, ...) are stored in gifs.h.

c
Copy
Edit
SSD1306_ShowGif(12, horsegif1, horsegif2, ..., horsegif12);
Custom Font Example
A 7x10 custom font character Ω is defined and displayed using:

c
Copy
Edit
SSD1306_PutCustomChar7x10(x, y, OhmChar_7x10, &Font_7x10, SSD1306_COLOR_WHITE);
