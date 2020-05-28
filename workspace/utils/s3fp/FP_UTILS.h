#pragma once 

extern "C" {
#include <quadmath.h>
}
#include <assert.h>
#include <inttypes.h>
#include "boost/multiprecision/cpp_int.hpp"
#include <random>
using namespace std;
using namespace boost::multiprecision;


// ---- global variables ----
default_random_engine RENGINE; 


// ---- utilities of decomposition of floating-point numbers ----
uint128_t MaxUint128 () {
  uint64_t all_one[2];
  all_one[0] = all_one[1] = 0xFFFFFFFFFFFFFFFF; 
  uint128_t mask_ones; 
  memcpy(&mask_ones, all_one, 16); 
  return mask_ones; 
}

void DecomposeIEEEFP (unsigned int bit_width, 
		      double in_fp, 
		      int &out_sign, 
		      uint128_t &out_expo, 
		      uint128_t &out_mant) {
  assert(bit_width == 32 ||
	 bit_width == 64 ||
	 bit_width == 128); 
  assert(sizeof(float) == 4 && sizeof(double) == 8);

  if (bit_width == 32) {
    float fp = in_fp;
    assert(fp == in_fp);

    uint32_t intfp; 
    int sign;
    uint32_t mant;
    uint32_t expo; 
    memcpy(&intfp, &fp, 4); 

    uint32_t mask_mant = 0x7FFFFF; 
    mant = intfp & mask_mant;
  
    uint32_t mask_expo = 0xFF; 
    expo = (intfp >> 23) & mask_expo; 
  
    if ((intfp >> 31) == 1) sign = 1;
    else sign = 0;

    out_sign = sign; 
    out_mant = mant;
    out_expo = expo;   
    assert(out_sign == sign && out_mant == mant && out_expo == expo); 
  }
  else if (bit_width == 64) {
    double fp = in_fp;
    assert(fp == in_fp);

    uint64_t intfp; 
    int sign;
    uint64_t mant;
    uint64_t expo; 
    memcpy(&intfp, &fp, 8); 

    uint64_t mask_mant = 0xFFFFFFFFFFFFF; 
    mant = intfp & mask_mant;
  
    uint64_t mask_expo = 0x7FF; 
    expo = (intfp >> 52) & mask_expo; 
    
    if ((intfp >> 63) == 1) sign = 1;
    else sign = 0;

    out_sign = sign; 
    out_mant = mant;
    out_expo = expo;   
    assert(out_sign == sign && out_mant == mant && out_expo == expo); 
  }
  else if (bit_width == 128) {
    double fp = in_fp;
    assert(fp == in_fp);
    
    uint128_t intfp; 
    int sign;
    uint128_t mant;
    uint128_t expo; 
    memcpy(&intfp, &fp, 16); 

    uint128_t mask_ones = MaxUint128(); 
    uint128_t mask_mant = mask_ones >> 16; // 0x0000FFFFFFFFFFFFFFFFFFFFFFFFFFFF; 
    mant = intfp & mask_mant;
  
    uint128_t mask_expo = mask_ones >> 113; // 0x00000000000000000000000000007FFF;
    expo = (intfp >> 112) & mask_expo; 
  
    if ((intfp >> 127) == 1) sign = 1;
    else sign = 0;

    out_sign = sign; 
    out_mant = mant;
    out_expo = expo; 
    assert(out_sign == sign && out_mant == mant && out_expo == expo); 
  }
  else assert(false && "ERROR [DecomposeIEEEFP]: Impossible Case... \n");
}


// ---- predicates for floating-point numbers ---- 
bool IsDenormalIEEEFP (int sign, 
		       uint128_t expo, 
		       uint128_t mant) {
  if (expo == 0 && mant != 0) return true;
  else return false;
}

bool IsZeroIEEEFP (int sign, 
		   uint128_t expo, 
		   uint128_t mant) {
  if (expo == 0 && mant == 0) return true;
  else return false;
}

