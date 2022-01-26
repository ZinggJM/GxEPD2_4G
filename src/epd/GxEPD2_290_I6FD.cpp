// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// based on Demo Example from Good Display: http://www.e-paper-display.com/download_list/downloadcategoryid=34&isMode=false.html
// Panel: GDEW029I6FD : https://www.good-display.com/product/209.html
// Controller: UC8151D : https://v4.cecdn.yun300.cn/100001_1909185148/UC8151D.pdf
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2

#include "GxEPD2_290_I6FD.h"

GxEPD2_290_I6FD::GxEPD2_290_I6FD(int16_t cs, int16_t dc, int16_t rst, int16_t busy) :
  GxEPD2_4G_EPD(cs, dc, rst, busy, LOW, 10000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
  _refresh_mode = full_refresh;
}

void GxEPD2_290_I6FD::clearScreen(uint8_t value)
{
  writeScreenBuffer(value);
  refresh(true);
  if (_refresh_mode != grey_refresh) writeScreenBuffer(value);
}

void GxEPD2_290_I6FD::writeScreenBuffer(uint8_t value)
{
  _initial_write = false; // initial full screen buffer clean done
  if (_refresh_mode == full_refresh) _Init_Part();
  _writeCommand(0x13); // set current
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; i++)
  {
    _writeData(value);
  }
  if (_initial_refresh || (_refresh_mode == grey_refresh))
  {
    _writeCommand(0x10); // preset previous
    for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; i++)
    {
      _writeData(0xFF); // 0xFF is white
    }
  }
}

