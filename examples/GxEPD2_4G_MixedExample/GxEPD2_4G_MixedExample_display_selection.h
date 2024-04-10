// Display Library example for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// Display Library based on Demo Example from Good Display: https://www.good-display.com/companyfile/32/
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2_4G

// Supporting Arduino Forum Topics (closed, read only):
// Good Display ePaper for Arduino: https://forum.arduino.cc/t/good-display-epaper-for-arduino/419657
// Waveshare e-paper displays with SPI: https://forum.arduino.cc/t/waveshare-e-paper-displays-with-spi/467865
//
// Add new topics in https://forum.arduino.cc/c/using-arduino/displays/23 for new questions and issues

// select the display driver class (only one) for your  panel
//#define GxEPD2_DRIVER_CLASS GxEPD2_154_GDEY0154D67 // GDEY0154D67 200x200, SSD1681, (FPC-B001 20.05.21)
//#define GxEPD2_DRIVER_CLASS GxEPD2_213_flex // GDEW0213I5F 104x212, UC8151 (IL0373), (WFT0213CZ16)
//#define GxEPD2_DRIVER_CLASS GxEPD2_213_GDEY0213B74 // GDEY0213B74 122x250, SSD1680, (FPC-A002 20.04.08)
//#define GxEPD2_DRIVER_CLASS GxEPD2_270     // GDEW027W3   176x264, EK79652 (IL91874), (WFI0190CZ22)
//#define GxEPD2_DRIVER_CLASS GxEPD2_290_T5  // GDEW029T5   128x296, UC8151 (IL0373), (WFT0290CZ10)
//#define GxEPD2_DRIVER_CLASS GxEPD2_290_T5D // GDEW029T5D  128x296, UC8151D, (WFT0290CZ10)
//#define GxEPD2_DRIVER_CLASS GxEPD2_290_I6FD // GDEW029I6FD  128x296, UC8151D, (WFT0290CZ10)
//#define GxEPD2_DRIVER_CLASS GxEPD2_371     // GDEW0371W7  240x416, UC8171 (IL0324), (missing)
//#define GxEPD2_DRIVER_CLASS GxEPD2_420     // GDEW042T2   400x300, UC8176 (IL0398), (WFT042CZ15)
//#define GxEPD2_DRIVER_CLASS GxEPD2_420_GDEY042T81 // GDEY042T81 400x300, SSD1683 (no inking)
//#define GxEPD2_DRIVER_CLASS GxEPD2_426_GDEQ0426T82 // GDEQ0426T82 480x800, SSD1677 (P426010-MF1-A)
//#define GxEPD2_DRIVER_CLASS GxEPD2_750_T7  // GDEW075T7   800x480, EK79655 (GD7965), (WFT0583CZ61)

// these panels don't support mixed content because of missing partial window update support
//#define GxEPD2_DRIVER_CLASS GxEPD2_290_T94 // GDEM029T94  128x296, SSD1680

// SS is usually used for CS. define here for easy change
#ifndef EPD_CS
#define EPD_CS SS
#endif

#if defined(GxEPD2_DRIVER_CLASS)

#if defined (ESP8266)
#define MAX_DISPLAY_BUFFER_SIZE (81920ul-34000ul-5000ul) // ~34000 base use, change 5000 to your application use
#define MAX_HEIGHT_BW(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))
#define MAX_HEIGHT_4G(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 4) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 4))
// adapt the constructor parameters to your wiring
GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=D8*/ EPD_CS, /*DC=D3*/ 0, /*RST=D4*/ 2, /*BUSY=D2*/ 4));
// mapping of Waveshare e-Paper ESP8266 Driver Board, new version
//GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=15*/ EPD_CS, /*DC=4*/ 4, /*RST=2*/ 2, /*BUSY=5*/ 5));
// mapping of Waveshare e-Paper ESP8266 Driver Board, old version
//GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=15*/ EPD_CS, /*DC=4*/ 4, /*RST=5*/ 5, /*BUSY=16*/ 16));
GxEPD2_4G_BW_R<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_BW(GxEPD2_DRIVER_CLASS)> display_bw(display.epd2);
#undef MAX_DISPLAY_BUFFER_SIZE
#undef MAX_HEIGHT_BW
#undef MAX_HEIGHT_4G
#endif

#if defined(ESP32)
#define MAX_DISPLAY_BUFFER_SIZE 65536ul // e.g.
#define MAX_HEIGHT_BW(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))
#define MAX_HEIGHT_4G(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 4) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 4))
// adapt the constructor parameters to your wiring
#if defined(ARDUINO_LOLIN_D32_PRO)
GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=5*/ EPD_CS, /*DC=*/ 0, /*RST=*/ 2, /*BUSY=*/ 15));
#elif defined(ARDUINO_ESP32_DEV) // e.g. TTGO T8 ESP32-WROVER
GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=5*/ EPD_CS, /*DC=*/ 2, /*RST=*/ 0, /*BUSY=*/ 4));
#else
GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=5*/ EPD_CS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));
#endif
GxEPD2_4G_BW_R<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_BW(GxEPD2_DRIVER_CLASS)> display_bw(display.epd2);
#undef MAX_DISPLAY_BUFFER_SIZE
#undef MAX_HEIGHT_BW
#undef MAX_HEIGHT_4G
#endif

