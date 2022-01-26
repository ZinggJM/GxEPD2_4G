// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// based on Demo Example from Good Display, available here: http://www.e-paper-display.com/download_detail/downloadsId=806.html
// Panel: GDEM029T94 : https://www.good-display.com/product/360.html
// Controller : SSD1680 : https://www.good-display.com/companyfile/101.html
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2

#include "GxEPD2_290_T94.h"

GxEPD2_290_T94::GxEPD2_290_T94(int16_t cs, int16_t dc, int16_t rst, int16_t busy) :
  GxEPD2_4G_EPD(cs, dc, rst, busy, HIGH, 10000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
  _refresh_mode = full_refresh;
}

void GxEPD2_290_T94::clearScreen(uint8_t value)
{
  if (_refresh_mode == grey_refresh)
  {
    _Force_Init_Full();
    _writeScreenBuffer(0x26, value); // set previous
    _writeScreenBuffer(0x24, value); // set current
    refresh(false);
    _initial_write = false; // initial full screen buffer clean done
  }
  else
  {
    writeScreenBuffer(value);
    refresh(true);
    if (_refresh_mode != grey_refresh) writeScreenBufferAgain(value);
  }
}

void GxEPD2_290_T94::writeScreenBuffer(uint8_t value)
{
  if (_refresh_mode == grey_refresh) 
  {
    _Force_Init_Full();
    _writeScreenBuffer(0x26, value); // set previous
  }
  if (_refresh_mode == full_refresh) _Init_Part();
  if (_initial_write) _writeScreenBuffer(0x26, value); // set previous
  _writeScreenBuffer(0x24, value); // set current
  _initial_write = false; // initial full screen buffer clean done
}

void GxEPD2_290_T94::writeScreenBufferAgain(uint8_t value)
{
  if (_refresh_mode == full_refresh) _Init_Part();
  _writeScreenBuffer(0x24, value); // set current
}

void GxEPD2_290_T94::_writeScreenBuffer(uint8_t command, uint8_t value)
{
  _writeCommand(command);
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; i++)
  {
    _writeData(value);
  }
}

