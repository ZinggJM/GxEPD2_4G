// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// based on Demo Example from Good Display: https://www.good-display.com/comp/xcompanyFile/downloadNew.do?appId=24&fid=1373&id=1125
// Panel: GDEY075T7 : https://www.good-display.com/product/396.html
// Controller: UC8179 : https://v4.cecdn.yun300.cn/100001_1909185148/UC8179.pdf
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2

#include "GxEPD2_750_GDEY075T7.h"

GxEPD2_750_GDEY075T7::GxEPD2_750_GDEY075T7(int16_t cs, int16_t dc, int16_t rst, int16_t busy) :
  GxEPD2_4G_EPD(cs, dc, rst, busy, LOW, 10000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
  _refresh_mode = full_refresh;
}

void GxEPD2_750_GDEY075T7::clearScreen(uint8_t value)
{
  // full refresh needed for all cases (previous != screen)
  if (!_init_display_done) _InitDisplay();
  _writeScreenBuffer(0x10, value); // set previous
  _writeScreenBuffer(0x13, value); // set current
  refresh(false); // full refresh
  _initial_write = false;
}

void GxEPD2_750_GDEY075T7::writeScreenBuffer(uint8_t value)
{
  if (_initial_write) return clearScreen(value);
  if (!_init_display_done && !_init_4G_done) _InitDisplay();
  _setPartialRamArea(0, 0, WIDTH, HEIGHT);
  if (_init_display_done) return _writeScreenBuffer(0x13, value); // set current
  else // 4G
  {
    _writeScreenBuffer(0x13, value); // set current
    _writeScreenBuffer(0x10, value); // set previous
  }
}

void GxEPD2_750_GDEY075T7::writeScreenBufferAgain(uint8_t value)
{
  _writeScreenBuffer(0x13, value); // set current
  _writeScreenBuffer(0x10, value); // set previous
}

void GxEPD2_750_GDEY075T7::_writeScreenBuffer(uint8_t command, uint8_t value)
{
  _writeCommand(command);
  _startTransfer();
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; i++)
  {
    _transfer(value);
  }
  _endTransfer();
}

void GxEPD2_750_GDEY075T7::_writeScreenBufferArea(uint8_t command, uint8_t value, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  _setPartialRamArea(x, y, w, h);
  _writeCommand(command);
  _startTransfer();
  for (uint32_t i = 0; i < uint32_t(w) * uint32_t(h) / 8; i++)
  {
    _transfer(value);
  }
  _endTransfer();
}

