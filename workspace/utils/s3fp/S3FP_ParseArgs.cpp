#include "S3FP_ParseArgs.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <assert.h>


using namespace std;


#define SETTING_FNAME "./s3fp_setting" 
#define SBUFF_SIZE 300


string stringTrim (string ins) {
  int is = 0;
  int ie = ins.length() - 1;
  
  while (ie >= 0) {
    if (ins[ie] == ' ') ie--;
    else break;
  }

  if (ie == 0) return "";
  
  while (is < ie) {
    if (ins[is] == ' ') is++;
    else break; 
  }

  return ins.substr(is, (ie-is)+1);
}


inline 
bool aImpliesB (bool a, bool b) {
  return (a == false || b == true); 
}


bool getNameValue (string sline, string &sname, string &svalue) {
  int ieq = sline.find("=");
  if (ieq == string::npos) return false;
  sname = stringTrim(sline.substr(0, ieq));
  svalue = stringTrim(sline.substr(ieq+1));
  return true;
}


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
		     string &errors_stream_file) {
  bool has_rt_mode = false;
  bool has_timeout = false;
  bool has_t_resource = false;
  bool has_rseed = false;
  bool has_parallelrt = false;
  bool has_t_fp_error = false;
  bool has_uniform_input = false;
  bool has_uniform_inputlb = false;
  bool has_uniform_inputub = false;
  bool has_input_range_file = false;
  bool has_conf_to_errors_file = false;
  bool has_rel_delta = false;
  bool has_check_unstable_error = false;
  bool has_n_input_repeats = false;
  bool has_n_outputs = false;
  bool has_backup_input_to_errors = false;
  bool has_dump_errors_stream = false;

  ifstream sfile (SETTING_FNAME); 

  char sbuff[SBUFF_SIZE];
  string sname;
  string svalue; 
  while (!sfile.eof()) {
    sfile.getline(sbuff, SBUFF_SIZE); 
    string sline (sbuff);
    getNameValue(sline, sname, svalue);

    if (sname.compare("RT") == 0) {
      if (svalue.compare("URT") == 0) rt_mode = URT_RT_MODE;
      else if (svalue.compare("BGRT") == 0) rt_mode = BGRT_RT_MODE;
      else if (svalue.compare("ILS") == 0) rt_mode = ILS_RT_MODE;
      else if (svalue.compare("PSO") == 0) rt_mode = PSO_RT_MODE;
      else assert(false);
      has_rt_mode = true;
    }
    else if (sname.compare("TIMEOUT") == 0) {
      timeout = atoi(svalue.c_str());
      has_timeout = true;
    }
    else if (sname.compare("RESOURCE") == 0) {
      if (svalue.compare("TIME") == 0) t_resource = TIME_TRESOURCE;
      else if (svalue.compare("SVE") == 0) t_resource = SVE_TRESOURCE;
      else assert(false);
      has_t_resource = true;
    }
    else if (sname.compare("RSEED") == 0) {
      rseed = atoi(svalue.c_str());
      has_rseed = true;
    }
    else if (sname.compare("PARALLELRT") == 0) {
      if (svalue.compare("true") == 0) parallelrt = true;
      else if (svalue.compare("false") == 0) parallelrt = false;
      else assert(false); 
      has_parallelrt = true;
    }
    else if (sname.compare("FTERROR") == 0) {
      if (svalue.compare("ABS") == 0) t_fp_error = ABS_FP_ERROR;
      else if (svalue.compare("REL") == 0) t_fp_error = REL_FP_ERROR;
      else if (svalue.compare("ULP") == 0) t_fp_error = ULP_FP_ERROR;
      else assert(false); 
      has_t_fp_error = true;
    }
    else if (sname.compare("REL_DELTA") == 0) {
      rel_delta = (double) atof(svalue.c_str());
      has_rel_delta = true;
    }
    else if (sname.compare("UNIFORM_INPUT") == 0) {
      if (svalue.compare("true") == 0) uniform_input = true;
      else uniform_input = false;
      has_uniform_input = true;
    }
    else if (sname.compare("UNIFORM_INPUTLB") == 0) {
      uniform_inputlb = (double) atof(svalue.c_str());
      has_uniform_inputlb = true;
    }
    else if (sname.compare("UNIFORM_INPUTUB") == 0) {
      uniform_inputub = (double) atof(svalue.c_str());
      has_uniform_inputub = true;
    }
    else if (sname.compare("INPUT_RANGE_FILE") == 0) {
      input_range_file = svalue; 
      has_input_range_file = true;
    }
    else if (sname.compare("CONF_TO_ERRORS_FILE") == 0) {
      conf_to_errors_file = svalue;
      has_conf_to_errors_file = true;
      dump_conf_to_errors = true;
    }
    else if (sname.compare("CHECK_UNSTABLE_ERROR") == 0) {
      if (svalue.compare("true") == 0) check_unstable_error = true;
      else check_unstable_error = false;
      has_check_unstable_error = true;
    }
    else if (sname.compare("N_INPUT_REPEATS") == 0) {
      n_input_repeats = atoi(svalue.c_str());
      has_n_input_repeats = true;
    }
    else if (sname.compare("N_OUTPUTS") == 0) {
      n_outputs = atoi(svalue.c_str());
      has_n_outputs= true;
    }
    else if (sname.compare("INPUT_TO_ERRORS_FILE") == 0) {
      input_to_errors_file = svalue; 
      has_backup_input_to_errors = true;
      backup_input_to_errors = true;
    }
    else if (sname.compare("ERRORS_STREAM_FILE") == 0) {
      errors_stream_file = svalue;
      has_dump_errors_stream = true;
      dump_errors_stream = true;
    }
    else assert(false); 
  }

  sfile.close();

  if (has_rel_delta == false) {
    cout << "[WARNING] Missing Specification of REL_DELTA" << endl;
    cout << "Assuming REL_DELTA = 0" << endl;
    rel_delta = 0;
  }
  if (has_rt_mode == false) {
    cout << "[ERROR] Missing Specification of RT Mode" << endl;    
    assert(has_rt_mode);
  }
  if (has_timeout == false) {
    cout << "[ERROR] Missing Specification of Timeout" << endl;    
    assert(has_timeout);
  }
  if (has_t_resource == false) {
    cout << "[ERROR] Missing Specification of Resource Type" << endl;    
    assert(has_t_resource);
  }
  // assert(has_rseed);
  // assert(has_parallelrt);
  if (has_t_fp_error == false) {
    cout << "[ERROR] Missing Specification of FP Error Type" << endl;    
    assert(has_t_fp_error); 
  }
  if (has_uniform_input == false) {
    cout << "[ERROR] Missing Specification of Using Uniform Input (boolean)" << endl;    
    assert(has_uniform_input);  
  }
  if (aImpliesB(uniform_input, has_uniform_inputlb) == false) {
    cout << "[ERROR] Missing Specification of Uniform Lower Bound" << endl;    
    assert(aImpliesB(uniform_input, has_uniform_inputlb));
  }
  if (aImpliesB(uniform_input, has_uniform_inputub) == false) {
    cout << "[ERROR] Missing Specification of Uniform Upper Bound" << endl;    
    assert(aImpliesB(uniform_input, has_uniform_inputub));
  }
  if (aImpliesB(uniform_input==false, has_input_range_file) == false) {
    cout << "[ERROR] Missing Specification of Input Range File" << endl;    
    assert(aImpliesB(uniform_input==false, has_input_range_file));
  }
  if (has_conf_to_errors_file == false) 
    dump_conf_to_errors = false;
  if (has_backup_input_to_errors == false) 
    backup_input_to_errors = false;

  if (rt_mode == NA_RT_MODE) { 
    cout << "[ERROR] Bad Specification of RT Mode" << endl;
    assert(rt_mode != NA_RT_MODE);
  }
  if (timeout <= 0) {
    cout << "[ERROR] Bad Specification of Timeout" << endl;
    assert(timeout > 0);
  }
  if (t_resource == NA_TRESOURCE) {
    cout << "[ERROR] Bad Specification of Resource Type" << endl;
    assert(t_resource != NA_TRESOURCE);
  }
  if (t_fp_error == NA_FP_ERROR) {
    cout << "[ERROR] Bad Specification of FP Error Type" << endl;
    assert(t_fp_error != NA_FP_ERROR);
  }
  if (has_n_input_repeats && 
      n_input_repeats < 1) {
    cout << "[ERROR] Bad Specification of N_INPUT_REPEATS" << endl;
    assert(n_input_repeats >= 1);
  }
  if (has_n_outputs && 
      n_outputs < 1) {
    cout << "[ERROR] Bad Specification of N_OUTPUTS" << endl;
    assert(n_outputs >= 1); 
  }

  return true;
}