void GxEPD2_290_I6FD::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
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
  _writeCommand(0x91); // partial in
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(0x13);
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
  _writeCommand(0x92); // partial out
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_290_I6FD::writeImage_4G(const uint8_t bitmap[], uint8_t bpp, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
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
  _writeCommand(0x91); // partial in
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(0x10);
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
      _writeData(out_byte);
    }
  }
  _writeCommand(0x13);
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
      _writeData(out_byte);
    }
  }
  _writeCommand(0x92); // partial out
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_290_I6FD::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
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
  int16_t wbc = (w + 7) / 8; // width bytes on controller
  x -= x % 8; // byte boundary on controller
  w = wbc * 8; // byte boundary on controller
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
  _writeCommand(0x91); // partial in
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(0x13);
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
  _writeCommand(0x92); // partial out
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_290_I6FD::writeImagePart_4G(const uint8_t bitmap[], uint8_t bpp, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
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
  _writeCommand(0x91); // partial in
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(0x10);
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
      _writeData(out_byte);
    }
  }
  _writeCommand(0x13);
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
      _writeData(out_byte);
    }
  }
  _writeCommand(0x92); // partial out
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_290_I6FD::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_290_I6FD::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                     int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_290_I6FD::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    writeImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_290_I6FD::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_290_I6FD::drawImage_4G(const uint8_t bitmap[], uint8_t bpp, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage_4G(bitmap, bpp, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_290_I6FD::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_290_I6FD::drawImagePart_4G(const uint8_t bitmap[], uint8_t bpp, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                       int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart_4G(bitmap, bpp, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_290_I6FD::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(black, color, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImage(black, color, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_290_I6FD::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(black, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImagePart(black, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_290_I6FD::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeNative(data1, data2, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeNative(data1, data2, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_290_I6FD::refresh(bool partial_update_mode)
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

void GxEPD2_290_I6FD::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
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
  _writeCommand(0x91); // partial in
  _setPartialRamArea(x1, y1, w1, h1);
  if (_refresh_mode == grey_refresh) _Update_4G();
  else _Update_Part();
  _writeCommand(0x92); // partial out
}

void GxEPD2_290_I6FD::powerOff(void)
{
  _PowerOff();
}

void GxEPD2_290_I6FD::hibernate()
{
  _PowerOff();
  if (_rst >= 0)
  {
    _writeCommand(0x07); // deep sleep
    _writeData(0xA5);    // check code
    _hibernating = true;
  }
}

void GxEPD2_290_I6FD::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  uint16_t xe = (x + w - 1) | 0x0007; // byte boundary inclusive (last byte)
  uint16_t ye = y + h - 1;
  x &= 0xFFF8; // byte boundary
  _writeCommand(0x90); // partial window
  //_writeData(x / 256);
  _writeData(x % 256);
  //_writeData(xe / 256);
  _writeData(xe % 256);
  _writeData(y / 256);
  _writeData(y % 256);
  _writeData(ye / 256);
  _writeData(ye % 256);
  _writeData(0x01); // don't see any difference
  //_writeData(0x00); // don't see any difference
}

void GxEPD2_290_I6FD::_PowerOn()
{
  if (!_power_is_on)
  {
    _writeCommand(0x04);
    _waitWhileBusy("_PowerOn", power_on_time);
  }
  _power_is_on = true;
}

void GxEPD2_290_I6FD::_PowerOff()
{
  _writeCommand(0x02); // power off
  _waitWhileBusy("_PowerOff", power_off_time);
  _power_is_on = false;
  _refresh_mode = full_refresh;
}

void GxEPD2_290_I6FD::_InitDisplay()
{
  if (_hibernating) _reset();
  _writeCommand(0x00); //panel setting
  _writeData(0x1f);    //LUT from OTP, 128x296
  _writeCommand(0x61); //resolution setting
  _writeData (WIDTH);
  _writeData (HEIGHT >> 8);
  _writeData (HEIGHT & 0xFF);
}

// full screen update LUT 0~3 gray
const unsigned char GxEPD2_290_I6FD::lut_20_vcom0_4G[] PROGMEM =
{
  0x00, 0x0A, 0x00, 0x00, 0x00, 0x01,
  0x00, 0x14, 0x14, 0x00, 0x00, 0x01,
  0x00, 0x14, 0x00, 0x00, 0x00, 0x01,
  0x00, 0x13, 0x0A, 0x01, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00,
};
//R21
const unsigned char GxEPD2_290_I6FD::lut_21_ww_4G[] PROGMEM =
{
  0x40, 0x0A, 0x00, 0x00, 0x00, 0x01,
  0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
  0x10, 0x14, 0x0A, 0x00, 0x00, 0x01,
  0xA0, 0x13, 0x01, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
//R22H  r
const unsigned char GxEPD2_290_I6FD::lut_22_bw_4G[] PROGMEM =
{
  0x40, 0x0A, 0x00, 0x00, 0x00, 0x01,
  0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
  0x00, 0x14, 0x0A, 0x00, 0x00, 0x01,
  0x99, 0x0C, 0x01, 0x03, 0x04, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
//R23H  w
const unsigned char GxEPD2_290_I6FD::lut_23_wb_4G[] PROGMEM =
{
  0x40, 0x0A, 0x00, 0x00, 0x00, 0x01,
  0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
  0x00, 0x14, 0x0A, 0x00, 0x00, 0x01,
  0x99, 0x0B, 0x04, 0x04, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
//R24H  b
const unsigned char GxEPD2_290_I6FD::lut_24_bb_4G[] PROGMEM =
{
  0x80, 0x0A, 0x00, 0x00, 0x00, 0x01,
  0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
  0x20, 0x14, 0x0A, 0x00, 0x00, 0x01,
  0x50, 0x13, 0x01, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

//partial screen update LUT
//#define Tx19 0x19 // original value is 25 (phase length)
#define Tx19 0x10   // new value for test is 16 (phase length)
//#define Tx19 0x20   // new value for test is 32 (phase length)
const unsigned char GxEPD2_290_I6FD::lut_20_vcomDC_partial[] PROGMEM =
{
  0x00, Tx19, 0x01, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00,
};

const unsigned char GxEPD2_290_I6FD::lut_21_ww_partial[] PROGMEM =
{
  0x00, Tx19, 0x01, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char GxEPD2_290_I6FD::lut_22_bw_partial[] PROGMEM =
{
  0x80, Tx19, 0x01, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char GxEPD2_290_I6FD::lut_23_wb_partial[] PROGMEM =
{
  0x40, Tx19, 0x01, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char GxEPD2_290_I6FD::lut_24_bb_partial[] PROGMEM =
{
  0x00, Tx19, 0x01, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void GxEPD2_290_I6FD::_Force_Init_Full()
{
  _Init_Full();
  _refresh_mode = forced_full_refresh;
}

void GxEPD2_290_I6FD::_Init_Full()
{
  _InitDisplay();
  _writeCommand(0x00); //panel setting
  _writeData(0x1f);    //LUT from OTP, 128x296
  _PowerOn();
  _refresh_mode = full_refresh;
}

void GxEPD2_290_I6FD::_Init_4G()
{
  _InitDisplay();
  _writeCommand(0x00); //panel setting
  _writeData(0xbf);    //LUT from register, 128x296
  _writeCommand(0x50);
  _writeData(0x17);    //WBmode:VBDF 17|D7 VBDW 97 VBDB 57   WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7
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
  _PowerOn();
  _refresh_mode = grey_refresh;
}

void GxEPD2_290_I6FD::_Init_Part()
{
  _InitDisplay();
  _writeCommand(0x00); //panel setting
  _writeData(0xbf);    //LUT from register, 128x296
  _writeCommand(0x82); //vcom_DC setting
  _writeData (0x08);
  _writeCommand(0x50);
  _writeData(0x17);    //WBmode:VBDF 17|D7 VBDW 97 VBDB 57   WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7
  _writeCommand(0x20);
  _writeDataPGM(lut_20_vcomDC_partial, sizeof(lut_20_vcomDC_partial));
  _writeCommand(0x21);
  _writeDataPGM(lut_21_ww_partial, sizeof(lut_21_ww_partial));
  _writeCommand(0x22);
  _writeDataPGM(lut_22_bw_partial, sizeof(lut_22_bw_partial));
  _writeCommand(0x23);
  _writeDataPGM(lut_23_wb_partial, sizeof(lut_23_wb_partial));
  _writeCommand(0x24);
  _writeDataPGM(lut_24_bb_partial, sizeof(lut_24_bb_partial));
  _PowerOn();
  _refresh_mode = fast_refresh;
}

void GxEPD2_290_I6FD::_Update_Full()
{
  _writeCommand(0x12); //display refresh
  _waitWhileBusy("_Update_Full", full_refresh_time);
}

void GxEPD2_290_I6FD::_Update_4G()
{
  _writeCommand(0x12); //display refresh
  _waitWhileBusy("_Update_4G", full_refresh_time);
}

void GxEPD2_290_I6FD::_Update_Part()
{
  _writeCommand(0x12); //display refresh
  _waitWhileBusy("_Update_Part", partial_refresh_time);
}
