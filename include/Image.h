#ifndef _IMAGE_H_
#define _IMAGE_H_

struct ImageData {
  uint16_t width;
  uint16_t height;
  uint16_t* data;
};

extern ImageData Image;

#endif 