bool IsInfIEEEFP (unsigned int bit_width, 
		  int sign, 
		  uint128_t expo, 
		  uint128_t mant) {
  switch (bit_width) {
  case 32:
    if (expo == 0xFF && mant == 0) return true;
    break;
  case 64:
    if (expo == 0x7FF && mant == 0) return true;
    break;
  case 128:
    if (expo == 0x7FF && mant == 0) return true;
    break;
  default:
    assert(false && "ERROR [IsInfIEEEFP]: Invalid bit_width \n"); 
    break;
  }
  return false;
}

bool IsNaNIEEEFP (unsigned int bit_width, 
		  int sign, 
		  uint128_t expo, 
		  uint128_t mant) {
  assert(sign == 0 || sign == 1);

  switch (bit_width) {
  case 32:
    if (expo == 0xFF && mant != 0) return true;
    break;
  case 64:
    if (expo == 0x7FF && mant != 0) return true;
    break;
  case 128:
    if (expo == 0x7FF && mant != 0) return true;
    break;
  default:
    assert(false && "ERROR [IsNaNIEEEFP]: Invalid bit_width \n"); 
    break;
  }
  return false;
}


// ---- utilities for creating new floating-point numbers ---- 
double MakeIEEEFP (unsigned int bit_width, 
		       int sign, 
		       uint128_t in_expo, 
		       uint128_t in_mant) {
  assert(sign == 0 || sign == 1);
  assert(bit_width == 32 ||
	 bit_width == 64 ||
	 bit_width == 128); 
  assert(sizeof(float) == 4 && sizeof(double) == 8);
  
  if (bit_width == 32) {
    uint32_t int_ret; 
    uint32_t expo = in_expo.convert_to<uint32_t>();
    uint32_t mant = in_mant.convert_to<uint32_t>();
    assert(expo == in_expo && mant == in_mant); 

    int_ret = sign; 
    int_ret = int_ret << 31; 
    int_ret += (expo << 23);
    int_ret += mant; 
    float fp_ret;
    memcpy(&fp_ret, &int_ret, 4); 
    return (double) fp_ret;
  }
  else if (bit_width == 64) {
    uint64_t int_ret; 
    uint64_t expo = in_expo.convert_to<uint64_t>();
    uint64_t mant = in_mant.convert_to<uint64_t>();
    assert(expo == in_expo && mant == in_mant); 
    
    int_ret = sign; 
    int_ret = int_ret << 63;
    int_ret += (expo << 52); 
    int_ret += mant; 
    double fp_ret;
    memcpy(&fp_ret, &int_ret, 8); 
    return (double) fp_ret;
  }
  else if (bit_width == 128) {
    uint128_t int_ret; 
    uint128_t expo = in_expo;
    uint128_t mant = in_mant; 
    assert(expo == in_expo && mant == in_mant); 
    
    int_ret = sign; 
    int_ret = int_ret << 127; 
    int_ret += (expo << 112); 
    int_ret += mant; 
    double fp_ret;
    memcpy(&fp_ret, &int_ret, 16); 
    return fp_ret;
  }
  else assert(false && "ERROR [MakeIEEEFP]: Impossible Case... \n");
}


