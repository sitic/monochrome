#include "bmp.h"

Bmp::Bmp(BmpFileHeader &myFH, BmpInfoHeader &myIH, const int &width,
         const int &height, const unsigned short &bpp, const unsigned int &arg)
    : FH(myFH), IH(myIH) {
  imageArray = new unsigned char **[height];
  for (int i = 0; i < height; ++i) {
    imageArray[i] = new unsigned char *[width];
    for (int j = 0; j < width; ++j) {
      imageArray[i][j] = new unsigned char[bpp / 8];
      for (int k = 0; k < bpp / 8; ++k)
        imageArray[i][j][k] = 0;
    }
  }
  pallete = new RGBQUAD[arg];
  for (int i = 0; i < arg; ++i) {
    pallete[i].rgbBlue = arg;
    pallete[i].rgbGreen = arg;
    pallete[i].rgbRed = arg;
    pallete[i].rgbReserved = arg;
  }
}
Bmp::Bmp() {}

Bmp::~Bmp() {

  for (int i = 0; i < 300; i++) {
    for (int j = 0; j < 620; j++) {
      // delete [] imageArray[i][j];
    }
    // delete [] imageArray[i];
  }
  delete[] imageArray;

  delete[] pallete;
}

ostream &operator<<(ostream &output, Bmp &arg) {

  return output << hex << "Bmp File Type:\t\t\t" << arg.FH.type << endl
                << dec

                << "Image Width:\t\t\t" << arg.IH.width << "\tpixel" << endl
                << "Image Height:\t\t\t" << arg.IH.height << "\tpixel" << endl;
  //<<"Mode:\t\t\t\t"<<arg.IH.bits<< "\tbits per pixel" << endl
  //<<"Bmp Size:\t\t\t"<<arg.FH.size<<endl
  //<<"Bmp Reserved one:\t\t"<<arg.FH.reserved1<<endl
  //<<"Bmp Reserved two:\t\t"<<arg.FH.reserved1<<endl
  //<<"Bmp Header Offset:\t\t"<<arg.FH.offset<<endl //Offset in bytes meaning
  //that after byte 134 the actual pixels can be found
  //<<"Bmp Header Size:\t\t"<<arg.IH.size<<endl
  //<<"Image Compression:\t\t"<<arg.IH.compression<<endl
  //<<"RGBA Data Section Size:\t\t"<<arg.IH.imagesize<< "\tbytes" << endl
  //<<"Number of colours:\t\t"<<arg.IH.ncolors<<endl;
}
const int Bmp::getHeight() const { return IH.height; }
const int Bmp::getWidth() const { return IH.width; }
const unsigned short Bmp::getBpp() const { return IH.bits; }
void Bmp::loadArrayPtr(unsigned char ***myArray, int height, int width,
                       int bpp) {
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      for (int k = 0; k < bpp; ++k) {
        imageArray[i][j][k] = myArray[i][j][k];
      }
    }
  }
}

void Bmp::emptyArrayPtr(unsigned char ***myArray, int height, int width,
                        int bpp) {
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      delete[] imageArray[i][j];
    }
    delete[] imageArray[i];
  }
  delete[] imageArray;
}

void Bmp::loadPallete(RGBQUAD *myPallete, const unsigned int &limit) {
  for (int i = 0; i < limit; ++i) {
    pallete[i] = myPallete[i];
  }
}
BmpFileHeader Bmp::getBMPFH() { return FH; }
BmpInfoHeader Bmp::getBMPIH() { return IH; }
void Bmp::setOffset(int arg) { FH.offset = arg; }
unsigned char Bmp::getImageArray(int height, int width, int bpp) {
  return imageArray[height][width][bpp];
}
RGBQUAD Bmp::getPallete(int arg) { return pallete[arg]; }
