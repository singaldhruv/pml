#ifndef _ENCODER_
#define _ENCODER_

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <ctime>
#include <new>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include "models/utils/datatypes.h"
#include "predictor.h"

using namespace std;

/* An Encoder does arithmetic encoding.  Methods:
   Encoder(COMPRESS, f) creates encoder for compression to archive f, which
     must be open for writing in binary mode
   Encoder(DECOMPRESS, f) creates encoder for decompression from archive f,
     which must be open for reading in binary mode
   encode(bit) in COMPRESS mode compresses bit to file f.
   encode() in DECOMPRESS mode returns the next decompressed bit from file f.
   print() prints compression statistics
*/

typedef enum {COMPRESS, DECOMPRESS} Mode;
class Encoder {
private:
  Predictor predictor;
  const Mode mode;       // Compress or decompress?
  FILE* archive;         // Compressed data file
  U32 x1, x2;            // Range, initially [0, 1), scaled by 2^32
  U32 x;                 // Last 4 input bytes of archive.
  int eofs;              // Number of EOF's read from data
  long xchars;           // Number of bytes of compressed data
  long encodes;          // Number of bits of uncompressed data
  int start_time;        // Clocks at start of compression
  long total_encodes;    // Sum of encodes
  int total_time;        // Sum of compression times
public:
  Encoder(Mode m, FILE* f);
  int encode(int bit=0);
  void print();
  ~Encoder();
};
#endif
