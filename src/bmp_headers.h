#ifndef BMP_HEADERS
#define BMP_HEADERS
/*
 I have found that the following pragma is necessary to ensure
 the correct size of the structs.

 In the file, the BmpFileHeader struct is 14 bytes long and
 the BmpInfoHeader is 40 bytes long
 Works for GNU, Borland bcc32, Microsoft VC++

 */

#pragma pack(push, 1)
typedef struct {
  unsigned short type;      /* Two chars "BM" in little-endian order  */
  unsigned int size;        /*unsigned int 4 bytes File size        */
  unsigned short reserved1; // unsigned short 2 bytes
  unsigned short reserved2;
  unsigned int offset; /* Bytes from start of file to pixel data */
} BmpFileHeader;

typedef struct {
  unsigned int size;     /* Size of this header  (40 bytes)= DOWRD */
  int width;             /* Bitmap width in pixels                 */
  int height;            /* Bitmap height in pixels                */
  unsigned short planes; /* Must be 1                              */
  unsigned short bits;   /* Bits per pixel. (8 for 256-color bmps) */

  unsigned int compression; /* 0 means no compression                 */
  unsigned int imagesize;   /* Number of bytes in pixel data section  */
  int xresolution;          /* Not used by any of my functions        */
  int yresolution;          /* Not used by any of my functions        */

  unsigned int ncolors;         /* 0 means use all colors                 */
  unsigned int importantcolors; /* 0 means all are important              */

} BmpInfoHeader;

typedef struct /**** Colormap entry structure ****/
{
  unsigned char rgbBlue;     /* Blue value */
  unsigned char rgbGreen;    /* Green value */
  unsigned char rgbRed;      /* Red value */
  unsigned char rgbReserved; /* Reserved */
} RGBQUAD;

#pragma pack(pop)

#endif