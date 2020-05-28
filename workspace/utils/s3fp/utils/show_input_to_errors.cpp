#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
extern "C" {
#include <quadmath.h>
} 

#define HFP_TYPE double 

using namespace std;


int main (int argc, char *argv[]) {
  assert(argc == 2);
  char *fname = argv[1];
  FILE *infile = fopen(fname, "r");

  unsigned int input_bitwidth; 
  unsigned int n_inputs;
  unsigned int error_bitwidth; 
  unsigned int n_errors;
  unsigned int ilen, elen;

  fseek(infile, 0, SEEK_END);
  unsigned long len_total = ftell(infile);
  fseek(infile, 0, SEEK_SET);
  assert(len_total >= (sizeof(unsigned int) * 4));
  fread(&input_bitwidth, sizeof(unsigned int), 1, infile);
  fread(&n_inputs, sizeof(unsigned int), 1, infile);
  fread(&error_bitwidth, sizeof(unsigned int), 1, infile);
  fread(&n_errors, sizeof(unsigned int), 1, infile);
  assert((input_bitwidth % 8) == 0 && (error_bitwidth % 8) == 0);  
  assert(input_bitwidth == 32 || input_bitwidth == 64 || input_bitwidth == 128); 
  assert(error_bitwidth == 32 || error_bitwidth == 64 || error_bitwidth == 128); 
  ilen = input_bitwidth / 8;
  elen = error_bitwidth / 8;
  unsigned long len_inputs = ilen * n_inputs;
  unsigned long len_errors = elen * n_errors;

  unsigned long len_data = len_total - (sizeof(unsigned int) * 4);
  assert(len_data % (len_inputs + len_errors) == 0);
  unsigned long n_sections = len_data / (len_inputs + len_errors);

  cout << "input bitwidth : " << input_bitwidth << endl;
  cout << "# input variables per case : " << n_inputs << endl;
  cout << "error bitwidth : " << error_bitwidth << endl;
  cout << "# error numbers per case : " << n_errors << endl;
  for (unsigned long si = 0 ; si < n_sections ; si++) {
    cout << "---- input => errors (" << si << ") ----" << endl;
    for (unsigned long ii = 0 ; ii < n_inputs ; ii++) {
      if (input_bitwidth == 32) {
	float idata;
	fread(&idata, sizeof(float), 1, infile);
	cout << "input[" << ii << "] = " << idata << endl;
      }
      else if (input_bitwidth == 64) {
	double idata; 
	fread(&idata, sizeof(double), 1, infile);
	cout << "input[" << ii << "] = " << idata << endl;
      }
      else if (input_bitwidth == 128) {
	HFP_TYPE idata;
	fread(&idata, sizeof(HFP_TYPE), 1, infile);
	cout << "input[" << ii << "] = " << (long double) idata << endl;
      }
      else assert(false);
    }
    for (unsigned long ei = 0 ; ei < n_errors ; ei++) {
      if (error_bitwidth == 32) {
	float edata; 
	fread(&edata, sizeof(float), 1, infile);
	cout << "error[" << ei << "] = " << edata << endl;
      }
      else if (error_bitwidth == 64) {
	double edata; 
	fread(&edata, sizeof(double), 1, infile);
	cout << "error[" << ei << "] = " << edata << endl;
      }
      else if (error_bitwidth == 128) {
	HFP_TYPE edata; 
	fread(&edata, sizeof(HFP_TYPE), 1, infile);
	cout << "error[" << ei << "] = " << (long double) edata << endl;
      }
      else assert(false);
    }
  }

  fclose(infile); 

  return 0;
}