// ---- calculate the numbers of floating-point numbers between two FPS ----
// Number of FPS between [low_fp, high_fp) 
uint128_t NFPSBetweenTwoIEEEFPS (unsigned int bit_width, 
				 double low_fp, 
				 double high_fp, 
				 bool include_subnormal = false) { 
  assert(bit_width == 32 ||
	 bit_width == 64 ||
	 bit_width == 128); 
  assert(sizeof(float) == 4 && sizeof(double) == 8);

  assert(low_fp <= high_fp);
  if (low_fp == high_fp) return 0;

  uint128_t n_fps = 0;
  
  int low_sign, high_sign;
  uint128_t low_mant, high_mant;
  uint128_t low_expo, high_expo;
  DecomposeIEEEFP(bit_width, low_fp, low_sign, low_expo, low_mant);
  DecomposeIEEEFP(bit_width, high_fp, high_sign, high_expo, high_mant);
  assert(high_sign == 1 || high_sign == 0);
  assert(low_sign == 1 || low_sign == 0);
  assert(IsInfIEEEFP(bit_width, low_sign, low_expo, low_mant) == false);
  assert(IsNaNIEEEFP(bit_width, low_sign, low_expo, high_mant) == false);
  assert(IsInfIEEEFP(bit_width, high_sign, high_expo, high_mant) == false);
  assert(IsNaNIEEEFP(bit_width, high_sign, high_expo, high_mant) == false);  

  if (high_fp == 0) {
    if (include_subnormal) 
      return 1 + NFPSBetweenTwoIEEEFPS(bit_width, 
				       low_fp, 
				       MakeIEEEFP(bit_width, 1, 0, 1), // the largest -subnormal
				       true);
    else 
      return 1 + NFPSBetweenTwoIEEEFPS(bit_width, 
				       low_fp, 
				       MakeIEEEFP(bit_width, 1, 1, 0), // the largest -normal
				       false);
  }
  else if (low_fp == 0) {
    if (include_subnormal) 
      return 1 + NFPSBetweenTwoIEEEFPS(bit_width, 
				       MakeIEEEFP(bit_width, 0, 0, 1), // the smallest +subnormal
				       high_fp, 
				       true);
    else 
      return 1 + NFPSBetweenTwoIEEEFPS(bit_width, 
				       MakeIEEEFP(bit_width, 0, 1, 0), // the smallest +normal
				       high_fp, 
				       false); 
  }
  else if (high_sign == 0 && low_sign == 1) {
    return NFPSBetweenTwoIEEEFPS(bit_width, 0, high_fp, include_subnormal) 
      + NFPSBetweenTwoIEEEFPS(bit_width, low_fp, 0, include_subnormal);
  }
  else if (high_sign == 0 && low_sign == 0) {
    bool subn_high = IsDenormalIEEEFP(high_sign, high_expo, high_mant); 
    bool subn_low = IsDenormalIEEEFP(low_sign, low_expo, low_mant); 

    if ((subn_high == true) &&
	(subn_low == true)) {
      assert(high_mant >= low_mant);
      return high_mant - low_mant; 
    }
    else if ((subn_high == false) &&
	     (subn_low == true)) {
      n_fps = NFPSBetweenTwoIEEEFPS(bit_width, 
				    MakeIEEEFP(bit_width, 0, 1, 0), // the smallest +normal
				    high_fp, 
				    include_subnormal); 
      if (include_subnormal) { 
	double new_high_fp; // the largest +subnormal
	uint128_t mask_ones = MaxUint128(); 
	switch (bit_width) {
	case 32:
	  new_high_fp = MakeIEEEFP(bit_width, 0, 0, 0x7FFFFF);
	  break;
	case 64:
	  new_high_fp = MakeIEEEFP(bit_width, 0, 0, 0xFFFFFFFFFFFFF);
	  break;
	case 128:
	  mask_ones = mask_ones >> 16; 
	  new_high_fp = MakeIEEEFP(bit_width, 0, 0, mask_ones);
	  break;
	default:
	  assert(false && "ERROR: Impossible Case... \n");
	  break;
	}
	n_fps += (1 + NFPSBetweenTwoIEEEFPS(bit_width, 
					    low_fp, 
					    new_high_fp, 
					    include_subnormal));
      }
     
      return n_fps;
    }
    else if ((subn_high == false) &&
	     (subn_low == false)) {
      if (high_expo == low_expo) {
	assert(high_mant >= low_mant);
	return (high_mant - low_mant);
      }
      else if (high_expo > low_expo) {
	double new_high_fp; 
	uint128_t mask_ones = MaxUint128();
	switch (bit_width) {
	case 32:
	  n_fps = ((high_expo - low_expo) - 1) << 23; 
	  new_high_fp = MakeIEEEFP(bit_width, 0, low_expo, 0x7FFFFF); 
	  break;
	case 64:
	  n_fps = ((high_expo - low_expo) - 1) << 52; 
	  new_high_fp = MakeIEEEFP(bit_width, 0, low_expo, 0xFFFFFFFFFFFFF);
	  break;
	case 128:
	  n_fps = ((high_expo - low_expo) - 1) << 112; 
	  mask_ones = mask_ones >> 16; 
	  new_high_fp = MakeIEEEFP(bit_width, 0, low_expo, mask_ones);
	  break;
	default:
	  assert(false && "ERROR: Impossible Case... \n");
	  break;
	}
	n_fps += NFPSBetweenTwoIEEEFPS(bit_width, 
				       MakeIEEEFP(bit_width, 0, high_expo, 0), 
				       high_fp, 
				       include_subnormal);
	n_fps += (1 + NFPSBetweenTwoIEEEFPS(bit_width, 
					    low_fp, 
					    new_high_fp, 
					    include_subnormal));
      }
      else assert(false && "ERROR: Impossible Case... \n");
    }
    else assert(false && "ERROR: Impossible Case... \n");
  }
  else if (high_sign == 1 && low_sign == 1) {
    return NFPSBetweenTwoIEEEFPS(bit_width, 
				 (-1)*high_fp, 
				 (-1)*low_fp, 
				 include_subnormal);
  }
  else assert(false && "Error Impossible Case... \n");

  return n_fps; 
}


