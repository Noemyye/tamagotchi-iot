#ifndef _ADAFRUIT_SSD1351_H_
#define _ADAFRUIT_SSD1351_H_

#include <Adafruit_GFX.h>
#include <SPI.h>

// Screen dimensions
#define SSD1351_WIDTH  128
#define SSD1351_HEIGHT 128

// Commands
#define SSD1351_CMD_SETCOLUMN     0x15
#define SSD1351_CMD_SETROW        0x75
#define SSD1351_CMD_WRITERAM      0x5C
#define SSD1351_CMD_READRAM       0x5D
#define SSD1351_CMD_SETREMAP      0xA0
#define SSD1351_CMD_STARTLINE     0xA1
#define SSD1351_CMD_DISPLAYOFFSET 0xA2
#define SSD1351_CMD_NORMALDISPLAY 0xA6
#define SSD1351_CMD_INVERTDISPLAY 0xA7
#define SSD1351_CMD_FUNCTIONSELECT 0xAB
#define SSD1351_CMD_DISPLAYOFF    0xAE
#define SSD1351_CMD_DISPLAYON     0xAF
#define SSD1351_CMD_PRECHARGE     0xB1
#define SSD1351_CMD_DISPLAYENHANCE 0xB2
#define SSD1351_CMD_CLOCKDIV      0xB3
#define SSD1351_CMD_SETVSL        0xB4
#define SSD1351_CMD_SETGPIO       0xB5
#define SSD1351_CMD_PRECHARGE2    0xB6
#define SSD1351_CMD_SETGRAY       0xB8
#define SSD1351_CMD_USELUT        0xB9
#define SSD1351_CMD_PRECHARGELEVEL 0xBB
#define SSD1351_CMD_VCOMH         0xBE
#define SSD1351_CMD_CONTRASTABC   0xC1
#define SSD1351_CMD_CONTRASTMASTER 0xC7
#define SSD1351_CMD_MUXRATIO      0xCA
#define SSD1351_CMD_COMMANDLOCK   0xFD
#define SSD1351_CMD_HORIZSCROLL   0x96
#define SSD1351_CMD_STOPSCROLL    0x9E
#define SSD1351_CMD_STARTSCROLL   0x9F

class Adafruit_SSD1351 : public Adafruit_GFX {
 public:
  Adafruit_SSD1351(int16_t w, int16_t h, int8_t cs_pin, int8_t dc_pin, int8_t mosi_pin, int8_t sclk_pin, int8_t rst_pin);
  Adafruit_SSD1351(int16_t w, int16_t h, int8_t cs_pin, int8_t dc_pin, int8_t rst_pin);
  
  bool begin(void);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void fillScreen(uint16_t color);
  void updateScreen(void);
  
  void setTextSize(uint8_t s);
  void setTextColor(uint16_t c);
  void setTextColor(uint16_t c, uint16_t bg);
  void setCursor(int16_t x, int16_t y);
  void print(const char* str);
  void print(int n);
  void print(unsigned int n);
  void print(long n);
  void print(unsigned long n);
  void print(double n);
  void println(const char* str);
  void println(int n);
  void println(unsigned int n);
  void println(long n);
  void println(unsigned long n);
  void println(double n);
  void println(void);
  
  void setTextWrap(bool w);
  
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  
  void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h);
  
 private:
  int8_t _cs, _dc, _rst, _mosi, _sclk;
  bool _hwSPI;
  
  void writeCommand(uint8_t c);
  void writeData(uint8_t c);
  void writeData16(uint16_t c);
};

#endif 