// can't use package "STMF1 Boards (STM32Duino.com)" (Roger Clark) anymore with Adafruit_GFX, use "STM32 Boards (selected from submenu)" (STMicroelectronics)
#if defined(ARDUINO_ARCH_STM32)
#define MAX_DISPLAY_BUFFER_SIZE 15000ul // ~15k is a good compromise
#define MAX_HEIGHT_BW(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))
#define MAX_HEIGHT_4G(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 4) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 4))
// adapt the constructor parameters to your wiring
GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=PA4*/ EPD_CS, /*DC=*/ PA3, /*RST=*/ PA2, /*BUSY=*/ PA1));
GxEPD2_4G_BW_R<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_BW(GxEPD2_DRIVER_CLASS)> display_bw(display.epd2);
#undef MAX_DISPLAY_BUFFER_SIZE
#undef MAX_HEIGHT_BW
#undef MAX_HEIGHT_4G
#endif

#if defined(__AVR)
#if defined (ARDUINO_AVR_MEGA2560) // Note: SS is on 53 on MEGA
#define MAX_DISPLAY_BUFFER_SIZE 5000 // e.g. full height for 200x200
#else // Note: SS is on 10 on UNO, NANO
#define MAX_DISPLAY_BUFFER_SIZE 800 // 
#endif
#define MAX_HEIGHT_BW(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))
#define MAX_HEIGHT_4G(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 4) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 4))
// adapt the constructor parameters to your wiring
GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=*/ EPD_CS, /*DC=*/ 8, /*RST=*/ 9, /*BUSY=*/ 7));
GxEPD2_4G_BW_R<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_BW(GxEPD2_DRIVER_CLASS)> display_bw(display.epd2);
#undef MAX_HEIGHT_BW
#undef MAX_HEIGHT_4G
#endif

#if defined(ARDUINO_ARCH_SAM)
#define MAX_DISPLAY_BUFFER_SIZE 32768ul // e.g., up to 96k
#define MAX_HEIGHT_BW(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))
#define MAX_HEIGHT_4G(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 4) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 4))
// adapt the constructor parameters to your wiring
GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=10*/ EPD_CS, /*DC=*/ 8, /*RST=*/ 9, /*BUSY=*/ 7));
GxEPD2_4G_BW_R<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_BW(GxEPD2_DRIVER_CLASS)> display_bw(display.epd2);
#undef MAX_DISPLAY_BUFFER_SIZE
#undef MAX_HEIGHT_BW
#undef MAX_HEIGHT_4G
#endif

#if defined(ARDUINO_ARCH_SAMD)
#define MAX_DISPLAY_BUFFER_SIZE 15000ul // ~15k is a good compromise
#define MAX_HEIGHT_BW(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))
#define MAX_HEIGHT_4G(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 4) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 4))
// adapt the constructor parameters to your wiring
GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=4*/ 4, /*DC=*/ 7, /*RST=*/ 6, /*BUSY=*/ 5));
//GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=4*/ 4, /*DC=*/ 3, /*RST=*/ 2, /*BUSY=*/ 1)); // my Seed XIOA0
//GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=4*/ 3, /*DC=*/ 2, /*RST=*/ 1, /*BUSY=*/ 0)); // my other Seed XIOA0
GxEPD2_4G_BW_R<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_BW(GxEPD2_DRIVER_CLASS)> display_bw(display.epd2);
#undef MAX_DISPLAY_BUFFER_SIZE
#undef MAX_HEIGHT_BW
#undef MAX_HEIGHT_4G
#endif

#if defined(ARDUINO_ARCH_RP2040)
#define MAX_DISPLAY_BUFFER_SIZE 131072ul // e.g. half of available ram
#define MAX_HEIGHT_BW(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))
#define MAX_HEIGHT_4G(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 4) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 4))
#if defined(ARDUINO_NANO_RP2040_CONNECT)
// adapt the constructor parameters to your wiring
GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=*/ EPD_CS, /*DC=*/ 8, /*RST=*/ 9, /*BUSY=*/ 7));
#endif
#if defined(ARDUINO_RASPBERRY_PI_PICO)
// adapt the constructor parameters to your wiring
//GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=*/ EPD_CS, /*DC=*/ 8, /*RST=*/ 9, /*BUSY=*/ 7)); // my proto board
// mapping of GoodDisplay DESPI-PICO. NOTE: uses alternate HW SPI pins!
GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=*/ 3, /*DC=*/ 2, /*RST=*/ 1, /*BUSY=*/ 0)); // DESPI-PICO
//GxEPD2_4G_4G<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_4G(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=*/ 3, /*DC=*/ 2, /*RST=*/ 11, /*BUSY=*/ 10)); // DESPI-PICO modified
#endif
GxEPD2_4G_BW_R<GxEPD2_DRIVER_CLASS, MAX_HEIGHT_BW(GxEPD2_DRIVER_CLASS)> display_bw(display.epd2);
#undef MAX_DISPLAY_BUFFER_SIZE
#undef MAX_HEIGHT_BW
#undef MAX_HEIGHT_4G
#endif

#endif
