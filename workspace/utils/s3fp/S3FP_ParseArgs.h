#include<iostream>

using namespace std;


enum ENUM_RT_MODE {
  NA_RT_MODE, 
  URT_RT_MODE, 
  BGRT_RT_MODE, 
  ILS_RT_MODE, 
  PSO_RT_MODE
}; 

enum ENUM_TRESOURCE {
  NA_TRESOURCE,
  TIME_TRESOURCE, 
  SVE_TRESOURCE
};

enum ENUM_FP_ERROR {
  NA_FP_ERROR, 
  REL_FP_ERROR, 
  ABS_FP_ERROR,
  ULP_FP_ERROR
};


bool S3FP_ParseArgs (ENUM_RT_MODE &rt_mode, 
		     unsigned int &timeout, 
		     ENUM_TRESOURCE &t_resource, 
		     unsigned int &rseed, 
		     bool &parallelrt, 
		     ENUM_FP_ERROR &t_fp_error, 
		     double &rel_delta,
		     bool &uniform_input, 
		     double &uniform_inputlb, 
		     double &uniform_inputub, 
		     string &input_range_file, 
		     bool &dump_conf_to_errors, 
		     string &conf_to_errors_file, 
		     bool &check_unstable_error, 
		     unsigned int &n_input_repeats, 
		     unsigned int &n_outputs, 
		     bool &backup_input_to_errors, 
		     string &input_to_errors_file, 
		     bool &dump_errors_stream, 
		     string &errors_stream_file); 
