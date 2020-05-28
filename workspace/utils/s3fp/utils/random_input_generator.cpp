#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
extern "C" {
#include <quadmath.h>
}

#define FSCALE_M0 100000000
#define FLOW_M0 -1.0
#define FHIGH_M0 1.0


double rand128_method_0 (int input_bitwidth) {
  assert(FHIGH_M0 >= FLOW_M0);
  double randf = ((double)(rand() % FSCALE_M0) / (double)FSCALE_M0) * ((double)FHIGH_M0 - (double)FLOW_M0) + (double)FLOW_M0;
  if (input_bitwidth == 32) {
    float idata = (float) randf;
    return idata;
  }
  else if (input_bitwidth == 64) {
    double idata = (double) randf;
    return idata;
  }
  else if (input_bitwidth == 128) {
    return randf;
  }
  else assert(false);
}


int main (int argc, char *argv[]) {
  assert(argc == 5); 
  
  char *iname = argv[1];
  int input_size = atoi(argv[2]);
  int input_bitwidth = atoi(argv[3]);
  int rseed = atoi(argv[4]);
  assert(input_bitwidth == 32 || 
	 input_bitwidth == 64 || 
	 input_bitwidth == 128); 
  
  
  srand(rseed);
  FILE *ifile = fopen(iname, "w");
  assert(ifile != NULL);

  for (int i = 0 ; i < input_size ; i++) {
    double fdata = rand128_method_0(input_bitwidth); 
    fwrite(&fdata, sizeof(double), 1, ifile);
  }

  fclose(ifile);
  
  return 0;
}
