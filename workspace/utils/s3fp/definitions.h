#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <vector>
#include <map>
#include <time.h>
extern "C" {
#include <quadmath.h>
}
#include<cmath>

#ifdef S3FP_DDIO
#include <qd/dd_real.h>
#include <qd/qd_real.h>
#define HFP_TYPE dd_real
#define DD_OUTPUT_PRECISION 15
#else 
#define HFP_TYPE double
#endif 

// #define S3FP_VERBOSE 


#define RSCALE 100000000

#define RANGE_GRANULARITY 20
#define N_SEARCH_ITERATIONS 10
#define MAX_SEARCH_RUNS -1
#define N_RETRY_READ_OUTPUT 100

#ifndef N_RANDOM_BINARY_PARTITIONS 
#define N_RANDOM_BINARY_PARTITIONS 1
#endif 

#ifndef RRESTART 
#define RRESTART 0.2
#endif 

/* 
   function prototypes
*/
void rmFile (const std::string &fname); 
