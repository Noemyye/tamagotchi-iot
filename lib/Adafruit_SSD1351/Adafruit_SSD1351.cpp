#include "Adafruit_SSD1351.h"

Adafruit_SSD1351::Adafruit_SSD1351(int16_t w, int16_t h, int8_t cs_pin, int8_t dc_pin, int8_t mosi_pin, int8_t sclk_pin, int8_t rst_pin) : Adafruit_GFX(w, h) {
  _cs = cs_pin;
  _dc = dc_pin;
  _rst = rst_pin;
  _mosi = mosi_pin;
  _sclk = sclk_pin;
  _hwSPI = false;
}

Adafruit_SSD1351::Adafruit_SSD1351(int16_t w, int16_t h, int8_t cs_pin, int8_t dc_pin, int8_t rst_pin) : Adafruit_GFX(w, h) {
  _cs = cs_pin;
  _dc = dc_pin;
  _rst = rst_pin;
  _hwSPI = true;
}

bool Adafruit_SSD1351::begin(void) {
  pinMode(_dc, OUTPUT);
  pinMode(_cs, OUTPUT);
  pinMode(_rst, OUTPUT);
  
  if (!_hwSPI) {
    pinMode(_mosi, OUTPUT);
    pinMode(_sclk, OUTPUT);
  }
  
  digitalWrite(_cs, HIGH);
  
  // Reset display
  digitalWrite(_rst, HIGH);
  delay(100);
  digitalWrite(_rst, LOW);
  delay(100);
  digitalWrite(_rst, HIGH);
  delay(100);
  
  // Initialize display
  writeCommand(0xFD); // Command lock
  writeData(0x12);
  
  writeCommand(0xAE); // Display off
  
  writeCommand(0xA0); // Set re-map
  writeData(0x74);
  
  writeCommand(0xA1); // Set display start line
  writeData(0x00);
  
  writeCommand(0xA2); // Set display offset
  writeData(0x00);
  
  writeCommand(0xA4); // Set normal display
  
  writeCommand(0xA8); // Set multiplex ratio
  writeData(0x7F);
  
  writeCommand(0xB1); // Set phase length
  writeData(0x32);
  
  writeCommand(0xB2); // Set clock div
  writeData(0x14);
  
  writeCommand(0xBE); // Set VCOMH
  writeData(0x05);
  
  writeCommand(0xC1); // Set contrast A
  writeData(0xC8);
  
  writeCommand(0xC2); // Set contrast B
  writeData(0x80);
  
  writeCommand(0xC3); // Set contrast C
  writeData(0xC8);
  
  writeCommand(0xC7); // Set master contrast
  writeData(0x0F);
  
  writeCommand(0xCA); // Set VSL
  writeData(0x03);
  
  writeCommand(0xE0); // Set gamma
  writeData(0x02);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x02);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  
  writeCommand(0xE1); // Set gamma
  writeData(0x02);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x02);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  writeData(0x06);
  
  writeCommand(0xAF); // Display on
  
  fillScreen(BLACK);
  
  return true;
}

void Adafruit_SSD1351::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) return;
  
  writeCommand(0x15); // Set column address
  writeData(x);
  writeData(127);
  
  writeCommand(0x75); // Set row address
  writeData(y);
  writeData(127);
  
  writeCommand(0x5C); // Write RAM
  
  writeData16(color);
}

void Adafruit_SSD1351::fillScreen(uint16_t color) {
  fillRect(0, 0, width(), height(), color);
}

void Adafruit_SSD1351::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  for (int16_t i = x; i < x + w; i++) {
    for (int16_t j = y; j < y + h; j++) {
      drawPixel(i, j, color);
    }
  }
}

void Adafruit_SSD1351::updateScreen(void) {
  // Nothing to do here
}

void Adafruit_SSD1351::setTextSize(uint8_t s) {
  textsize = s;
}

void Adafruit_SSD1351::setTextColor(uint16_t c) {
  textcolor = c;
  textbgcolor = c;
}

void Adafruit_SSD1351::setTextColor(uint16_t c, uint16_t bg) {
  textcolor = c;
  textbgcolor = bg;
}

void Adafruit_SSD1351::setCursor(int16_t x, int16_t y) {
  cursor_x = x;
  cursor_y = y;
}

void Adafruit_SSD1351::print(const char* str) {
  while (*str) {
    write(*str++);
  }
}

void Adafruit_SSD1351::print(int n) {
  char buf[8];
  sprintf(buf, "%d", n);
  print(buf);
}

void Adafruit_SSD1351::print(unsigned int n) {
  char buf[8];
  sprintf(buf, "%u", n);
  print(buf);
}

void Adafruit_SSD1351::print(long n) {
  char buf[8];
  sprintf(buf, "%ld", n);
  print(buf);
}

void Adafruit_SSD1351::print(unsigned long n) {
  char buf[8];
  sprintf(buf, "%lu", n);
  print(buf);
}

void Adafruit_SSD1351::print(double n) {
  char buf[8];
  sprintf(buf, "%f", n);
  print(buf);
}

void Adafruit_SSD1351::println(const char* str) {
  print(str);
  cursor_y += textsize * 8;
  cursor_x = 0;
}

void Adafruit_SSD1351::println(int n) {
  print(n);
  cursor_y += textsize * 8;
  cursor_x = 0;
}

void Adafruit_SSD1351::println(unsigned int n) {
  print(n);
  cursor_y += textsize * 8;
  cursor_x = 0;
}

void Adafruit_SSD1351::println(long n) {
  print(n);
  cursor_y += textsize * 8;
  cursor_x = 0;
}

void Adafruit_SSD1351::println(unsigned long n) {
  print(n);
  cursor_y += textsize * 8;
  cursor_x = 0;
}

void Adafruit_SSD1351::println(double n) {
  print(n);
  cursor_y += textsize * 8;
  cursor_x = 0;
}

void Adafruit_SSD1351::println(void) {
  cursor_y += textsize * 8;
  cursor_x = 0;
}

void Adafruit_SSD1351::setTextWrap(bool w) {
  wrap = w;
}

void Adafruit_SSD1351::drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h) {
  for (int16_t i = 0; i < w; i++) {
    for (int16_t j = 0; j < h; j++) {
      drawPixel(x + i, y + j, bitmap[j * w + i]);
    }
  }
}

void Adafruit_SSD1351::writeCommand(uint8_t c) {
  digitalWrite(_dc, LOW);
  digitalWrite(_cs, LOW);
  
  if (_hwSPI) {
    SPI.transfer(c);
  } else {
    shiftOut(_mosi, _sclk, MSBFIRST, c);
  }
  
  digitalWrite(_cs, HIGH);
}

void Adafruit_SSD1351::writeData(uint8_t c) {
  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  
  if (_hwSPI) {
    SPI.transfer(c);
  } else {
    shiftOut(_mosi, _sclk, MSBFIRST, c);
  }
  
  digitalWrite(_cs, HIGH);
}

void Adafruit_SSD1351::writeData16(uint16_t c) {
  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  
  if (_hwSPI) {
    SPI.transfer16(c);
  } else {
    shiftOut(_mosi, _sclk, MSBFIRST, c >> 8);
    shiftOut(_mosi, _sclk, MSBFIRST, c & 0xFF);
  }
  
  digitalWrite(_cs, HIGH);
} 