// ---- shift IEEE floating-point numbers ---- 
// direction == 0 --> to +inf
// direction == 1 --> to -inf
void ShiftIEEEFP (unsigned int bit_width, 
		  int sign, uint128_t expo, uint128_t mant, 
		  int direction, 
		  uint128_t distance,
		  bool include_denormal, 
		  int &out_sign, uint128_t &out_expo, uint128_t &out_mant) {
  // cout << "---- class (" << sign << ", " << expo << ", " << mant << ") (" << direction << ", " << distance << ") ----" << endl;
  assert(sign == 0 || sign == 1);
  assert(bit_width == 32 ||
	 bit_width == 64 ||
	 bit_width == 128); 
  assert(sizeof(float) == 4 && sizeof(double) == 8);
  assert(direction == 0 || direction == 1);
  assert(include_denormal || (IsDenormalIEEEFP(sign, expo, mant) == false));

  if (distance == 0) {
    out_sign = sign;
    out_expo = expo;
    out_mant = mant;
    return ;
  }

  uint128_t ones112 = MaxUint128(); 
  uint128_t max_mant; 
  uint128_t diff_expo; 
  uint128_t diff_mant;
  switch (bit_width) {
  case 32:
    max_mant = 0x7FFFFF; 
    diff_expo = distance >> 23; 
    diff_mant = distance & max_mant; 
    break;
  case 64:
    max_mant = 0xFFFFFFFFFFFFF; 
    diff_expo = distance >> 52;
    diff_mant = distance & max_mant; 
    break;
  case 128:
    max_mant = ones112;
    diff_expo = distance >> 112; 
    diff_mant = distance & max_mant; 
    break;
  default:
    assert(false && "ERROR: Impossible Case... \n");
    break;
  }

  if (sign == 0) {
    if (direction == 0) { // go +inf
      if (IsZeroIEEEFP(sign, expo, mant)) { // start from zero
	if (include_denormal) 
	  ShiftIEEEFP(bit_width, 
		      sign, 0, 1, 
		      0, 
		      (distance - 1), 
		      include_denormal, 
		      out_sign, out_expo, out_mant);
	else 
	  ShiftIEEEFP(bit_width, 
		      sign, 1, 0, 
		      0, 
		      (distance - 1), 
		      include_denormal, 
		      out_sign, out_expo, out_mant);
	return ; 
      }
      else { // start from non-zero 
	out_sign = 0;
	out_expo = expo + diff_expo;
	if ((max_mant - mant) < diff_mant) {
	  out_expo++; 
	  diff_mant = (diff_mant - (max_mant - mant)) - 1; 
	  out_mant = diff_mant; 
	}
	else {
	  out_mant = mant + diff_mant; 
	}
	return ;
      }
    }
    else { // go -inf
      if (IsZeroIEEEFP(sign, expo, mant)) { // start from zero 
	ShiftIEEEFP (bit_width, 
		     sign, expo, mant, 
		     0, 
		     distance, 
		     include_denormal, 
		     out_sign, out_expo, out_mant); 
	out_sign = (out_sign == 1 ? 0 : 1);
	return ; 
      }
      else { // start from non-zero
	uint128_t gap;
	if (include_denormal)
	  gap = NFPSBetweenTwoIEEEFPS(bit_width, 
				      MakeIEEEFP(bit_width, sign, 0, 1), 
				      MakeIEEEFP(bit_width, sign, expo, mant), 
				      include_denormal);
	else 
	  gap = NFPSBetweenTwoIEEEFPS(bit_width, 
				      MakeIEEEFP(bit_width, sign, 1, 0), 
				      MakeIEEEFP(bit_width, sign, expo, mant), 
				      include_denormal);
	  
	if (gap >= distance) {
	  if (include_denormal) 
	    ShiftIEEEFP (bit_width, 
			 sign, 0, 1, 
			 0, 
			 (gap - distance), 
			 include_denormal, 
			 out_sign, out_expo, out_mant); 
	  else 
	    ShiftIEEEFP (bit_width, 
			 sign, 1, 0, 
			 0, 
			 (gap - distance), 
			 include_denormal, 
			 out_sign, out_expo, out_mant); 

	}
	else if ((gap+1) == distance) {
	  DecomposeIEEEFP(bit_width, 0, out_sign, out_expo, out_mant);
	}
	else { // (gap+1) < distance
	  if (include_denormal)
	    ShiftIEEEFP (bit_width, 
			 sign, 0, 1, 
			 0, 
			 ((distance - (gap+1)) - 1), 
			 include_denormal, 
			 out_sign, out_expo, out_mant); 
	  else 
	    ShiftIEEEFP (bit_width, 
			 sign, 1, 0, 
			 0, 
			 ((distance - (gap+1)) - 1), 
			 include_denormal, 
			 out_sign, out_expo, out_mant); 
	  out_sign = (out_sign == 0 ? 1 : 0); 
	}
	
	return ;
      }
    }
  }
  else { // sign == 1
    ShiftIEEEFP (bit_width, 
		 0, expo, mant, 
		 (direction == 0 ? 1 : 0), 
		 distance,
		 include_denormal, 
		 out_sign, out_expo, out_mant); 
    out_sign = (out_sign == 0 ? 1 : 0); 
    
    return ;
  }
}

