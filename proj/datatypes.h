#ifndef _DATATYPES_
#define _DATATYPES_
// 8-32 bit unsigned types, adjust as appropriate
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned long U32;

class U24 {  // 24-bit unsigned int
  U8 b0, b1, b2;  // Low, mid, high byte
public:
  explicit U24(int x=0): b0(x), b1(x>>8), b2(x>>16) {}
  operator int() const {return (((b2<<8)|b1)<<8)|b0;}
}; 

#endif
