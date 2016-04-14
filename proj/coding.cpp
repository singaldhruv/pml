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
using namespace std;

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


/* A Predictor predicts the next bit given the bits so far using a
collection of models.  Methods:

   p() returns probability of a 1 being the next bit, P(y = 1)
     as a 16 bit number (0 to 64K-1).
   update(y) updates the models with bit y (0 or 1)
*/

class Predictor {

public:
  U16 p() const {
    int n0=1, n1=n0;
//    m1.predict(n0, n1);
//    m2.predict(n0, n1);
//    m3.predict(n0, n1);
//    m4.predict(n0, n1);
    return U16(65535.0*n1/(n0+n1));
  }
  void update(int y) {
//    m1.update(y);
//    m2.update(y);
//    m3.update(y);
//    m4.update(y);
  }
};

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

// Read one byte from encoder e
int decompress(Encoder& e) {  // Decompress 8 bits, MSB first
  int c=0;
  for (int i=0; i<8; ++i)  
    c=(c<<1)+e.encode();
  return c;
}

// Write one byte c to encoder e
void compress(Encoder& e, int c) {
  for (int i=0; i<8; ++i) {  // Compress 8 bits, MSB first
    e.encode((c&128)?1:0);
    c<<=1;
  }
}

// Fail if out of memory
void handler() {
  printf("Out of memory\n");
  exit(1);
}

// Read and return a line of input from FILE f (default stdin) up to
// first control character.  Skips CR in CR LF.
string getline(FILE* f=stdin) {
  int c;
  string result="";
  while ((c=getc(f))!=EOF && c>=32)
    result+=char(c);
  if (c=='\r')
    (void) getc(f);
  return result;
}

// User interface
int main(int argc, char** argv) {
  clock();
  set_new_handler(handler);

  // Check arguments
  if (argc<2) {
    printf(
      "PAQ1 file compressor/archiver, (C) 2002, Matt Mahoney, mmahoney@cs.fit.edu\n"
      "This program is free software distributed without warranty under the terms\n"
      "of the GNU General Public License, see http://www.gnu.org/licenses/gpl.txt\n"
      "\n"
      "To compress:         PAQ1 archive filenames...  (archive will be created)\n"
      "  or (MSDOS):        dir/b | PAQ1 archive  (reads file names from input)\n"
      "To extract/compare:  PAQ1 archive  (does not clobber existing files)\n"
      "To view contents:    more < archive\n");
    return 1;
  }

  // File names and sizes from input or archive
  vector<string> filename; // List of names
  vector<long> filesize;   // Size or -1 if error

  // Extract files
  FILE* archive=fopen(argv[1], "rb");
  if (archive) {
    if (argc>2) {
      printf("File %s already exists\n", argv[1]);
      return 1;
    }
    printf("Extracting archive %s ...\n", argv[1]);

    // Read "PAQ1\r\n" at start of archive
    if (getline(archive) != "PAQ1") {
      printf("Archive file %s not in PAQ1 format\n", argv[1]);
      return 1;
    }

    // Read "size filename" in "%10d %s\r\n" format
    while (true) {
      string s=getline(archive);
      if (s.size()>10) {
        filesize.push_back(atol(s.c_str()));
        filename.push_back(s.substr(11));
      }
      else
        break;
    }

    // Test end of header for "\f\0"
    {
      int c1=0, c2=0;
      if ((c1=getc(archive))!='\f' || (c2=getc(archive))!=0) {
        printf("%s: Bad PAQ1 header format %d %d\n", argv[1],
          c1, c2);
        return 1;
      }
    }

    // Extract files from archive data
    Encoder e(DECOMPRESS, archive);
    for (int i=0; i<int(filename.size()); ++i) {
      printf("%10ld %s: ", filesize[i], filename[i].c_str());

      // Compare with existing file
      FILE* f=fopen(filename[i].c_str(), "rb");
      const long size=filesize[i];
      if (f) {
        bool different=false;
        for (long j=0; j<size; ++j) {
          int c1=decompress(e);
          int c2=getc(f);
          if (!different && c1!=c2) {
            printf("differ at offset %ld, archive=%d file=%d\n",
              j, c1, c2);
            different=true;
          }
        }
        if (!different)
          printf("identical\n");
        fclose(f);
      }

      // Extract to new file
      else {
        f=fopen(filename[i].c_str(), "wb");
        if (!f)
          printf("cannot create, skipping...\n");
        for (long j=0; j<size; ++j) {
          int c=decompress(e);
          if (f)
            putc(c, f);
        }
        if (f) {
          printf("extracted\n");
          fclose(f);
        }
      }
    }
  }

  // Compress files
  else {

    // Read file names from command line or input
    if (argc>2)
      for (int i=2; i<argc; ++i)
        filename.push_back(argv[i]);
    else {
      printf(
        "Enter names of files to compress, followed by blank line or EOF.\n");
      while (true) {
        string s=getline(stdin);
        if (s.size()==0 || s=="")
          break;
        else
          filename.push_back(s);
      }
    }

    // Get file sizes
    for (int i=0; i<int(filename.size()); ++i) {
      FILE* f=fopen(filename[i].c_str(), "rb");
      if (!f) {
        printf("File not found, skipping: %s\n",
          filename[i].c_str());
        filesize.push_back(-1);
      }
      else {
        fseek(f, 0L, SEEK_END);
        filesize.push_back(ftell(f));
        fclose(f);
      }
    }
    if (filesize.size()==0
        || *max_element(filesize.begin(), filesize.end())<0) {
      printf("No files to compress, no archive created.\n");
      return 1;
    }

    // Write header
    archive=fopen(argv[1], "wb");
    if (!archive) {
      printf("Cannot create archive: %s\n", argv[1]);
      return 1;
    }
    fprintf(archive, "PAQ1\r\n");
    for (int i=0; i<int(filename.size()); ++i) {
      if (filesize[i]>=0)
        fprintf(archive, "%10ld %s\r\n", filesize[i], filename[i].c_str());
    }
    putc(032, archive);
    putc('\f', archive);
    putc(0, archive);

    // Write data
    Encoder e(COMPRESS, archive);
    for (int i=0; i<int(filename.size()); ++i) {
      const int size=filesize[i];
      if (size>=0) {
        printf("%s: ", filename[i].c_str());
        FILE* f=fopen(filename[i].c_str(), "rb");
        int c;
        for (long j=0; j<size; ++j) {
          if (f)
            c=getc(f);
          else
            c=0;
          compress(e, c);
        }
        if (f) {
          fclose(f);
          e.print();
        }
      }
    }
  }
  return 0;
}

