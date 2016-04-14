#include "encoder.h"

// Constructor
Encoder::Encoder(Mode m, FILE* f): predictor(), mode(m), archive(f), x1(0),
    x2(0xffffffff), x(0), eofs(0), xchars(0), encodes(0), start_time(0),
    total_encodes(0), total_time(0) {
  start_time=clock();

  // In DECOMPRESS mode, initialize x to the first 4 bytes of the archive
  if (mode==DECOMPRESS) {
    for (int i=0; i<4; ++i) {
      int c=getc(archive);
      if (c==EOF) {
        c=0;
        ++eofs;
      }
      else
        ++xchars;
      x=(x<<8)+c;
    }
  }
}

/* encode(bit) -- Split the range [x1, x2] at x in proportion to predictor
   P(y = 1).  In COMPRESS mode, make the lower or upper subrange
   the new range according to y.  In DECOMPRESS mode, return 0 or 1
   according to which subrange x is in, and make this the new range.

   Maintain x1 <= x <= x2 as the last 4 bytes of compressed data:
   In COMPRESS mode, write the leading bytes of x2 that match x1.
   In DECOMPRESS mode, shift out these bytes and shift in an equal
   number of bytes into x from the archive.
*/
int Encoder::encode(int y) {
  ++encodes;

  // Split the range
  U32 p=65535-predictor.p(); // Probability P(0) * 64K rounded down
  const U32 xdiff=x2-x1;
  U32 xmid=x1;  // = x1+p*(x2-x1) multiply without overflow, round down
  if (xdiff>=0x10000000) xmid+=(xdiff>>16)*p;
  else if (xdiff>=0x1000000) xmid+=((xdiff>>12)*p)>>4;
  else if (xdiff>=0x100000) xmid+=((xdiff>>8)*p)>>8;
  else if (xdiff>=0x10000) xmid+=((xdiff>>4)*p)>>12;
  else xmid+=(xdiff*p)>>16;

  // Update the range
  if (mode==COMPRESS) {
    if (y)
      x1=xmid+1;
    else
      x2=xmid;
  }
  else {
    if (x<=xmid) {
      y=0;
      x2=xmid;
    }
    else {
      y=1;
      x1=xmid+1;
    }
  }
  predictor.update(y);

  // Shift equal MSB's out
  while (((x1^x2)&0xff000000)==0) {
    if (mode==COMPRESS) {
      putc(x2>>24, archive);
      ++xchars;
    }
    x1<<=8;
    x2=(x2<<8)+255;
    if (mode==DECOMPRESS) {
      int c=getc(archive);
      if (c==EOF) {
        c=0;
        if (++eofs>5) {
          printf("Premature end of archive\n");
          print();
          exit(1);
        }
      }
      else
        ++xchars;
      x=(x<<8)+c;
    }
  }
  return y;
}

// Destructor
Encoder::~Encoder() {

  // In COMPRESS mode, write out the remaining bytes of x, x1 < x < x2
  if (mode==COMPRESS) {
    while (((x1^x2)&0xff000000)==0) {
      putc(x2>>24, archive);
      x1<<=8;
      x2=(x2<<8)+255;
    }
    putc(x2>>24, archive);  // First unequal byte
  }
  if (total_encodes>0) {
    long total_xchars=ftell(archive);
    printf("%ld/%ld = %6.4f bpc (%4.2f%%) in %1.2f sec\n",
      total_xchars, total_encodes/8,
      total_xchars*64.0/total_encodes, total_xchars*800.0/total_encodes,
      double(total_time)/CLOCKS_PER_SEC);
  }
}

// Print Encoder stats
void Encoder::print() {
  int now=clock();
  if (encodes>0)
    printf("%ld/%ld = %6.4f bpc (%4.2f%%) in %1.2f sec\n",
      xchars, encodes/8,
      xchars*64.0/encodes, xchars*800.0/encodes,
      double(now-start_time)/CLOCKS_PER_SEC);
  else
    printf("0 bytes\n");
  total_time+=now-start_time;
  start_time=now;
  total_encodes+=encodes;
  encodes=0;
  xchars=0;
}

