#pragma once

#include "definitions.h"
#include "FPR.h"
#include <vector>

using namespace std;

/**
 * A class which runs external programs
 */
class EvaluationBasis {
 private:
  unsigned int n_inputs; 
  unsigned int input_bitwidth; 
  string exeLP;
  string outnameLP;
  string exeHP;
  string outnameHP;
  string inputname;
  unsigned int n_input_repeats; 
  unsigned int n_outputs;

  string rtIOName(const string &prefix, unsigned int pid, unsigned int tid) const {
    stringstream rtss;
    rtss << prefix << "." << pid << "." << tid;
    return rtss.str();
  }

  // HFP_TYPE 
  void readOutput(const string &outname, 
		  vector<HFP_TYPE> &ret, 
		  int *error) const {
    FILE *fp = fopen(outname.c_str(), "r");

    if (fp) {
      fseek(fp, 0, SEEK_END);
      unsigned int n_fbytes = ftell(fp);
      fseek(fp, 0, SEEK_SET);
#ifdef S3FP_DDIO
      assert(n_fbytes % (sizeof(double)*2) == 0);
      unsigned int n_foutputs = n_fbytes / (sizeof(double)*2);
      assert(n_foutputs == n_outputs);
#else
      assert(n_fbytes % sizeof(HFP_TYPE) == 0);
      unsigned int n_foutputs = n_fbytes / sizeof(HFP_TYPE);
      assert(n_foutputs == n_outputs);
#endif

      for (unsigned int i = 0 ; i < n_outputs ; i++) {
	HFP_TYPE val;
	assert(!feof(fp));
#ifdef S3FP_DDIO
	fread(&val.x[0], sizeof(double), 1, fp);
	fread(&val.x[1], sizeof(double), 1, fp);
#else 
	fread(&val, sizeof(HFP_TYPE), 1, fp);
#endif 
	ret.push_back(val);
      }
      fclose(fp);
      *error = 0;
    }
    else {
      cout << "Output file " << outname << " doesn't exist" << endl;
      *error = 1;
    }
  }

 public:
  EvaluationBasis () {
    input_bitwidth = 0;
  }

  EvaluationBasis (unsigned int in_n_inputs, 
		   unsigned int in_input_bitwidth, 
		   const string &in_exeLP, 
		   const string &in_outnameLP, 
		   const string &in_exeHP, 
		   const string &in_outnameHP, 
		   const string &in_inputname,
		   unsigned int in_n_input_repeats, 
		   unsigned int in_n_outputs) {
    assert (in_input_bitwidth == 32 || in_input_bitwidth == 64 || in_input_bitwidth == 128);
    assert (in_n_input_repeats >= 1 && in_n_outputs >= 1);

    n_inputs = in_n_inputs;
    input_bitwidth = in_input_bitwidth; 
    exeLP = in_exeLP;
    outnameLP = in_outnameLP;
    exeHP = in_exeHP;
    outnameHP = in_outnameHP;
    inputname = in_inputname;
    n_input_repeats = in_n_input_repeats;
    n_outputs = in_n_outputs; 
  }

  EvaluationBasis &operator = (const EvaluationBasis &rhs) {
    n_inputs = rhs.n_inputs;
    input_bitwidth = rhs.input_bitwidth; 

    exeLP = rhs.exeLP;
    outnameLP = rhs.outnameLP;
    exeHP = rhs.exeHP;
    outnameHP = rhs.outnameHP;
    inputname = rhs.inputname;
    n_input_repeats = rhs.n_input_repeats;
    n_outputs = rhs.n_outputs;

    return *this;
  }

  string getInputname(bool PARALLELRT = false, unsigned int _pid = 0, unsigned int _tid = 0) const {
    if (PARALLELRT) {
      return rtIOName(inputname, _pid, _tid);
    }
    return inputname;
  }

  // Runs the low precision program on the current input and returns the result
  // HFP_TYPE 
  void runLP (int *error, 
	      vector<HFP_TYPE> &outputs, 
	      bool PARALLELRT = false, 
	      unsigned int _pid = 0, 
	      unsigned int _tid = 0) const { 
    stringstream exelp_para;
    string outname;
    string inname;

    if (PARALLELRT) { 
      outname = rtIOName(outnameLP, _pid, _tid);
      inname = rtIOName(inputname, _pid, _tid);
    }
    else {
      outname = outnameLP;
      inname = inputname;
    }

    rmFile(outname);
    exelp_para << exeLP << " " << inname << " " << outname;

    stringstream runlp;
#ifdef MPINP
    runlp << "mpirun -np " << MPINP << " " << exelp_para.set();
    system(runlp.str().c_str());
#else
    runlp << exelp_para.str() << " 2>&1 > __outdump"; 
    system(runlp.str().c_str());
#endif

    // return 
    readOutput(outname, outputs, error);
  }

  // Runs the high precision program on the current input and returns the result
  // HFP_TYPE 
  void runHP (int *error, 
	      vector<HFP_TYPE> &outputs, 
	      bool PARALLELRT = false, 
	      unsigned int _pid = 0, 
	      unsigned int _tid = 0) const { 
    stringstream exehp_para;
    string outname;
    string inname;

    if (PARALLELRT) { 
      outname = rtIOName(outnameHP, _pid, _tid);
      inname = rtIOName(inputname, _pid, _tid);
    }
    else {
      outname = outnameHP;
      inname = inputname;
    }

    rmFile(outname);
    exehp_para << exeHP << " " << inname << " " << outname;

    stringstream runhp;
#ifdef MPINP
    runhp << "mpirun -np " << MPINP << " " << exehp_para.str();
    system(runhp.str().c_str());
#else
    runhp << exehp_para.str() << " 2>&1 > __outdump"; 
    system(runhp.str().c_str());
#endif

    // return 
    readOutput(outname, outputs, error);
  }

  void prepareInput (const CONF &conf, bool PARALLELRT = false, 
		     unsigned int _pid = 0, unsigned int _tid = 0) const {
    if (PARALLELRT) {
      assert(n_input_repeats == 1);
      if (input_bitwidth == 32) 
	sampleCONF<float>(rtIOName(inputname, _pid, _tid), conf); 
      else if (input_bitwidth == 64)
	sampleCONF<double>(rtIOName(inputname, _pid, _tid), conf);
      else if (input_bitwidth == 128) 
	sampleCONF<HFP_TYPE>(rtIOName(inputname, _pid, _tid), conf);
      else assert(false);
    }
    else {
      for (unsigned int i = 0 ; i < n_outputs ; i++) {
	const char *write_mode = (i == 0) ? "w" : "a";
	if (input_bitwidth == 32) 
	  sampleCONF<float>(inputname, conf, write_mode);
	else if (input_bitwidth == 64)
	  sampleCONF<double>(inputname, conf, write_mode);
	else if (input_bitwidth == 128) 
	  sampleCONF<HFP_TYPE>(inputname, conf, write_mode);
	else assert(false);
      }
    }
  }

  unsigned int getNInputRepeats() { return n_input_repeats; }

  unsigned int getNOutputs() { return n_outputs; }

  unsigned int getNInputs() { return n_inputs; }

  unsigned int getInputBitwidth () { return input_bitwidth; }

  string getExeLP() { return exeLP; }
  string getExeHP() { return exeHP; }
};
