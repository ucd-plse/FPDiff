#include <iostream>
#include <stdio.h>
#include <assert.h>
extern "C" {
#include <quadmath.h>
} 

using namespace std;


int main (int argc, char *argv[]) {
  assert(argc == 3);

  char *inname = argv[1];
  char *outname = argv[2];

  FILE *infile = fopen(inname, "r");
  FILE *outfile = fopen(outname, "w");

  fseek(infile, 0, SEEK_END);
  long fbytes = ftell(infile);
  fseek(infile, 0, SEEK_SET);
  assert(fbytes % sizeof(double) == 0);
  long fsize = fbytes / sizeof(double);

  for (long i = 0 ; i < fsize ; i++) {
    double fperr; 
    fread(&fperr, sizeof(double), 1, infile);
    fprintf(outfile, "%41.40Lf\n", (long double) fperr);
  }

  fclose(infile);
  fclose(outfile);

  return 0;
}
