#pragma once
#ifndef BMP_H
#define BMP_H
#include "bmp_headers.h"
#include <iostream>

using namespace std;

class Bmp {
public:
  Bmp(BmpFileHeader &, BmpInfoHeader &, const int &, const int &,
      const unsigned short &, const unsigned int &);

  Bmp();
  ~Bmp();

  friend ostream &operator<<(ostream &, Bmp &);

  const int getWidth() const;
  const int getHeight() const;
  const unsigned short getBpp() const;
  unsigned char getImageArray(int, int, int);
  RGBQUAD getPallete(int);
  BmpFileHeader getBMPFH();
  BmpInfoHeader getBMPIH();
  void loadArrayPtr(unsigned char ***, int, int, int);
  void emptyArrayPtr(unsigned char ***, int, int, int);
  void loadPallete(RGBQUAD *, const unsigned int &);
  void setOffset(int);

private:
  BmpFileHeader FH;
  BmpInfoHeader IH;
  unsigned char ***imageArray;
  RGBQUAD *pallete;
};
#endif