void ShiftIEEEFP (unsigned int bit_width, 
		  double in_fp, 
		  int direction, 
		  uint128_t distance,
		  bool include_denormal, 
		  double &out_fp) {
  int in_sign, out_sign;
  uint128_t in_expo, out_expo;
  uint128_t in_mant, out_mant;
  DecomposeIEEEFP(bit_width, in_fp, in_sign, in_expo, in_mant);

  ShiftIEEEFP(bit_width, 
	      in_sign, in_expo, in_mant, 
	      direction, 
	      distance, 
	      include_denormal, 
	      out_sign, out_expo, out_mant);
  out_fp = MakeIEEEFP(bit_width, out_sign, out_expo, out_mant);
}


// ---- utilities for "fair" floating-point number sampling ---- 
void SetSeedRandomEngine(unsigned int rseed) {
  RENGINE.seed(rseed); 
}

double FairRandomIEEEFP (unsigned int bit_width, 
			 double low_fp, 
			 double high_fp, 
			 bool include_denormal) {
  assert(bit_width == 32 || bit_width == 64); 
  assert(low_fp <= high_fp);

  int low_sign, hihg_sign;
  uint128_t low_expo, high_expo;
  uint128_t low_mant, high_mant; 
  uint64_t n_fps_between = NFPSBetweenTwoIEEEFPS (bit_width, 
						  low_fp, 
						  high_fp, 
						  include_denormal).convert_to<uint64_t>(); 
  uniform_int_distribution<uint64_t> dist(0, n_fps_between);
  uint64_t rnum = dist(RENGINE); 
  
  double ret_fp; 
  ShiftIEEEFP(bit_width, 
	      (double)low_fp, 
	      0, 
	      rnum,
	      include_denormal, 
	      ret_fp);
  assert(ret_fp == (double)ret_fp);

  return (double)ret_fp; 
}