void GxEPD2_290_T94::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_refresh_mode == grey_refresh) _writeImage(0x26, bitmap, x, y, w, h, invert, mirror_y, pgm);
  _writeImage(0x24, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_290_T94::writeImageForFullRefresh(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImage(0x26, bitmap, x, y, w, h, invert, mirror_y, pgm);
  _writeImage(0x24, bitmap, x, y, w, h, invert, mirror_y, pgm);
}


void GxEPD2_290_T94::writeImageAgain(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImage(0x24, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_290_T94::_writeImage(uint8_t command, const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
  x -= x % 8; // byte boundary
  w = wb * 8; // byte boundary
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  uint16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  uint16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (_refresh_mode == grey_refresh) _Force_Init_Full();
  else if (_refresh_mode == full_refresh) _Init_Part();
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(command);
  for (uint16_t i = 0; i < h1; i++)
  {
    for (uint16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data;
      // use wb, h of bitmap for index!
      int16_t idx = mirror_y ? j + dx / 8 + ((h - 1 - (i + dy))) * wb : j + dx / 8 + (i + dy) * wb;
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
      _writeData(data);
    }
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_290_T94::writeImage_4G(const uint8_t bitmap[], uint8_t bpp, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  uint16_t ppb = (bpp == 2 ? 4 : (bpp == 4 ? 2 : (bpp == 8 ? 1 : 0)));
  uint8_t mask = (bpp == 2 ? 0xC0 : (bpp == 4 ? 0xF0 : 0xFF));
  uint8_t grey1 = (bpp == 2 ? 0x80 : 0xA0); // demo limit for 4bpp
  if (ppb == 0) return;
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
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
  _Init_4G();
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(0x26);
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
      _writeData(~out_byte);
    }
  }
  _writeCommand(0x24);
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
      _writeData(~out_byte);
    }
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_290_T94::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_refresh_mode == grey_refresh) _writeImagePart(0x26, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  _writeImagePart(0x24, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_290_T94::writeImagePartAgain(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImagePart(0x24, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_290_T94::_writeImagePart(uint8_t command, const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                     int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % 8; // byte boundary
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  uint16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  uint16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (_refresh_mode == grey_refresh) _Force_Init_Full();
  else if (_refresh_mode == full_refresh) _Init_Part();
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(command);
  for (uint16_t i = 0; i < h1; i++)
  {
    for (uint16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data;
      // use wb_bitmap, h_bitmap of bitmap for index!
      int16_t idx = mirror_y ? x_part / 8 + j + dx / 8 + ((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap : x_part / 8 + j + dx / 8 + (y_part + i + dy) * wb_bitmap;
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
      _writeData(data);
    }
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_290_T94::writeImagePart_4G(const uint8_t bitmap[], uint8_t bpp, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                       int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  uint16_t ppb = (bpp == 2 ? 4 : (bpp == 4 ? 2 : (bpp == 8 ? 1 : 0)));
  uint8_t mask = (bpp == 2 ? 0xC0 : (bpp == 4 ? 0xF0 : 0xFF));
  uint8_t grey1 = (bpp == 2 ? 0x80 : 0xA0); // demo limit for 4bpp
  if (ppb == 0) return;
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
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
  _Init_4G();
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(0x26);
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
      _writeData(~out_byte);
    }
  }
  _writeCommand(0x24);
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
      _writeData(~out_byte);
    }
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_290_T94::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_290_T94::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_290_T94::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    writeImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_290_T94::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImageAgain(bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_290_T94::drawImage_4G(const uint8_t bitmap[], uint8_t bpp, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage_4G(bitmap, bpp, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_290_T94::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                   int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImagePartAgain(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_290_T94::drawImagePart_4G(const uint8_t bitmap[], uint8_t bpp, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                      int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart_4G(bitmap, bpp, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_290_T94::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    drawImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_290_T94::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                   int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    drawImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_290_T94::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    drawImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_290_T94::refresh(bool partial_update_mode)
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

void GxEPD2_290_T94::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
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
  _setPartialRamArea(x1, y1, w1, h1);
  if (_refresh_mode == grey_refresh) _Update_4G();
  else _Update_Part();
}

void GxEPD2_290_T94::powerOff()
{
  _PowerOff();
}

void GxEPD2_290_T94::hibernate()
{
  _PowerOff();
  if (_rst >= 0)
  {
    _writeCommand(0x10); // deep sleep mode
    _writeData(0x1);     // enter deep sleep
    _hibernating = true;
  }
}

void GxEPD2_290_T94::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  _writeCommand(0x11); // set ram entry mode
  _writeData(0x03);    // x increase, y increase : normal mode
  _writeCommand(0x44);
  _writeData(x / 8);
  _writeData((x + w - 1) / 8);
  _writeCommand(0x45);
  _writeData(y % 256);
  _writeData(y / 256);
  _writeData((y + h - 1) % 256);
  _writeData((y + h - 1) / 256);
  _writeCommand(0x4e);
  _writeData(x / 8);
  _writeCommand(0x4f);
  _writeData(y % 256);
  _writeData(y / 256);
}

void GxEPD2_290_T94::_PowerOn()
{
  if (!_power_is_on)
  {
    _writeCommand(0x22);
    _writeData(0xc0);
    _writeCommand(0x20);
    _waitWhileBusy("_PowerOn", power_on_time);
  }
  _power_is_on = true;
}

void GxEPD2_290_T94::_PowerOff()
{
  if (_power_is_on)
  {
    _writeCommand(0x22);
    _writeData(0x83);
    _writeCommand(0x20);
    _waitWhileBusy("_PowerOff", power_off_time);
  }
  _power_is_on = false;
  _using_partial_mode = false;
}

void GxEPD2_290_T94::_InitDisplay()
{
  if (_hibernating) _reset();
  delay(10); // 10ms according to specs
  _writeCommand(0x12);  //SWRESET
  delay(10); // 10ms according to specs
  _writeCommand(0x01); //Driver output control
  _writeData(0x27);
  _writeData(0x01);
  _writeData(0x00);
  _writeCommand(0x11); //data entry mode
  _writeData(0x03);
  _writeCommand(0x3C); //BorderWavefrom
  _writeData(0x05);
  _writeCommand(0x21); //  Display update control
  _writeData(0x00);
  _writeData(0x80);
  _writeCommand(0x18); //Read built-in temperature sensor
  _writeData(0x80);
  _setPartialRamArea(0, 0, WIDTH, HEIGHT);
}

// full screen update LUT 0~3 gray
const unsigned char GxEPD2_290_T94::lut_4G[] PROGMEM =
{
  0x40,  0x48, 0x80, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
  0x8,  0x48, 0x10, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
  0x2,  0x48, 0x4,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
  0x20, 0x48, 0x1,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
  0xA,  0x19, 0x0,  0x3,  0x8,  0x0,  0x0,
  0x14, 0x1,  0x0,  0x14, 0x1,  0x0,  0x3,
  0xA,  0x3,  0x0,  0x8,  0x19, 0x0,  0x0,
  0x1,  0x0,  0x0,  0x0,  0x0,  0x0,  0x1,
  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
  0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x0,  0x0,  0x0,
  0x22, 0x17, 0x41, 0x0,  0x32, 0x1C
};

void GxEPD2_290_T94::_Force_Init_Full()
{
  _Init_Full();
  _refresh_mode = forced_full_refresh;
}

void GxEPD2_290_T94::_Init_Full()
{
  _InitDisplay();
  _PowerOn();
  _refresh_mode = full_refresh;
}

void GxEPD2_290_T94::_Init_4G()
{
  //_InitDisplay();
  if (_hibernating) _reset();
  delay(10); // 10ms according to specs
  _writeCommand(0x12); // soft reset
  //delay(10); // 10ms according to specs
  _waitWhileBusy("_Init_4G", full_refresh_time);
  _writeCommand(0x74); //set analog block control
  _writeData(0x54);
  _writeCommand(0x7E); //set digital block control
  _writeData(0x3B);
  _writeCommand(0x01); //Driver output control
  _writeData(0x27);
  _writeData(0x01);
  _writeData(0x00);
  _setPartialRamArea(0, 0, WIDTH, HEIGHT);
  _writeCommand(0x3C); //BorderWavefrom
  _writeData(0x00);
  _writeCommand(0x2C);     //VCOM Voltage
  _writeData(0x1C); //LUT_DATA_4Gray[158]);    //0x1C
  _writeCommand(0x3F); //EOPQ
  _writeData(0x22); //LUT_DATA_4Gray[153]);
  _writeCommand(0x03); //VGH
  _writeData(0x17); //LUT_DATA_4Gray[154]);
  _writeCommand(0x04); //
  _writeData(0x41); //LUT_DATA_4Gray[155]); //VSH1
  _writeData(0x0); //LUT_DATA_4Gray[156]); //VSH2
  _writeData(0x32); //LUT_DATA_4Gray[157]); //VSL
  _writeCommand(0x21); //  Display update control
  _writeData(0x00);
  _writeData(0x80);
  _writeCommand(0x32);
  _writeDataPGM(lut_4G, 153);
  _PowerOn();
  _refresh_mode = grey_refresh;
}

void GxEPD2_290_T94::_Init_Part()
{
  _InitDisplay();
  _PowerOn();
  _refresh_mode = fast_refresh;
}

void GxEPD2_290_T94::_Update_Full()
{
  _writeCommand(0x22);
  _writeData(0xf4);
  _writeCommand(0x20);
  _waitWhileBusy("_Update_Full", full_refresh_time);
}

void GxEPD2_290_T94::_Update_4G()
{
  _writeCommand(0x22);
  _writeData(0xc4);
  _writeCommand(0x20);
  _waitWhileBusy("_Update_4G", full_refresh_time);
}

void GxEPD2_290_T94::_Update_Part()
{
  _writeCommand(0x22);
  _writeData(0xfc);
  _writeCommand(0x20);
  _waitWhileBusy("_Update_Part", partial_refresh_time);
}

void GxEPD2_290_T94::drawGreyLevels()
{
  clearScreen(); delay(1000);
  _Init_4G();
  _writeCommand(0x24);
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 32; i++)
  {
    _writeData(0x00);
  }
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 32; i++)
  {
    _writeData(0xFF);
  }
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 32; i++)
  {
    _writeData(0x00);
  }
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 32; i++)
  {
    _writeData(0xFF);
  }
  _writeCommand(0x26);
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 32; i++)
  {
    _writeData(0x00);
  }
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 32; i++)
  {
    _writeData(0x00);
  }
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 32; i++)
  {
    _writeData(0xFF);
  }
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 32; i++)
  {
    _writeData(0xFF);
  }
  _Update_4G();
}