void GxEPD2_750_GDEY075T7::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.print("_writeImage("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", "); Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  if (_refresh_mode == grey_refresh) _writeImage(0x10, bitmap, x, y, w, h, invert, mirror_y, pgm);
  _writeImage(0x13, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_750_GDEY075T7::writeImageForFullRefresh(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImage(0x10, bitmap, x, y, w, h, invert, mirror_y, pgm); // set previous
  _writeImage(0x13, bitmap, x, y, w, h, invert, mirror_y, pgm); // set current
}

//void GxEPD2_750_GDEY075T7::writeImageAgain(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
//{
//  _writeImage(0x10, bitmap, x, y, w, h, invert, mirror_y, pgm); // set previous
//  _writeImage(0x13, bitmap, x, y, w, h, invert, mirror_y, pgm); // set current
//}

void GxEPD2_750_GDEY075T7::_writeImage(uint8_t command, const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  uint16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
  x -= x % 8; // byte boundary
  w = wb * 8; // byte boundary
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (_refresh_mode == grey_refresh) _Force_Init_Full();
  else if (_refresh_mode == full_refresh) _Init_Part();
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
  _writeCommand(0x91); // partial in
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(command);
  _startTransfer();
  for (int16_t i = 0; i < h1; i++)
  {
    for (int16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data;
      // use wb, h of bitmap for index!
      uint16_t idx = mirror_y ? j + dx / 8 + uint16_t((h - 1 - (i + dy))) * wb : j + dx / 8 + uint16_t(i + dy) * wb;
      if (pgm)
      {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
        data = pgm_read_byte(&bitmap[idx]);
#else
        data = bitmap[idx];
#endif
      }
      else
      {
        data = bitmap[idx];
      }
      if (invert) data = ~data;
      _transfer(data);
    }
  }
  _endTransfer();
  _writeCommand(0x92); // partial out
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_750_GDEY075T7::writeImage_4G(const uint8_t bitmap[], uint8_t bpp, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.print("writeImage_4G("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", "); Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  uint16_t ppb = (bpp == 2 ? 4 : (bpp == 4 ? 2 : (bpp == 8 ? 1 : 0)));
  uint8_t mask = (bpp == 2 ? 0xC0 : (bpp == 4 ? 0xF0 : 0xFF));
  uint8_t grey1 = (bpp == 2 ? 0x80 : 0xA0); // demo limit for 4bpp
  if (ppb == 0) return;
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  int16_t wbc = (w + 7) / 8; // width bytes on controller
  x -= x % 8; // byte boundary on controller
  w = wbc * 8; // byte boundary on controller
  int16_t wb = (w + ppb - 1) / ppb; // width bytes of bitmap, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  uint16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  uint16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (!_init_4G_done) _Init_4G();
  _writeCommand(0x91); // partial in
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(0x10);
  _startTransfer();
  for (uint16_t i = 0; i < h1; i++) // lines
  {
    for (uint16_t j = 0; j < w1 / ppb; j += bpp) // out bytes
    {
      uint8_t out_byte = 0;
      for (uint16_t k = 0; k < bpp; k++) // in bytes (bpp per out byte)
      {
        uint8_t in_byte;
        // use wb, h of bitmap for index!
        uint32_t idx = mirror_y ? j + k + dx / ppb + uint32_t((h - 1 - (i + dy))) * wb : j + k + dx / ppb + uint32_t(i + dy) * wb;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          in_byte = pgm_read_byte(&bitmap[idx]);
#else
          in_byte = bitmap[idx];
#endif
        }
        else
        {
          in_byte = bitmap[idx];
        }
        if (invert) in_byte = ~in_byte;
        for (uint16_t n = 0; n < ppb; n++) // bits, nibbles (ppb per in byte)
        {
          out_byte <<= 1;
          uint8_t nibble = in_byte & mask;
          if (nibble == mask) out_byte |= 0x01;//white
          else if (nibble == 0x00) out_byte |= 0x00;  //black
          else if (nibble >= grey1) out_byte |= 0x01;  //gray1
          else out_byte |= 0x00; //gray2
          in_byte <<= bpp;
        }
      }
      _transfer(out_byte);
    }
  }
  _endTransfer();
  _writeCommand(0x13);
  _startTransfer();
  for (uint16_t i = 0; i < h1; i++) // lines
  {
    for (uint16_t j = 0; j < w1 / ppb; j += bpp) // out bytes
    {
      uint8_t out_byte = 0;
      for (uint16_t k = 0; k < bpp; k++) // in bytes (bpp per out byte)
      {
        uint8_t in_byte;
        // use wb, h of bitmap for index!
        uint32_t idx = mirror_y ? j + k + dx / ppb + uint32_t((h - 1 - (i + dy))) * wb : j + k + dx / ppb + uint32_t(i + dy) * wb;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          in_byte = pgm_read_byte(&bitmap[idx]);
#else
          in_byte = bitmap[idx];
#endif
        }
        else
        {
          in_byte = bitmap[idx];
        }
        if (invert) in_byte = ~in_byte;
        for (uint16_t n = 0; n < ppb; n++) // bits, nibbles (ppb per in byte)
        {
          out_byte <<= 1;
          uint8_t nibble = in_byte & mask;
          if (nibble == mask) out_byte |= 0x01;//white
          else if (nibble == 0x00) out_byte |= 0x00;  //black
          else if (nibble >= grey1) out_byte |= 0x00;  //gray1
          else out_byte |= 0x01; //gray2
          in_byte <<= bpp;
        }
      }
      _transfer(out_byte);
    }
  }
  _endTransfer();
  _writeCommand(0x92); // partial out
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_750_GDEY075T7::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImagePart(0x13, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

//void GxEPD2_750_GDEY075T7::writeImagePartAgain(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
//    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
//{
//  _writeImagePart(0x13, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
//  _writeImagePart(0x10, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
//}

void GxEPD2_750_GDEY075T7::_writeImagePart(uint8_t command, const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  uint16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % 8; // byte boundary
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (_refresh_mode == grey_refresh) _Force_Init_Full();
  else if (_refresh_mode == full_refresh) _Init_Part();
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
  _writeCommand(0x91); // partial in
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(command);
  _startTransfer();
  for (int16_t i = 0; i < h1; i++)
  {
    for (int16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data;
      // use wb_bitmap, h_bitmap of bitmap for index!
      uint16_t idx = mirror_y ? x_part / 8 + j + dx / 8 + uint16_t((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap : x_part / 8 + j + dx / 8 + uint16_t(y_part + i + dy) * wb_bitmap;
      if (pgm)
      {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
        data = pgm_read_byte(&bitmap[idx]);
#else
        data = bitmap[idx];
#endif
      }
      else
      {
        data = bitmap[idx];
      }
      if (invert) data = ~data;
      _transfer(data);
    }
  }
  _endTransfer();
  _writeCommand(0x92); // partial out
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_750_GDEY075T7::writeImagePart_4G(const uint8_t bitmap[], uint8_t bpp, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  uint16_t ppb = (bpp == 2 ? 4 : (bpp == 4 ? 2 : (bpp == 8 ? 1 : 0)));
  uint8_t mask = (bpp == 2 ? 0xC0 : (bpp == 4 ? 0xF0 : 0xFF));
  uint8_t grey1 = (bpp == 2 ? 0x80 : 0xA0); // demo limit for 4bpp
  if (ppb == 0) return;
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + ppb - 1) / ppb; // width bytes, bitmaps are padded
  x_part -= x_part % ppb; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % ppb; // byte boundary
  w = ppb * ((w + ppb - 1) / ppb); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  uint16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  uint16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (!_init_4G_done) _Init_4G();
  _writeCommand(0x91); // partial in
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(0x10);
  _startTransfer();
  for (uint16_t i = 0; i < h1; i++) // lines
  {
    for (uint16_t j = 0; j < w1 / ppb; j += bpp) // out bytes
    {
      uint8_t out_byte = 0;
      for (uint16_t k = 0; k < bpp; k++) // in bytes (bpp per out byte)
      {
        uint8_t in_byte;
        // use wb_bitmap, h_bitmap of bitmap for index!
        uint32_t idx = mirror_y ? x_part / ppb + j + k + dx / ppb + uint32_t((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap : x_part / ppb + j + k + dx / ppb + uint32_t(y_part + i + dy) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          in_byte = pgm_read_byte(&bitmap[idx]);
#else
          in_byte = bitmap[idx];
#endif
        }
        else
        {
          in_byte = bitmap[idx];
        }
        if (invert) in_byte = ~in_byte;
        for (uint16_t n = 0; n < ppb; n++) // bits, nibbles (ppb per in byte)
        {
          out_byte <<= 1;
          uint8_t nibble = in_byte & mask;
          if (nibble == mask) out_byte |= 0x01;//white
          else if (nibble == 0x00) out_byte |= 0x00;  //black
          else if (nibble >= grey1) out_byte |= 0x01;  //gray1
          else out_byte |= 0x00; //gray2
          in_byte <<= bpp;
        }
      }
      _transfer(out_byte);
    }
  }
  _endTransfer();
  _writeCommand(0x13);
  _startTransfer();
  for (uint16_t i = 0; i < h1; i++) // lines
  {
    for (uint16_t j = 0; j < w1 / ppb; j += bpp) // out bytes
    {
      uint8_t out_byte = 0;
      for (uint16_t k = 0; k < bpp; k++) // in bytes (bpp per out byte)
      {
        uint8_t in_byte;
        // use wb_bitmap, h_bitmap of bitmap for index!
        uint32_t idx = mirror_y ? x_part / ppb + j + k + dx / ppb + uint32_t((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap : x_part / ppb + j + k + dx / ppb + uint32_t(y_part + i + dy) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          in_byte = pgm_read_byte(&bitmap[idx]);
#else
          in_byte = bitmap[idx];
#endif
        }
        else
        {
          in_byte = bitmap[idx];
        }
        if (invert) in_byte = ~in_byte;
        for (uint16_t n = 0; n < ppb; n++) // bits, nibbles (ppb per in byte)
        {
          out_byte <<= 1;
          uint8_t nibble = in_byte & mask;
          if (nibble == mask) out_byte |= 0x01;//white
          else if (nibble == 0x00) out_byte |= 0x00;  //black
          else if (nibble >= grey1) out_byte |= 0x00;  //gray1
          else out_byte |= 0x01; //gray2
          in_byte <<= bpp;
        }
      }
      _transfer(out_byte);
    }
  }
  _endTransfer();
  _writeCommand(0x92); // partial out
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_750_GDEY075T7::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_750_GDEY075T7::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_750_GDEY075T7::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    writeImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_750_GDEY075T7::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImageAgain(bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_750_GDEY075T7::drawImage_4G(const uint8_t bitmap[], uint8_t bpp, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage_4G(bitmap, bpp, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_750_GDEY075T7::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImagePartAgain(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_750_GDEY075T7::drawImagePart_4G(const uint8_t bitmap[], uint8_t bpp, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart_4G(bitmap, bpp, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_750_GDEY075T7::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    drawImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_750_GDEY075T7::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    drawImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_750_GDEY075T7::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    drawImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_750_GDEY075T7::refresh(bool partial_update_mode)
{
  if (partial_update_mode) refresh(0, 0, WIDTH, HEIGHT);
  else
  {
    if (_refresh_mode == forced_full_refresh) _refresh_mode = full_refresh;
    if (_refresh_mode == fast_refresh) _Init_Full();
    if (_refresh_mode == grey_refresh) _Update_4G();
    else _Update_Full();
    _initial_refresh = false; // initial full update done
  }
}

void GxEPD2_750_GDEY075T7::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  if (_initial_refresh) return refresh(false); // initial update needs be full update
  if (_refresh_mode == forced_full_refresh) return refresh(false);
  // intersection with screen
  int16_t w1 = x < 0 ? w + x : w; // reduce
  int16_t h1 = y < 0 ? h + y : h; // reduce
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  w1 = x1 + w1 < int16_t(WIDTH) ? w1 : int16_t(WIDTH) - x1; // limit
  h1 = y1 + h1 < int16_t(HEIGHT) ? h1 : int16_t(HEIGHT) - y1; // limit
  if ((w1 <= 0) || (h1 <= 0)) return;
  // make x1, w1 multiple of 8
  w1 += x1 % 8;
  if (w1 % 8 > 0) w1 += 8 - w1 % 8;
  x1 -= x1 % 8;
  if (_refresh_mode == full_refresh) _Init_Part();
  if (usePartialUpdateWindow) _writeCommand(0x91); // partial in
  else if (usePartialUpdateWindow_4G && (_refresh_mode == grey_refresh)) _writeCommand(0x91); // partial in
  _setPartialRamArea(x1, y1, w1, h1);
  if (_refresh_mode == grey_refresh) _Update_4G();
  else _Update_Part();
  if (usePartialUpdateWindow) _writeCommand(0x92); // partial out
  else if (usePartialUpdateWindow_4G && (_refresh_mode == grey_refresh)) _writeCommand(0x92); // partial out
}

void GxEPD2_750_GDEY075T7::powerOff(void)
{
  _PowerOff();
}

void GxEPD2_750_GDEY075T7::hibernate()
{
  _PowerOff();
  if (_rst >= 0)
  {
    _writeCommand(0x07); // deep sleep
    _writeData(0xA5);    // check code
    _hibernating = true;
    _init_display_done = false;
    _init_4G_done = false;
  }
}

void GxEPD2_750_GDEY075T7::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  uint16_t xe = (x + w - 1) | 0x0007; // byte boundary inclusive (last byte)
  uint16_t ye = y + h - 1;
  x &= 0xFFF8; // byte boundary
  _writeCommand(0x90); // partial window
  _writeData(x / 256);
  _writeData(x % 256);
  _writeData(xe / 256);
  _writeData(xe % 256);
  _writeData(y / 256);
  _writeData(y % 256);
  _writeData(ye / 256);
  _writeData(ye % 256);
  //_writeData(0x00); // Gates scan only inside of the partial window (looks worse)
  _writeData(0x01); // Gates scan both inside and outside of the partial window. (default)
}

void GxEPD2_750_GDEY075T7::_PowerOn()
{
  if (!_power_is_on)
  {
    _writeCommand(0x04);
    _waitWhileBusy("_PowerOn", power_on_time);
  }
  _power_is_on = true;
}

void GxEPD2_750_GDEY075T7::_PowerOff()
{
  if (_power_is_on)
  {
    _writeCommand(0x02); // power off
    _waitWhileBusy("_PowerOff", power_off_time);
  }
  _power_is_on = false;
}

void GxEPD2_750_GDEY075T7::_InitDisplay()
{
  //Serial.println("_InitDisplay");
  if (_hibernating) _reset();
  else
  {
    // soft reset needed to undo any TSFIX
    _writeCommand(0x00); // PANEL SETTING
    _writeData(0x1e);    // soft reset; KW: 3f, KWR: 2F, BWROTP: 0f, BWOTP: 1f
    delay(2);
    _writeCommand(0x00); // PANEL SETTING
    _writeData(0x1f);    // KW: 3f, KWR: 2F, BWROTP: 0f, BWOTP: 1f
    //_waitWhileBusy("_InitDisplay reset", power_on_time); // up to 15ms
    delay(20);
    _power_is_on = false;
  }
  _writeCommand(0x00); // PANEL SETTING
  _writeData(0x1f);    // KW: 3f, KWR: 2F, BWROTP: 0f, BWOTP: 1f
  // same POWER SETTING as from OTP
  _writeCommand(0x01); // POWER SETTING
  _writeData (0x07); // enable internal
  _writeData (0x07); // VGH=20V,VGL=-20V
  _writeData (0x3f); // VDH=15V
  _writeData (0x3f); // VDL=-15V
  _writeData (0x09); // VDHR=4.2V
  //Enhanced display drive(Add 0x06 command)
  _writeCommand(0x06); //Booster Soft Start
  _writeData (0x17);
  _writeData (0x17);
  _writeData (0x28);
  _writeData (0x17);
  _writeCommand(0x61); //tres
  _writeData (WIDTH / 256); //source 800
  _writeData (WIDTH % 256);
  _writeData (HEIGHT / 256); //gate 480
  _writeData (HEIGHT % 256);
  _writeCommand(0x15); // DUSPI
  _writeData(0x00);    // disabled
  _writeCommand(0x50); // VCOM AND DATA INTERVAL SETTING
  _writeData(0x29);    // LUTKW, N2OCP: copy new to old
  _writeData(0x07);    // CDI 10hsynch (default)
  _writeCommand(0x60); // TCON SETTING
  _writeData(0x22);    // S2G G2S, 12 (default)
  _writeCommand(0xE3); // PWS
  _writeData(0x22);    // VCOM 2 line period, Source 2 * 660ns
  _init_display_done = true;
  _init_4G_done = false;
}

// full screen update LUT 0~3 gray
const unsigned char GxEPD2_750_GDEY075T7::lut_20_vcom0_4G[] PROGMEM =
{
  0x00, 0x0A, 0x00, 0x00, 0x00, 0x01,
  0x60, 0x14, 0x14, 0x00, 0x00, 0x01,
  0x00, 0x14, 0x0A, 0x00, 0x00, 0x01,
  0x00, 0x13, 0x0A, 0x01, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

//R21 white: o-n: 1-1
const unsigned char GxEPD2_750_GDEY075T7::lut_21_ww_4G[] PROGMEM =
{ // 10 w
  0x40, 0x0A, 0x00, 0x00, 0x00, 0x01, // 01 00 00 00
  0x90, 0x14, 0x14, 0x00, 0x00, 0x01, // 10 01 00 00
  0x10, 0x14, 0x0A, 0x00, 0x00, 0x01, // 00 01 00 00
  0xA0, 0x13, 0x0A, 0x00, 0x00, 0x01, // 10 10 00 00
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

//R22H light grey: o-n: 0-1
const unsigned char GxEPD2_750_GDEY075T7::lut_22_bw_4G[] PROGMEM =
{ // 10 w
  0x40, 0x0A, 0x00, 0x00, 0x00, 0x01, // 01 00 00 00
  0x90, 0x14, 0x14, 0x00, 0x00, 0x01, // 10 01 00 00
  0x00, 0x14, 0x0A, 0x00, 0x00, 0x01, // 00 00 00 00
  0x99, 0x0C, 0x01, 0x03, 0x04, 0x01, // 10 01 10 01
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

//R23H dark grey: o-n: 1-0
const unsigned char GxEPD2_750_GDEY075T7::lut_23_wb_4G[] PROGMEM =
{ // 01 b
  0x40, 0x0A, 0x00, 0x00, 0x00, 0x01, // 01 00 00 00
  0x90, 0x14, 0x14, 0x00, 0x00, 0x01, // 10 01 00 00
  0x00, 0x14, 0x0A, 0x00, 0x00, 0x01, // 00 00 00 00
  0x99, 0x0B, 0x04, 0x04, 0x01, 0x01, // 10 01 10 01
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

//R24H black: o-n: 0-0
const unsigned char GxEPD2_750_GDEY075T7::lut_24_bb_4G[] PROGMEM =
{ // 01 b
  0x80, 0x0A, 0x00, 0x00, 0x00, 0x01, // 10 00 00 00
  0x90, 0x14, 0x14, 0x00, 0x00, 0x01, // 10 01 00 00
  0x20, 0x14, 0x0A, 0x00, 0x00, 0x01, // 00 10 00 00
  0x50, 0x13, 0x01, 0x00, 0x00, 0x01, // 01 01 00 00
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// experimental partial screen update LUTs with balanced charge
// LUTs are filled with zeroes

#define T1 30 // charge balance pre-phase
#define T2  5 // optional extension
#define T3 30 // color change phase (b/w)
#define T4  5 // optional extension for one color

const unsigned char GxEPD2_750_GDEY075T7::lut_20_LUTC_partial[] PROGMEM =
{
  0x00, T1, T2, T3, T4, 1, // 00 00 00 00
};

const unsigned char GxEPD2_750_GDEY075T7::lut_21_LUTWW_partial[] PROGMEM =
{ // 10 w
  0x00, T1, T2, T3, T4, 1, // 00 00 00 00
};

const unsigned char GxEPD2_750_GDEY075T7::lut_22_LUTKW_partial[] PROGMEM =
{ // 10 w
  //0x48, T1, T2, T3, T4, 1, // 01 00 10 00
  0x5A, T1, T2, T3, T4, 1, // 01 01 10 10 more white
};

const unsigned char GxEPD2_750_GDEY075T7::lut_23_LUTWK_partial[] PROGMEM =
{ // 01 b
  0x84, T1, T2, T3, T4, 1, // 10 00 01 00
  //0xA5, T1, T2, T3, T4, 1, // 10 10 01 01 more black
};

const unsigned char GxEPD2_750_GDEY075T7::lut_24_LUTKK_partial[] PROGMEM =
{ // 01 b
  0x00, T1, T2, T3, T4, 1, // 00 00 00 00
};

const unsigned char GxEPD2_750_GDEY075T7::lut_25_LUTBD_partial[] PROGMEM =
{
  0x00, T1, T2, T3, T4, 1, // 00 00 00 00
};

void GxEPD2_750_GDEY075T7::_Force_Init_Full()
{
  _Init_Full();
  _refresh_mode = forced_full_refresh;
}

void GxEPD2_750_GDEY075T7::_Init_Full()
{
  //Serial.println("_Init_Full");
  _InitDisplay();
  _writeCommand(0x00); // panel setting
  _writeData(0x1f);    // full update LUT from OTP
  _PowerOn();
  _init_4G_done = false;
  if (_refresh_mode == grey_refresh)
  {
    _writeScreenBuffer(0x13, 0xff); // set current
    _writeScreenBuffer(0x10, 0xff); // set previous
    _initial_write = false;
    _refresh_mode = forced_full_refresh;
  } else _refresh_mode = full_refresh;
}

void GxEPD2_750_GDEY075T7::_Init_4G()
{
  //Serial.println("_Init_4G");
  _InitDisplay();
  _writeCommand(0x00); //panel setting
  _writeData(0x3f);    // 4G update LUT from registers
  _writeCommand(0x50); // VCOM AND DATA INTERVAL SETTING
  _writeData(0x31);    // LUTBD
  _writeData(0x07);
  _writeCommand(0x82); // vcom_DC setting
  _writeData (0x30);   // -2.5V same value as in OTP
  _writeCommand(0x20);
  _writeDataPGM(lut_20_vcom0_4G, sizeof(lut_20_vcom0_4G));
  _writeCommand(0x21);
  _writeDataPGM(lut_21_ww_4G, sizeof(lut_21_ww_4G));
  _writeCommand(0x22);
  _writeDataPGM(lut_22_bw_4G, sizeof(lut_22_bw_4G));
  _writeCommand(0x23);
  _writeDataPGM(lut_23_wb_4G, sizeof(lut_23_wb_4G));
  _writeCommand(0x24);
  _writeDataPGM(lut_24_bb_4G, sizeof(lut_24_bb_4G));
  _writeCommand(0x25);
  _writeDataPGM(lut_21_ww_4G, sizeof(lut_21_ww_4G));
  _writeScreenBuffer(0x13, 0xff); // set current
  _writeScreenBuffer(0x10, 0xff); // set previous
  _initial_write = false;
  _init_display_done = false;
  _init_4G_done = true;
  _refresh_mode = grey_refresh;
}

void GxEPD2_750_GDEY075T7::_Init_Part()
{
  //Serial.println("_Init_Part");
  _InitDisplay();
  if (hasFastPartialUpdate)
  {
    if (useFastPartialUpdateFromOTP)
    {
      _writeCommand(0xE0); // Cascade Setting (CCSET)
      _writeData(0x02);    // TSFIX
      _writeCommand(0xE5); // Force Temperature (TSSET)
      _writeData(0x6E);    // 110
    }
    else
    {
      _writeCommand(0x01); // POWER SETTING
      _writeData (0x07);
      _writeData (0x07);   // VGH=20V,VGL=-20V
      _writeData (0x3f);   // VDH=15V
      _writeData (0x3f);   // VDL=-15V
      _writeData (0x03);   // VDHR=3V (default)
      _writeCommand(0x00); // panel setting
      _writeData(0x3f);    // partial update LUT from registers
      _writeCommand(0x82); // vcom_DC setting
      _writeData (0x30);   // -2.5V same value as in OTP
      _writeCommand(0x50); // VCOM AND DATA INTERVAL SETTING
      _writeData(0x39);    // LUTBD, N2OCP: copy new to old
      _writeData(0x07);
      _writeCommand(0x20);
      _writeDataPGM(lut_20_LUTC_partial, sizeof(lut_20_LUTC_partial), 42 - sizeof(lut_20_LUTC_partial));
      _writeCommand(0x21);
      _writeDataPGM(lut_21_LUTWW_partial, sizeof(lut_21_LUTWW_partial), 42 - sizeof(lut_21_LUTWW_partial));
      _writeCommand(0x22);
      _writeDataPGM(lut_22_LUTKW_partial, sizeof(lut_22_LUTKW_partial), 42 - sizeof(lut_22_LUTKW_partial));
      _writeCommand(0x23);
      _writeDataPGM(lut_23_LUTWK_partial, sizeof(lut_23_LUTWK_partial), 42 - sizeof(lut_23_LUTWK_partial));
      _writeCommand(0x24);
      _writeDataPGM(lut_24_LUTKK_partial, sizeof(lut_24_LUTKK_partial), 42 - sizeof(lut_24_LUTKK_partial));
      _writeCommand(0x25);
      _writeDataPGM(lut_25_LUTBD_partial, sizeof(lut_25_LUTBD_partial), 42 - sizeof(lut_25_LUTBD_partial));
    }
  }
  _PowerOn();
  _init_4G_done = false;
  if (_refresh_mode == grey_refresh)
  {
    _writeScreenBuffer(0x13, 0xff); // set current
    _writeScreenBuffer(0x10, 0xff); // set previous
    _initial_write = false;
    _refresh_mode = forced_full_refresh;
  } else _refresh_mode = fast_refresh;
}

void GxEPD2_750_GDEY075T7::_Update_Full()
{
  if (useFastFullUpdate)
  {
    _writeCommand(0xE0); // Cascade Setting (CCSET)
    _writeData(0x02);    // TSFIX
    _writeCommand(0xE5); // Force Temperature (TSSET)
    _writeData(0x5A);    // 90
  }
  else
  {
    _writeCommand(0xE0); // Cascade Setting (CCSET)
    _writeData(0x00);    // no TSFIX, Temperature value is defined by internal temperature sensor
    _writeCommand(0x41); // TSE, Enable Temperature Sensor
    _writeData(0x00);    // TSE, Internal temperature sensor switch
  }
  _PowerOn();
  _writeCommand(0x12); //display refresh
  _waitWhileBusy("_Update_Full", full_refresh_time);
}

void GxEPD2_750_GDEY075T7::_Update_4G()
{
  _PowerOn();
  _writeCommand(0x12); //display refresh
  _waitWhileBusy("_Update_4G", grey_refresh_time);
}

void GxEPD2_750_GDEY075T7::_Update_Part()
{
  _PowerOn();
  _writeCommand(0x12); //display refresh
  _waitWhileBusy("_Update_Part", partial_refresh_time);
}
