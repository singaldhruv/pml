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

#include "encoder.h"

using namespace std;

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
      "To compress:         ./paqlike archive filenames...  (archive will be created)\n"
      "To extract/compare:  ./paqlike archive  (does not clobber existing files)\n"
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
      printf("Archive file %s not in paqlike format\n", argv[1]);
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

