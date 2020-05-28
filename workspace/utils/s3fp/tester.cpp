#include "FP_UTILS.h"
#include <iostream>

using namespace std; 


int main (void) {
  double fp_low = -2.0; 
  double fp_high = 2.0; 

  bool include_denormal = false;
  unsigned int bit_width = 32; 
  // unsigned int bit_width = 64; 
  int sign; 
  uint128_t mant;
  uint128_t expo; 
  DecomposeIEEEFP(bit_width, fp_low, sign, expo, mant);
  cout << sign << endl;
  cout << expo << endl;
  cout << mant << endl;
  cout << "--------" << endl;
  DecomposeIEEEFP(bit_width, fp_high, sign, expo, mant);;
  cout << sign << endl;
  cout << expo << endl;
  cout << mant << endl;
  cout << "--------" << endl;

  uint128_t n_fps = NFPSBetweenTwoIEEEFPS(bit_width, fp_low, fp_high, include_denormal); 
  cout << n_fps << endl;
  cout << "--------" << endl;

  int out_sign;
  uint128_t out_mant;
  uint128_t out_expo; 
  DecomposeIEEEFP(bit_width, fp_low, sign, expo, mant);

  cout << ">>> 1" << endl;
  ShiftIEEEFP (bit_width, 
	       sign, expo, mant, 
	       0, 
	       1, 
	       include_denormal, 
	       out_sign, out_expo, out_mant);
  cout << out_sign << endl;
  cout << out_expo << endl;
  cout << out_mant << endl;
  sign = out_sign;
  expo = out_expo;
  mant = out_mant; 
  
  cout << ">>> 2" << endl;
  ShiftIEEEFP (bit_width, 
	       sign, expo, mant, 
	       0, 
	       1, 
	       include_denormal, 
	       out_sign, out_expo, out_mant);
  cout << out_sign << endl;
  cout << out_expo << endl;
  cout << out_mant << endl;
  sign = out_sign;
  expo = out_expo;
  mant = out_mant; 
  
  cout << ">>> 3" << endl;
  ShiftIEEEFP (bit_width, 
	       sign, expo, mant, 
	       1, 
	       1, 
	       include_denormal, 
	       out_sign, out_expo, out_mant);
  cout << out_sign << endl;
  cout << out_expo << endl;
  cout << out_mant << endl;
  sign = out_sign;
  expo = out_expo;
  mant = out_mant; 
  
  cout << ">>> 4" << endl;
  ShiftIEEEFP (bit_width, 
	       sign, expo, mant, 
	       0, 
	       1065353216, 
	       // 4607182418800017408, 
	       include_denormal, 
	       out_sign, out_expo, out_mant);
  cout << out_sign << endl;
  cout << out_expo << endl;
  cout << out_mant << endl;
  sign = out_sign;
  expo = out_expo;
  mant = out_mant; 
  
  cout << ">>> 5" << endl;
  ShiftIEEEFP (bit_width, 
	       sign, expo, mant, 
	       0, 
	       1, 
	       // 8388608, 
	       // 4503599627370496, 
	       include_denormal, 
	       out_sign, out_expo, out_mant);
  cout << out_sign << endl;
  cout << out_expo << endl;
  cout << out_mant << endl;
  sign = out_sign;
  expo = out_expo;
  mant = out_mant; 

  cout << ">>> 6" << endl;
  ShiftIEEEFP (bit_width, 
	       sign, expo, mant, 
	       1, 
	       1, 
	       include_denormal, 
	       out_sign, out_expo, out_mant);
  cout << out_sign << endl;
  cout << out_expo << endl;
  cout << out_mant << endl;
  sign = out_sign;
  expo = out_expo;
  mant = out_mant; 
  
  cout << ">>> 7" << endl;
  ShiftIEEEFP (bit_width, 
	       sign, expo, mant, 
	       1, 
	       10, 
	       include_denormal, 
	       out_sign, out_expo, out_mant);
  cout << out_sign << endl;
  cout << out_expo << endl;
  cout << out_mant << endl;
  sign = out_sign;
  expo = out_expo;
  mant = out_mant; 
  
  cout << ">>> 8" << endl;
  ShiftIEEEFP (bit_width, 
	       sign, expo, mant, 
	       0, 
	       10, 
	       include_denormal, 
	       out_sign, out_expo, out_mant);
  cout << out_sign << endl;
  cout << out_expo << endl;
  cout << out_mant << endl;
  sign = out_sign;
  expo = out_expo;
  mant = out_mant; 
  
  cout << ">>> 9" << endl;
  ShiftIEEEFP (bit_width, 
	       sign, expo, mant, 
	       0, 
	       2, 
	       include_denormal, 
	       out_sign, out_expo, out_mant);
  cout << out_sign << endl;
  cout << out_expo << endl;
  cout << out_mant << endl;
  sign = out_sign;
  expo = out_expo;
  mant = out_mant; 
  
  cout << ">>> 10" << endl;
  ShiftIEEEFP (bit_width, 
	       sign, expo, mant, 
	       0,
	       8388608, 	       
	       // 1073741823, 
	       // 4611686018427387903, 
	       include_denormal, 
	       out_sign, out_expo, out_mant);
  cout << out_sign << endl;
  cout << out_expo << endl;
  cout << out_mant << endl;
  sign = out_sign;
  expo = out_expo;
  mant = out_mant; 

  // -------- --------

  double ranfp; 

  SetSeedRandomEngine(0); 

  ranfp = FairRandomIEEEFP (bit_width, 
			    (double) fp_low, 
			    (double) fp_high, 
			    include_denormal); 
  cout << "random: " << ranfp << endl;

  ranfp = FairRandomIEEEFP (bit_width, 
			    (double) fp_low, 
			    (double) fp_high, 
			    include_denormal); 
  cout << "random: " << ranfp << endl;

  ranfp = FairRandomIEEEFP (bit_width, 
			    (double) fp_low, 
			    (double) fp_high, 
			    include_denormal); 
  cout << "random: " << ranfp << endl;

  cout << "--------" << endl;
  
  SetSeedRandomEngine(1000);

  ranfp = FairRandomIEEEFP (bit_width, 
			    (double) fp_low, 
			    (double) fp_high, 
			    include_denormal); 
  cout << "random: " << ranfp << endl;

  ranfp = FairRandomIEEEFP (bit_width, 
			    (double) fp_low, 
			    (double) fp_high, 
			    include_denormal); 
  cout << "random: " << ranfp << endl;

  ranfp = FairRandomIEEEFP (bit_width, 
			    (double) fp_low, 
			    (double) fp_high, 
			    include_denormal); 
  cout << "random: " << ranfp << endl;

  cout << "--------" << endl;

  SetSeedRandomEngine(0); 

  ranfp = FairRandomIEEEFP (bit_width, 
			    (double) fp_low, 
			    (double) fp_high, 
			    include_denormal); 
  cout << "random: " << ranfp << endl;

  ranfp = FairRandomIEEEFP (bit_width, 
			    (double) fp_low, 
			    (double) fp_high, 
			    include_denormal); 
  cout << "random: " << ranfp << endl;

  ranfp = FairRandomIEEEFP (bit_width, 
			    (double) fp_low, 
			    (double) fp_high, 
			    include_denormal); 
  cout << "random: " << ranfp << endl;

  cout << "--------" << endl;

  cout << "======== ======== ========" << endl;
  
  for (unsigned int i = 0 ; i < 20 ; i++) {
    for (unsigned int j = 0 ; j < 1000000 ; j++) {
      ranfp = FairRandomIEEEFP (bit_width, 
				(double) -100.0, 
				(double) 100.0, 
				true); 
      assert((ranfp < 100.0) && (ranfp >= -100.0));
    }
  }

  cout << "======== ======== ========" << endl;

  for (unsigned int i = 0 ; i < 20 ; i++) {
    for (unsigned int j = 0 ; j < 1000000 ; j++) {
      ranfp = FairRandomIEEEFP (bit_width, 
				(double) -100.0, 
				(double) 100.0, 
				false); 
      assert((ranfp < 100.0) && (ranfp >= -100.0));
    }
  }

  cout << "======== ======== ========" << endl;

  for (unsigned int i = 0 ; i < 20 ; i++) {
    for (unsigned int j = 0 ; j < 1000000 ; j++) {
      ranfp = FairRandomIEEEFP (bit_width, 
				(double) -2.0, 
				(double) 2.0, 
				true); 
      assert((ranfp < 2.0) && (ranfp >= -2.0));
    }
  }

  cout << "======== ======== ========" << endl;

  for (unsigned int i = 0 ; i < 20 ; i++) {
    for (unsigned int j = 0 ; j < 1000000 ; j++) {
      ranfp = FairRandomIEEEFP (bit_width, 
				(double) -2.0, 
				(double) 2.0, 
				false); 
      assert((ranfp < 2.0) && (ranfp >= -2.0));
    }
  }

  cout << "======== ======== ========" << endl;
    
  return 0;
}


