# GxEPD2_4G
## Arduino Display Library for SPI E-Paper Displays

- GxEPD2 variant with support for 4 grey levels on supported e-papers

- With full Graphics and Text support using Adafruit_GFX

- For SPI e-paper displays from Dalian Good Display 
- and SPI e-paper boards from Waveshare

### note on grey level support
- 4 grey levels are available on some e-papers that support differential refresh
- 4 grey level support can be used instead of differential refresh
- "old data" and "new data" controller buffers provides 2 bits per pixel
- the 4 states per pixel can produce 4 grey levels using a special waveform
- the 4 grey level waveform table is taken from demo from Good Display
- the 4 grey levels may differ from display to display, and may be temperature dependent
- ghosting can't be avoided; using clearScreen() helps

### important note :
- the display panels are for 3.3V supply and 3.3V data lines
- never connect data lines directly to 5V Arduino data pins, use e.g. 4k7/10k resistor divider
- series resistor only is not enough for reliable operation (back-feed effect through protection diodes)
- 4k7/10k resistor divider may not work with flat cable extensions or Waveshare 4.2 board, use level converter then
- do not forget to connect GND
- the actual Waveshare display boards now have level converters and series regulator, safe for 5V
- use 4k7 pull-down on SS for ESP8266 for boards with level converters

### Paged Drawing, Picture Loop
 - This library uses paged drawing to limit RAM use and cope with missing single pixel update support
 - buffer size can be selected in the application by template parameter page_height, see GxEPD2_Example
 - Paged drawing is implemented as picture loop, like in U8G2 (Oliver Kraus)
 - see https://github.com/olikraus/u8glib/wiki/tpictureloop
 - Paged drawing is also available using drawPaged() and drawCallback(), like in GxEPD
- ` // GxEPD style paged drawing; drawCallback() is called as many times as needed `
- ` void drawPaged(void (*drawCallback)(const void*), const void* pv) `
- paged drawing is done using Adafruit_GFX methods inside picture loop or drawCallback

### Full Screen Buffer Support
 - full screen buffer is selected by setting template parameter page_height to display height
 - drawing to full screen buffer is done using Adafruit_GFX methods without picture loop or drawCallback
 - and then calling method display()

### Low Level Bitmap Drawing Support
 - bitmap drawing support to the controller memory and screen is available:
 - either through the template class instance methods that forward calls to the base display class
 - or directly using an instance of a base display class and calling its methods directly

### Supporting Arduino Forum Topics:

- Waveshare e-paper displays with SPI: http://forum.arduino.cc/index.php?topic=487007.0
- Good Dispay ePaper for Arduino : https://forum.arduino.cc/index.php?topic=436411.0

### Note on documentation
- GxEPD2 uses Adafruit_GFX for Graphics and Text support, which is well documented there
- GxEPD2 uses meaningful method names, and has some comments in the header files
- consult the header files GxEPD2_BW.h, GxEPD2_4G.h and GxEPD2_GFX.h
- for the concept of paged drawing and picture loop see: 
- https://github.com/olikraus/u8glib/wiki/tpictureloop

### Supported SPI e-paper panels from Good Display:
- GDEW0213I5F    2.13" b/w flexible
- GDEW029T5      2.9" b/w
- GDEW027W3      2.7" b/w
- GDEW0371W7     3.7" b/w
- GDEW042T2      4.2" b/w
- GDEW075T7      7.5" b/w 800x480

### Version 1.0.2
- added method drawGreyPixmap() to buffered graphics
- void drawGreyPixmap(const uint8_t pixmap[], int16_t depth, int16_t x, int16_t y, int16_t w, int16_t h);
#### Version 1.0.1
- added support for GDEW075T7
- GDEW075T7 uses same wavetable as the other ones, no "official" (demo) yet
- GDEW075T7 grey level behaviour is less good than e.g. on GDEW0371W7
- some fixes and improvements
#### Version 1.0.0
- initial version for 4 grey level support, experimental
- (slow) partial refresh doesn't work with 4G on some panels
