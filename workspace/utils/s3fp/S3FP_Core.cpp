#include "S3FP_Core.h"

#ifndef BEST_INPUT_FILE_NAME
#define BEST_INPUT_FILE_NAME "best_input"
#endif 

#ifndef NTPERP // number of threads per process 
#define NTPERP 10 
#endif 

#define SWARM_SIZE 20 

/* 
   function prototypes
*/
pair<HFP_TYPE, HFP_TYPE> 
varRange (unsigned int vindex, unsigned int n_vars, ifstream &irfile);
void initTPOOL (EvaluationBasis in_eva_basis);
void joinTPOOL (); 
void randCONF (CONF &ret_conf, unsigned int n_vars, bool plain=false);
void neighborCONFs (vector<CONF> &ret_confs, CONF this_conf, bool just_rand_one=false);
string CONFtoString (CONF conf); 
void dumpCONFtoStdout (CONF conf); 
void dumpCONFtoErrors (CONF conf, vector<FPR> rels);
void* threadBody (void *in_myid);
RTReport grtEvaluateCONF (EvaluationBasis eva_basis, unsigned int N, CONF conf); 
bool TerminationCriterion(time_t ts); 
void IterativeFirstImprovement (CONF conf, CONF &ret_conf, EvaluationBasis eva_basis, long nn_to_explore); 
void complementCONF (CONF base, CONF dsub, CONF &dret); 
void binarySubCONF (CONF base, CONF &dret, char select); 
void generateBinarySubCONFs (CONF base, vector<CONF> &ret_confs, unsigned int n_rand); 
void BinaryPartitionImprovement (CONF conf, CONF &ret_conf, EvaluationBasis eva_basis, unsigned int n_rand); 
bool isLittleEndian(); 
template <class FT> 
bool isValidFP (FT in_fp); 
void rtPlainRT (EvaluationBasis eva_basis, unsigned int n_vars ); 
void rtBGRT (EvaluationBasis eva_basis, unsigned int n_vars ); 
void rtILS (EvaluationBasis eva_basis, unsigned int n_vars ); 
void rtPSO (EvaluationBasis eva_basis, unsigned int n_particles, unsigned int n_vars, unsigned int N); 


/* 
   global variables
*/
unsigned int N_RTS = 0;
unsigned int N_VALID_RTS = 0;
CONF GLOBAL_BEST_CONF; 
RTReport GLOBAL_BEST;
unsigned int N_RESTARTS = 0;
unsigned int N_TERMINATION_CHECKS = 0;
time_t TSTART;
unsigned int ILSifi = 20;
unsigned int ILSr = 4;
unsigned int ILSs = 50;
unsigned int N_LOCAL_UPDATES = 0;
unsigned int N_GLOBAL_UPDATES = 0;
HFP_TYPE iteration_best = -1;
unsigned int _PID = 0;
unsigned int _TID = 0;
RTReport local_best;
pthread_t TPOOL[NTPERP];
unsigned int TIDS[NTPERP];
ThreadWorkQueue TWQ[NTPERP]; 
pthread_mutex_t lock_local_best;
pthread_mutex_t lock_inputgen;
pthread_barrier_t TBARR; 
pthread_barrierattr_t TBATTR;
ofstream ost_conf_to_errors; 
FILE *file_input_to_errors_local;
FILE *file_input_to_errors_global;
bool HALT_NOW;
double UNSTABLE_EPSILON = 0.0;
FILE *errors_stream; 

 

/* 
   main 
*/
int main (int argc, char *argv[]) {
  unsigned int i;
  unsigned int input_bitwidth = atoi(argv[1]);
  unsigned int n_vars = atoi(argv[2]);
  string input_name(argv[3]);
  string exeLP(argv[4]);
  string outLP_name(argv[5]);
  string exeHP(argv[6]);
  string outHP_name(argv[7]);

  cout << "==== S3FP arguments and settings ====" << endl;

  cout << "input bit-width: " << input_bitwidth << endl;
  cout << "n inputs: " << n_vars << endl;
  cout << "input_name: " << input_name << endl;
  cout << "LP EXE: " << exeLP << " -> " << outLP_name << endl;
  cout << "HP EXE: " << exeHP << " -> " << outHP_name << endl;

  double REL_DELTA_128; 
  double UNIFORM_INPUTLB_128;
  double UNIFORM_INPUTUB_128;
  assert(S3FP_ParseArgs(RT_MODE, 
			TIMEOUT, 
			T_RESOURCE, 
			RSEED, 
			PARALLELRT, 
			T_FP_ERROR, 
			REL_DELTA_128,
			UNIFORM_INPUT, 
			UNIFORM_INPUTLB_128, 
			UNIFORM_INPUTUB_128, 
			INPUT_RANGE_FILE, 
			DUMP_CONF_TO_ERRORS, 
			CONF_TO_ERRORS_FILE, 
			CHECK_UNSTABLE_ERROR, 
			N_INPUT_REPEATS, 
			N_OUTPUTS, 
			BACKUP_INPUT_TO_ERRORS, 
			INPUT_TO_ERRORS_FILE, 
			DUMP_ERRORS_STREAM, 
			ERRORS_STREAM_FILE));
#ifdef S3FP_DDIO
  REL_DELTA = dd_real((double)REL_DELTA_128);
  UNIFORM_INPUTLB = dd_real((double)UNIFORM_INPUTLB_128);
  UNIFORM_INPUTUB = dd_real((double)UNIFORM_INPUTUB_128);
#else 
  REL_DELTA = REL_DELTA_128;
  UNIFORM_INPUTLB = UNIFORM_INPUTLB_128;
  UNIFORM_INPUTUB = UNIFORM_INPUTUB_128;
#endif 

  cout << "RT_MODE: " << RT_MODE << endl;
  cout << "TIMEOUT: " << TIMEOUT << endl;
  cout << "T_RESOURCE: " << T_RESOURCE << endl;
  cout << "RSEED: " << RSEED << endl;
  cout << "PARALLELRT: " << PARALLELRT << endl;
  cout << "T_FP_ERROR: " << T_FP_ERROR << endl;
  cout << "UNIFORM_INPUT: " << UNIFORM_INPUT << endl;
  cout << "REL_DELTA: " << (OUTPUT_TYPE) REL_DELTA << endl;
  if (UNIFORM_INPUT) {
    cout << "UNIFORM_INPUTLB: " << (OUTPUT_TYPE) UNIFORM_INPUTLB << endl;
    cout << "UNIFORM_INPUTUB: " << (OUTPUT_TYPE) UNIFORM_INPUTUB << endl;	
  }
  if (UNIFORM_INPUT == false)
    cout << "INPUT_RANGE_FILE: " << INPUT_RANGE_FILE << endl;
  cout << "CHECK UNSTABLE ERROR: " << CHECK_UNSTABLE_ERROR << endl;
  if (DUMP_CONF_TO_ERRORS)
    cout << "CONF_TO_ERRORS_FILE: " << CONF_TO_ERRORS_FILE << endl;
  if (N_INPUT_REPEATS > 1) 
    cout << "N_INPUT_REPEATS: " << N_INPUT_REPEATS << endl;
  if (N_OUTPUTS > 1)
    cout << "N_OUTPUTS: " << N_OUTPUTS << endl;
  if (BACKUP_INPUT_TO_ERRORS) 
    cout << "INPUT_TO_ERRORS_FILE: " << INPUT_TO_ERRORS_FILE << endl;
  cout << "=====================================" << endl;

  // ---- initialize evaluation basis ----
  EvaluationBasis eva_basis (n_vars, 
			     input_bitwidth, 
			     exeLP, outLP_name, 
			     exeHP, outHP_name, 
			     input_name, 
			     N_INPUT_REPEATS, 
			     N_OUTPUTS);

  // ---- initialize the thread pool ----
  if (PARALLELRT) 
    initTPOOL(eva_basis);

  // ---- open conf-to-errors file ----
  if (DUMP_CONF_TO_ERRORS) {
    ost_conf_to_errors.open(CONF_TO_ERRORS_FILE.c_str());
    assert(ost_conf_to_errors.is_open());
  }

  // ---- open input-to-errors file ---- 
  if (BACKUP_INPUT_TO_ERRORS) {
    stringstream lss;
    stringstream gss;
    lss << INPUT_TO_ERRORS_FILE << ".local";
    gss << INPUT_TO_ERRORS_FILE << ".global";
    file_input_to_errors_local = fopen(lss.str().c_str(), "w");
    assert(file_input_to_errors_local != NULL);
    file_input_to_errors_global = fopen(gss.str().c_str(), "w");
    assert(file_input_to_errors_global != NULL);
    unsigned int n_errors = 1;
    unsigned int error_bitwidth = sizeof(HFP_TYPE) * 8;
    fwrite(&input_bitwidth, sizeof(unsigned int), 1, file_input_to_errors_local);
    fwrite(&input_bitwidth, sizeof(unsigned int), 1, file_input_to_errors_global);
    fwrite(&n_vars, sizeof(unsigned int), 1, file_input_to_errors_local);
    fwrite(&n_vars, sizeof(unsigned int), 1, file_input_to_errors_global);
    fwrite(&error_bitwidth, sizeof(unsigned int), 1, file_input_to_errors_local);
    fwrite(&error_bitwidth, sizeof(unsigned int), 1, file_input_to_errors_global);
    fwrite(&n_errors, sizeof(unsigned int), 1, file_input_to_errors_local);
    fwrite(&n_errors, sizeof(unsigned int), 1, file_input_to_errors_global);
  }

  // ---- open errors stream file ---- 
  if (DUMP_ERRORS_STREAM) {
    errors_stream = fopen(ERRORS_STREAM_FILE.c_str(), "w");
    assert(errors_stream != NULL);
  }

  // -- initialize some more global variables --
  srand(RSEED);
  startTimer(&TSTART);
  HALT_NOW = false;

  switch(RT_MODE) {
  case URT_RT_MODE: // unguided RT
    rtPlainRT (eva_basis, n_vars);
    break;
  case BGRT_RT_MODE: // binary guided RT
    rtBGRT (eva_basis, n_vars );
    break;
  case ILS_RT_MODE: // iterated local search 
    // ILSifi = -1;
    // ILSifi = n_vars; 
    // ILSifi = n_vars / 2;
    ILSifi = 20;
    // ILSr = n_vars;
    ILSr = 20;
    ILSs = 20;
    rtILS (eva_basis, n_vars);
    break;
  case PSO_RT_MODE: // particle swarm 
    rtPSO (eva_basis, SWARM_SIZE, n_vars, N_SEARCH_ITERATIONS);
    break;
  default:
    assert(false && "ERROR: not supported RT mode\n");
    break;
  }

  // ---- join threads ---- 
  if (PARALLELRT) 
    joinTPOOL();

  // ---- close conf-to-errors file 
  if (DUMP_CONF_TO_ERRORS)
    ost_conf_to_errors.close();

  // ---- close input-to-errors file 
  if (BACKUP_INPUT_TO_ERRORS) {
    fclose(file_input_to_errors_local);
    fclose(file_input_to_errors_global);
  }

  // ---- close errors stream file 
  if (DUMP_ERRORS_STREAM) 
    fclose(errors_stream); 

  return 0;
}

bool isTimeout(time_t ts) { 
  time_t tend; 
  time(&tend);
  if (CHECK_UNSTABLE_ERROR && HALT_NOW) return true;
  if (T_RESOURCE == SVE_TRESOURCE) 
    return (N_RTS >= TIMEOUT);
  else {
    return ((tend - ts) >= TIMEOUT);
  }
}


// return  1 if r1 <  r2                                                
// return  0 if r1 == r2                                              
// return -1 if r1 >  r2  
int RTReportCompare(RTReport &r1, RTReport &r2) {
  if (r1.has_best_relerr == false &&
      r2.has_best_relerr == false) return 0;
  else if (r1.has_best_relerr == false &&
           r2.has_best_relerr == true) return 1;
  else if (r1.has_best_relerr == true &&
           r2.has_best_relerr == false) return -1;
  else {
    if (r1.absolute_best_relerr < r2.absolute_best_relerr)
      return 1;
    else if (r1.absolute_best_relerr == r2.absolute_best_relerr)
      return 0;
    else // r1.absolute_best_relerr > r2.absolute_best_relerr  
      return -1;
  }
}


bool TerminationCriterion(time_t ts) { return isTimeout(ts); }


pair<HFP_TYPE, HFP_TYPE> 
varRange (unsigned int vindex, unsigned int n_vars, ifstream &irfile) {
  if (UNIFORM_INPUT) 
    return pair<HFP_TYPE, HFP_TYPE>(UNIFORM_INPUTLB, UNIFORM_INPUTUB);
  else {
    long double thislb;
    long double thisub;
    assert(irfile.is_open());
    assert(irfile.eof() == false);
    irfile >> thislb;
    assert(irfile.is_open());
    assert(irfile.eof() == false);
    irfile >> thisub;
    
#ifdef S3FP_DDIO
    return pair<HFP_TYPE, HFP_TYPE>(dd_real((double)thislb), 
				    dd_real((double)thisub));
#else
    return pair<HFP_TYPE, HFP_TYPE>((HFP_TYPE)thislb, (HFP_TYPE)thisub);
#endif 
  }
}


void initTPOOL (EvaluationBasis in_eva_basis) {
  lock_local_best = PTHREAD_MUTEX_INITIALIZER; 
  lock_inputgen = PTHREAD_MUTEX_INITIALIZER;

  pthread_barrier_init(&TBARR, &TBATTR, (NTPERP+1));

  for (unsigned int ti = 0 ; ti < NTPERP ; ti++) {
    TWQ[ti].init(in_eva_basis, N_SEARCH_ITERATIONS, false, false, false, 
		 _PID, ti);
    TIDS[ti] = ti;
    pthread_create(&TPOOL[ti], NULL, &threadBody, (void*)&TIDS[ti]);
  }
}


void joinTPOOL () {
  for (unsigned int ti = 0 ; ti < NTPERP ; ti++) 
    TWQ[ti].t_exit = true;
  pthread_barrier_wait(&TBARR);

  for (unsigned int ti = 0 ; ti < NTPERP ; ti++) {
    pthread_join(TPOOL[ti], NULL);
  }
}


void randCONF (CONF &ret_conf, unsigned int n_vars, bool plain) {
  ifstream irfile; 
  if (UNIFORM_INPUT == false) {
    irfile.open(INPUT_RANGE_FILE, ifstream::in);
    assert(irfile.is_open());
  }
  

  if (ret_conf.size() != 0) ret_conf.clear();
  for (unsigned int vi = 0 ; vi < n_vars ; vi++) {
    pair<HFP_TYPE, HFP_TYPE> var_range = varRange(vi, n_vars, irfile);

    if (plain == false) {
      FPR fpr (var_range.first, var_range.second, 
	       rand()%RANGE_GRANULARITY, RANGE_GRANULARITY);
      ret_conf.push_back(pair<unsigned int, FPR>(vi, fpr));
    }
    else {
      FPR fpr (var_range.first, var_range.second, 
	       0, 1);
      ret_conf.push_back(pair<unsigned int, FPR>(vi, fpr));
    }
  }

  if (UNIFORM_INPUT == false)
    irfile.close();
}


// when n_rands == -1 --> enumerate all possible neighbors 
void neighborCONFs (vector<CONF> &ret_confs, CONF this_conf, bool just_rand_one) {
  assert(just_rand_one || 
	 (just_rand_one == false && ret_confs.size() == 0));
  unsigned int n_vars = this_conf.size();

  unsigned int vi = 0; 
  bool up_neighbor = true;
  while (vi < n_vars) {
    if (just_rand_one) {
      vi = rand() % n_vars;
      if (rand() % 2 == 0)
	up_neighbor = true;
      else 
	up_neighbor = false;
    }

    FPR this_fpr = this_conf[vi].second;

    if (((just_rand_one && up_neighbor) || (just_rand_one == false)) && 
	(this_fpr.index_pars + 1 < this_fpr.n_pars)) {
      CONF new_conf = this_conf;
      (new_conf[vi].second.index_pars)++;
      ret_confs.push_back(new_conf);
      if (just_rand_one) break;
    }
    if (((just_rand_one && up_neighbor == false) || (just_rand_one == false)) && 
	(this_fpr.index_pars > 0)) {
      CONF new_conf = this_conf;
      (new_conf[vi].second.index_pars)--;
      ret_confs.push_back(new_conf);
      if (just_rand_one) break;
    } 

    if (just_rand_one == false) 
      vi++;
  }
}


void dumpInputOfInputToErrors (FILE *outfile, string inname, unsigned long n_inputs, bool lower_input_bitwidth, unsigned int input_bitwidth) {
  if (lower_input_bitwidth) 
    assert(input_bitwidth == 32 || input_bitwidth == 64); 
  assert(outfile != NULL);

  FILE *infile = fopen(inname.c_str(), "r");
  assert(infile != NULL); 
  for (unsigned int ii = 0 ; ii < n_inputs ; ii++) {
    HFP_TYPE idata;
    fread(&idata, sizeof(HFP_TYPE), 1, infile);
    if (lower_input_bitwidth && (input_bitwidth == 32)) {
#ifdef S3FP_DDIO
      float odata = (float) to_double(idata);
#else 
      float odata = (float)idata;
#endif 
      fwrite(&odata, sizeof(float), 1, outfile);
    }
    else if (lower_input_bitwidth && (input_bitwidth == 64)) {
#ifdef S3FP_DDIO
      double odata = (double) to_double(idata);
#else 
      double odata = (double) idata;
#endif 
      fwrite(&odata, sizeof(double), 1, outfile);
    }    
    else 
      fwrite(&idata, sizeof(HFP_TYPE), 1, outfile);
  }
  fclose(infile);
}


inline
void dumpErrorOfInputToErrors (FILE *outfile, HFP_TYPE fperr) {
  assert(outfile != NULL);
  fwrite(&fperr, sizeof(HFP_TYPE), 1, outfile);
} 


void dumpCONFtoErrors (ostream &ost, CONF conf, vector<FPR> rels) {
  unsigned int n_ivs = conf.size(); 
  unsigned int n_ovs = rels.size(); 

  for (unsigned int ii = 0 ; ii < n_ivs ; ii++) 
    ost << "input " << ii << " [" << (OUTPUT_TYPE) conf[ii].second.getLB() << ", " << (OUTPUT_TYPE) conf[ii].second.getUB() << "]" << endl;
  
  for (unsigned int oi = 0 ; oi < n_ovs ; oi++) 
    ost << "output " << oi << " [" << (OUTPUT_TYPE) rels[oi].getLB() << ", " << (OUTPUT_TYPE) rels[oi].getUB() << "]" << endl;

  ost << endl;
}


/**
 * Computes the ulp error of two floating-point numbers.
 * The parameter p specifies the intended precision of floating-point numbers:
 * p = 24 for float
 * p = 53 for double
 * p = FLT128_MANT_DIG for double
 */
double ulp_error(double approx, double exact, int p)
{
  if (!isValidFP(approx) || !isValidFP(exact)) {
    return 0;
  }

  int approxExp;
  double approxSignificand = frexpq(approx, &approxExp);
  double diff = approxSignificand - ldexpq(exact, -approxExp);

  double ulp = ldexpq(diff, p);
  cout << "[ulp_error]: " << (long double) approx << " vs " << (long double) exact << endl;
  cout << "    " << (long double) approxSignificand << " : " << (long double) diff << " : " << (long double) ulp << endl; 
  return ulp;
}

double ulp_error_2(double approx, double exact, int p)
{
  if (!isValidFP(approx) || !isValidFP(exact)) {
    return 0;
  }

  int approxExp;
  int exactExp;
  double approxSignificand = frexpq(approx, &approxExp);
  double exactSignificand = frexpq(exact, &exactExp);
  double diff = approxSignificand - ldexpq(exactSignificand, (exactExp-approxExp));
  double ulp = ldexpq(diff, p);
  cout << "[ulp_error]: " << (long double) approx << " vs " << (long double) exact << endl;
  cout << "    " << (long double) approxSignificand << " : " << (long double) exactSignificand << endl;
  cout << "    " << (long double) approxExp << " : " << (long double) exactExp << endl;
  cout << "    " << (long double) diff << " : " << (long double) ulp << endl; 
  cout << "----" << (long double) (ldexpq(approxSignificand, p) - ldexpq(exactSignificand, p)) << endl;
  return ulp;
}


/*
  Functions for Running executables and calculating errors
*/
// return bool to indicate whether the global best configuration need to be updated 
bool 
UpdateRTReport (HFP_TYPE vLP, HFP_TYPE vHP, const char *input_filename, HFP_TYPE &ret_fperr, int lp_bitwidth) {
  HFP_TYPE fperr; 

#ifdef S3FP_VERBOSE 
  cout << "updateRTReport : " << (OUTPUT_TYPE) vLP << " vs " << (OUTPUT_TYPE) vHP << endl;
#endif 

  if (CHECK_UNSTABLE_ERROR && HALT_NOW) return false;

  if (T_FP_ERROR == ABS_FP_ERROR || 
      (T_FP_ERROR == REL_FP_ERROR && vHP != 0) ||
      T_FP_ERROR == ULP_FP_ERROR) {
    // calculate fp error 
    if (T_FP_ERROR == ABS_FP_ERROR) {
      // Absolute error
      fperr = (vLP - vHP);
    }
    else if (T_FP_ERROR == REL_FP_ERROR) {
      // Relative error
      //HFP_TYPE denom = (vHP < 0 ? -vHP : vHP);
      HFP_TYPE denom = (abs(vHP) + abs(vLP)) / 2;
      denom = (denom < 0 ? -denom : denom);
      
      if (denom < REL_DELTA) {
	denom = REL_DELTA;
      }
      fperr = (vLP - vHP) / denom;
    }
    else if (T_FP_ERROR == ULP_FP_ERROR) {
      // ULP error
      // TODO: single precision is used for computing the ULP error,
      // it probably should depend on input_bitwidth
#ifdef S3FP_DDIO
      assert(false && "ERROR: unsupported ULP calculation on dd_real \n");
#else
      assert(lp_bitwidth == 32);
      fperr = ulp_error_2(vLP, vHP, 23);
#endif 
    }
    else assert(false);

    // return the calculated floating-point error 
    ret_fperr = fperr; 

    // If we decide to check unstable error, 
    // decide whether to halt now or not 
    if (CHECK_UNSTABLE_ERROR) {
      if ((vLP > 0 && vHP < 0) || 
	  (vLP < 0 && vHP > 0)) {
	if ((vLP >= UNSTABLE_EPSILON || vLP <= (-1 * UNSTABLE_EPSILON)) && 
	    (vHP >= UNSTABLE_EPSILON || vHP <= (-1 * UNSTABLE_EPSILON))) 
	  HALT_NOW = true;
      }
    }

    bool is_valid_fp = false;
#ifdef S3FP_DDIO 
    is_valid_fp = (isValidFP<double>(fperr.x[0]) && 
		   isValidFP<double>(fperr.x[1]));
#else
    is_valid_fp = isValidFP<HFP_TYPE>(fperr);
#endif 

    if (is_valid_fp) {    
      // cout << "vLP: " << (long double)vLP << "  vs  " << "vHP: " << (long double)vHP << endl;
      N_VALID_RTS++; 
      local_best.addRelerr(vLP, vHP, fperr, input_filename); 
      bool beat_global_best_conf = GLOBAL_BEST.addRelerr(vLP, vHP, fperr, input_filename); 
      if (beat_global_best_conf) {
	cout << "Update Global:  vLP: " << (OUTPUT_TYPE)vLP << "  vs  " << "vHP: " << (OUTPUT_TYPE)vHP << " -- " << (OUTPUT_TYPE) fperr << endl;

	// calculate statistical data
	// ERROR_SUM += fperr; 
	ERROR_SUM += (fperr >= 0 ? fperr : ((-1)*fperr)); 
	ERROR_SUM_2 += (fperr * fperr);
      }

      if (DUMP_ERRORS_STREAM) {
#ifdef S3FP_DDIO
	assert(false && "ERROR : DUMP_ERRORS_STREAM is not supported under double-double type for high precision floating-point numbers \n");
#else 
	// double fperr_to_errors_stream = (double) fperr; 
	double fperr_to_errors_stream = (double) (fperr >= 0 ? fperr : ((-1)*fperr)); 
	fwrite(&fperr_to_errors_stream, sizeof(double), 1, errors_stream);
#endif 	
      }

      return beat_global_best_conf;      
    }
  }

  return false;
}


void* 
threadBody (void* in_myid) {
  unsigned int myid = *((unsigned int*)in_myid);
  unsigned int mypid = TWQ[myid].t_pid;
  unsigned int mytid = TWQ[myid].t_tid;
  
  while (true) {
    pthread_barrier_wait(&TBARR); // await for start 
    
    if (TWQ[myid].t_exit) break;
    
    // generate input file 
    pthread_mutex_lock(&lock_inputgen);
    CONF curr_conf; TWQ[myid].popCONF(curr_conf);
    TWQ[myid].t_eva_basis.prepareInput(curr_conf, 
				       true, mypid, mytid);
    pthread_mutex_unlock(&lock_inputgen);
    
    // run the EXEs
    int lpErr, hpErr;
    vector<HFP_TYPE> vLPs;
    vector<HFP_TYPE> vHPs;
    TWQ[myid].t_eva_basis.runLP(&lpErr, vLPs, true, mypid, mytid);
    TWQ[myid].t_eva_basis.runHP(&hpErr, vHPs, true, mypid, mytid);
    assert(TWQ[myid].t_eva_basis.getNOutputs() == 1);
    assert((vLPs.size() == TWQ[myid].t_eva_basis.getNOutputs()) && 
	   (vHPs.size() == TWQ[myid].t_eva_basis.getNOutputs()));
    
    if (TWQ[myid].t_exit) break;
    
    HFP_TYPE vLP = vLPs[0];
    HFP_TYPE vHP = vHPs[0];

    pthread_mutex_lock(&lock_local_best);
    
    N_RTS++;
    
    // compute the error
    HFP_TYPE ret_fperr; 
    bool update_global_best_conf = false;
    if (lpErr == 0 && hpErr == 0) {
      assert(T_FP_ERROR == ABS_FP_ERROR || 
	     T_FP_ERROR == REL_FP_ERROR ||
	     T_FP_ERROR == ULP_FP_ERROR);
      update_global_best_conf = 
	UpdateRTReport (vLP, vHP, 
			TWQ[myid].t_eva_basis.getInputname(true, mypid, mytid).c_str(), 
			ret_fperr, 
			TWQ[myid].t_eva_basis.getInputBitwidth());
    }

    string my_inputname = TWQ[myid].t_eva_basis.getInputname(true, mypid, mytid);

    if (BACKUP_INPUT_TO_ERRORS) {
      dumpInputOfInputToErrors(file_input_to_errors_local, my_inputname, 
			       TWQ[myid].t_eva_basis.getNInputs(), 
			       true, 
			       TWQ[myid].t_eva_basis.getInputBitwidth());
      dumpErrorOfInputToErrors(file_input_to_errors_local, ret_fperr);
      if (update_global_best_conf) {
	dumpInputOfInputToErrors(file_input_to_errors_global, my_inputname, 
				 TWQ[myid].t_eva_basis.getNInputs(), 
				 true, 
				 TWQ[myid].t_eva_basis.getInputBitwidth());
	dumpErrorOfInputToErrors(file_input_to_errors_global, ret_fperr);
      }
    }
    
    if (update_global_best_conf) {
      GLOBAL_BEST_CONF = curr_conf;
      N_GLOBAL_UPDATES++; 
    }
    
    pthread_mutex_unlock(&lock_local_best);

    pthread_barrier_wait(&TBARR); // finish the work 
  }
}


// return the local best result
// WARNING: when N == 0 -> infinite number of tests!!
RTReport 
grtEvaluateCONF (EvaluationBasis eva_basis, unsigned int N, CONF conf) {
  // query the evaluation cache first 

  local_best.reset();

  if (PARALLELRT) {
    assert(NTPERP == N || N == 0); // we temporary have this limitation.... 
    for (unsigned int n = 0 ; (n < N || N==0) ; n+=NTPERP) {
      // for (unsigned int n = 0 ; (n < N || N==0) ; n++) {
      // check is timeout 
      if (isTimeout(TSTART)) break; 
      else {
	// modify TWQ
	for (unsigned int ti = 0 ; ti < NTPERP ; ti++) {
	  TWQ[ti].pushCONF(conf);
	}
	
	// start threads
	pthread_barrier_wait(&TBARR);
	
	// wait threads
	pthread_barrier_wait(&TBARR);
      }
    }

    // dump conf-to-errors file 
    if (DUMP_CONF_TO_ERRORS) {
      vector<FPR> rels;
      rels.push_back(local_best.best_relerr);
      dumpCONFtoErrors(ost_conf_to_errors, conf, rels);
    }
  }
  else { // PARALLELRT == false
    assert(N % eva_basis.getNOutputs() == 0); 
    for (unsigned int n = 0 ; (n < N || N==0) ; n = n + eva_basis.getNOutputs()) {
      // generate input file 
      eva_basis.prepareInput(conf);
      
      vector<HFP_TYPE> vLPs;
      vector<HFP_TYPE> vHPs;
      int lpErr, hpErr;

      // run the EXEs
      eva_basis.runLP(&lpErr, vLPs);
      eva_basis.runHP(&hpErr, vHPs);
      assert((vLPs.size() == eva_basis.getNOutputs()) && 
	     (vHPs.size() == eva_basis.getNOutputs()));
      
      // check is timeout 
      if (isTimeout(TSTART)) break; // return local_best;
      
      N_RTS += eva_basis.getNOutputs(); 

      // read the output files 
      for (unsigned int oi = 0 ; oi < eva_basis.getNOutputs() ; oi++) {
	HFP_TYPE vLP = vLPs[oi];
	HFP_TYPE vHP = vHPs[oi];

	HFP_TYPE ret_fperr; 
	bool update_global_best_conf = false;     
	if (lpErr == 0 && hpErr == 0) {
	  assert(T_FP_ERROR == ABS_FP_ERROR || 
		 T_FP_ERROR == REL_FP_ERROR ||
		 T_FP_ERROR == ULP_FP_ERROR);
	  update_global_best_conf = 
	    UpdateRTReport (vLP, vHP, 
			    eva_basis.getInputname().c_str(), 
			    ret_fperr, 
			    eva_basis.getInputBitwidth());
	}

	string my_inputname = eva_basis.getInputname();

	if (BACKUP_INPUT_TO_ERRORS) {
	  dumpInputOfInputToErrors(file_input_to_errors_local, my_inputname, 
				   eva_basis.getNInputs(), 
				   true, 
				   eva_basis.getInputBitwidth());
	  dumpErrorOfInputToErrors(file_input_to_errors_local, ret_fperr);
	  if (update_global_best_conf) {
	    dumpInputOfInputToErrors(file_input_to_errors_global, my_inputname, 
				     eva_basis.getNInputs(), 
				     true, 
				     eva_basis.getInputBitwidth());
	    dumpErrorOfInputToErrors(file_input_to_errors_global, ret_fperr);
	  }
	}

	if (update_global_best_conf) {
	  GLOBAL_BEST_CONF = conf;
	  N_GLOBAL_UPDATES++; 
	}
      }
    }

    if (DUMP_CONF_TO_ERRORS) {
      vector<FPR> rels;
      rels.push_back(local_best.best_relerr);
      dumpCONFtoErrors(ost_conf_to_errors, conf, rels);
    }
  }
  
  return local_best;
}


// if nn_to_explore == -1 -->> explore all neighbors 
void IterativeFirstImprovement (CONF conf, CONF &ret, RTReport &ret_report, EvaluationBasis eva_basis, long nn_to_explore) {
  assert(nn_to_explore == -1 || 
	 nn_to_explore > 0);
  ret = conf;
  ret_report = grtEvaluateCONF(eva_basis, N_SEARCH_ITERATIONS, ret);
  vector<CONF> neighbors; 
  if (nn_to_explore < 0) {
    neighborCONFs(neighbors, conf);
    nn_to_explore = neighbors.size();
  }
  else { 
    for (int ni = 0 ; ni < nn_to_explore ; ni++) 
      neighborCONFs(neighbors, conf, true);
    assert(neighbors.size() == nn_to_explore);
  }
  for (unsigned int ni = 0 ; ni < nn_to_explore ; ni++) {
    unsigned int nid = rand() % neighbors.size();
    CONF neighbor_conf = neighbors[nid];    
    RTReport neighbor_report = grtEvaluateCONF(eva_basis, N_SEARCH_ITERATIONS, neighbor_conf);
    if (RTReportCompare(ret_report, neighbor_report) > 0) {
      ret = neighbor_conf;
      ret_report = neighbor_report;
    }
    neighbors.erase(neighbors.begin() + nid);
  }
}


// ---- begin of routines for my algo. ----

void complementCONF (CONF base, CONF dsub, CONF &dret) {
  unsigned int dsize = base.size();
  assert(dret.size() == 0);
  assert(dsize == dsub.size());
  for (unsigned int vi = 0 ; vi < dsize ; vi++) {
    assert(base[vi].first == vi && dsub[vi].first == vi);
    assert(base[vi].second.n_pars == 1 && base[vi].second.index_pars == 0);
    assert(dsub[vi].second.n_pars == 1 && base[vi].second.index_pars == 0);
    if (base[vi].second.ub == dsub[vi].second.ub) {
      assert(base[vi].second.lb <= dsub[vi].second.lb); 
      FPR new_fpr (base[vi].second.lb, dsub[vi].second.lb, 0, 1);
      dret.push_back(pair<unsigned int, FPR>(vi, new_fpr));
    }
    else if (base[vi].second.lb == dsub[vi].second.lb) {
      assert(base[vi].second.ub >= dsub[vi].second.ub);
      FPR new_fpr (dsub[vi].second.ub, base[vi].second.ub, 0, 1);
      dret.push_back(pair<unsigned int, FPR>(vi, new_fpr));
    }
    else {
      cerr << "ERROR: Invalid Arguments of complementCONF" << endl;
      assert(false);
    }
  }
}

// if select == 'r' -> random 
// if select == 'u' -> all upper binary sub. 
// if select == 'l' -> all lower binary sub. 
// otherwise -> error 
void binarySubCONF (CONF base, CONF &dret, char select) {
  assert(dret.size() == 0); 
  assert(select == 'r' || select == 'u' || select == 'l'); 
  unsigned int dsize = base.size();
  for (unsigned int vi = 0 ; vi < dsize ; vi++) {
    assert(base[vi].first == vi);
    HFP_TYPE mb = (base[vi].second.ub + base[vi].second.lb) / 2.0; 
    char uorl = (select == 'r') ? 
      ((rand() % 2 == 0) ? 'u' : 'l') : 
      select ;
    if (uorl == 'l') {
      FPR new_fpr (base[vi].second.lb, mb, 0, 1);
      dret.push_back(pair<unsigned int, FPR>(vi, new_fpr));
    }
    else { // uorl == 'u'
      FPR new_fpr (mb, base[vi].second.ub, 0, 1);
      dret.push_back(pair<unsigned int, FPR>(vi, new_fpr));
    }
  }
}

// will return (n_rand * 2 + 2) confs 
void generateBinarySubCONFs (CONF base, vector<CONF> &ret_confs, unsigned int n_rand) {
  assert(ret_confs.size() == 0);
  CONF all_upper; binarySubCONF(base, all_upper, 'u');
  CONF all_lower; binarySubCONF(base, all_lower, 'l');
  ret_confs.push_back(all_upper);
  ret_confs.push_back(all_lower);

  for (unsigned int di = 0 ; di < n_rand ; di++) {
    CONF new_conf0; binarySubCONF(base, new_conf0, 'r');
    CONF new_conf1; complementCONF(base, new_conf0, new_conf1); 
    ret_confs.push_back(new_conf0);
    ret_confs.push_back(new_conf1);
  }
}

// will try (n_rand * 2 + 2) confs 
void BinaryPartitionImprovement (CONF conf, CONF &ret_conf, EvaluationBasis eva_basis, unsigned int n_rand) {
  vector<CONF> cands; 
  
  generateBinarySubCONFs(conf, cands, n_rand);
  assert(cands.size() == (n_rand * 2 + 2));

  RTReport curr_best = grtEvaluateCONF(eva_basis, N_SEARCH_ITERATIONS, cands[0]);
  ret_conf = cands[0]; 

  for (unsigned int di = 1 ; di < cands.size() ; di++) {
    RTReport this_best = grtEvaluateCONF(eva_basis, N_SEARCH_ITERATIONS, cands[di]);
    if (RTReportCompare(curr_best, this_best) > 0) {
      curr_best = this_best;
      ret_conf = cands[di];
    }
  }
}

// ---- end of routines for my algo. ---- 


bool isLittleEndian() {
  short int n = 0x1;
  char *np = (char*)&n;
  return (np[0] == 1);
}


// return 1 for true
// return 0 for false
template <class FT> 
bool isValidFP (FT in_fp) {
  assert(sizeof(FT)==4 || sizeof(FT)==8 || sizeof(FT)==16);
  if (sizeof(FT) == 4) {
    uint32_t fp;
    memcpy(&fp, &in_fp, 4);
    uint64_t mant = fp & 0x7FFFFF;
    int64_t expo = (fp >> 23) & 0xFF;
    int64_t sign = (((fp >> 23) & 0x100) >> 8);
    if (expo == 255) return false;
  }
  else if (sizeof(FT) == 8) {
    uint64_t fp;
    memcpy(&fp, &in_fp, 8);
    uint64_t mant = fp & 0xFFFFFFFFFFFFF;
    int64_t expo = (fp >> 52) & 0x7FF;
    int64_t sign = (((fp >> 52) & 0x800) >> 11);
    if (expo == 2047) return false;
  }
  else { // sizeof(FT) == 16
    uint64_t fp[2]; 
    memcpy(&fp, &in_fp, 16);
    uint64_t fpM;
    uint64_t fpL;
    if (isLittleEndian()) {
      fpL = fp[0];
      fpM = fp[1];
    }
    else {
      fpM = fp[0];
      fpL = fp[1];
    }
    uint64_t mantM = fpM & 0xFFFFFFFFFFFF;
    int64_t expo = (fpM >> 48) & 0x7FFF;
    int64_t sign = (((fpM >> 48) & 0x8000) >> 15);
    if (expo == 32767) return false;
  }
  return true;
}


void reportResult (const char *tname) {
  cout << "N VALID " << tname << " TESTS: " << N_VALID_RTS << endl;
  cout << "N " << tname << " TESTS: " << N_RTS << endl;
  cout << "N LOCAL UPDATES: " << N_LOCAL_UPDATES << endl;
  cout << "N GLOBAL UPDATES: " << N_GLOBAL_UPDATES << endl;
  if (HALT_NOW) {
    time_t tend; 
    time(&tend);    
    cout << "HALT EARLY: # of SVE : " << N_RTS << endl;
    cout << "HALT EARLY: time : " << (tend - TSTART) << endl;
  }
  GLOBAL_BEST.dumpToStdout();   
  if (DUMP_CONF_TO_ERRORS) 
    GLOBAL_BEST.dumpToOStream(ost_conf_to_errors);
  GLOBAL_BEST.dumpBestInputToFile (BEST_INPUT_FILE_NAME);

  // output statistical data
#ifdef S3FP_DDIO
  HFP_TYPE error_ave = dd_real(0.0);
  HFP_TYPE error_var = dd_real(0.0);
  error_ave = ERROR_SUM / dd_real((double)N_VALID_RTS);
  error_var = (ERROR_SUM_2 / dd_real((double)N_VALID_RTS)) - (error_ave * error_ave);
#else 
  HFP_TYPE error_ave = 0.0;
  HFP_TYPE error_var = 0.0;
  error_ave = ERROR_SUM / (HFP_TYPE)N_VALID_RTS;
  error_var = (ERROR_SUM_2 / (HFP_TYPE)N_VALID_RTS) - (error_ave * error_ave); 
#endif
  cout << "AVERAGE ERROR: " << (OUTPUT_TYPE) error_ave << endl;
  cout << "ERROR VARIANCE: " << (OUTPUT_TYPE) error_var << endl;
}


// ======== begin of plain RT functions =========
void rtPlainRT (EvaluationBasis eva_basis, unsigned int n_vars) {
  CONF plain_conf; randCONF(plain_conf, n_vars, true);
  grtEvaluateCONF(eva_basis, 0, plain_conf);

  reportResult("PLAIN");
}
// ======== end of plain RT functions =========


// ========= begin of BGRT functions ========
void rtBGRT (EvaluationBasis eva_basis, unsigned int n_vars   ) {
  CONF plain_conf; randCONF(plain_conf, n_vars, true);
  CONF curr_subconf = plain_conf;
  
  while (!TerminationCriterion(TSTART)) {
    BinaryPartitionImprovement(curr_subconf, curr_subconf, eva_basis, N_RANDOM_BINARY_PARTITIONS); 

    bool go_restart = false;
    go_restart = toRestart(N_RESTARTS);

    if (go_restart) {
      curr_subconf = plain_conf;    
    }
  }

  reportResult("BGRT");
}
// ========= end of BGRT functions =======


// ======== begin of ILS functions ========
void rtILS (EvaluationBasis eva_basis, unsigned int n_vars ) {
  // init the GLOBAL_BEST_CONF 
  CONF local_conf; randCONF(local_conf, n_vars);
  RTReport local_err = grtEvaluateCONF(eva_basis, N_SEARCH_ITERATIONS, local_conf);

  for (unsigned int r = 0 ; r < ILSr ; r++) {
    CONF curr_conf; randCONF(curr_conf, n_vars);
    RTReport curr_err = grtEvaluateCONF(eva_basis, N_SEARCH_ITERATIONS, curr_conf);
    if (RTReportCompare(local_err, curr_err) > 0) {
      local_conf = curr_conf;
      local_err = curr_err;
    }
  }
  cout << "--- finished the random start ----" << endl;

  IterativeFirstImprovement(local_conf, local_conf, local_err, 
			    eva_basis, ILSifi); // -1);

  cout << "--- ILS finished phase 1 ---- " << endl;
  while (!TerminationCriterion(TSTART)) {
    cout << "--- ILS phase 2 iteration ---- " << endl;
    CONF curr_conf = local_conf;
    RTReport curr_err = local_err;
    vector<CONF> neighbors; 
    for (unsigned int s = 0 ; s < ILSs ; s++) {
      neighbors.clear();
      /*
      neighborCONFs (neighbors, curr_conf); 
      assert(neighbors.size() > 0); 
      curr_conf = neighbors[rand()% neighbors.size()];
      */
      neighborCONFs (neighbors, curr_conf, true);
      assert(neighbors.size() == 1);
      curr_conf = neighbors[0];
    }

    IterativeFirstImprovement(curr_conf, local_conf, local_err, 
			      eva_basis, ILSifi); // -1); 

    if (toRestart(N_RESTARTS)) {
      randCONF(local_conf, n_vars);
    }
  }

  reportResult("ILS"); 
}
// ======== end of ILS functions ========


// ======== begin of PSO functions ========
typedef struct {
  CONF conf;
  HFP_TYPE absolute_best_relerr;
} PARTICLE;
typedef vector<PARTICLE> SWARM; 
SWARM CURR_SWARM;

void dumpSWARMtoStdout (SWARM this_swarm) {
  SWARM::iterator siter = this_swarm.begin();
  for ( ; siter != this_swarm.end() ; siter++) {
    dumpCONFtoStdout(siter->conf);
  }
}

void randSWARM (unsigned int n_particles, unsigned int n_vars) {
  for (int pi = 0 ; pi < n_particles ; pi++) {
    CONF new_conf;
    randCONF(new_conf, n_vars, false);
    PARTICLE new_part;
    new_part.conf = new_conf;
    new_part.absolute_best_relerr = -1;
    CURR_SWARM.push_back(new_part);
  }
}
  
void grtEvaluateSWARM (EvaluationBasis eva_basis, unsigned int N) {
  SWARM next_swarm; 

  // evaluate particles and rank them 
  SWARM::iterator csiter = CURR_SWARM.begin();
  for ( ; csiter != CURR_SWARM.end() ; csiter++) {
    RTReport part_report = grtEvaluateCONF(eva_basis, N, csiter->conf);
    if (isTimeout(TSTART)) return; 
    csiter->absolute_best_relerr = part_report.absolute_best_relerr; 

    SWARM::iterator nsiter = next_swarm.begin();
    for ( ; nsiter != next_swarm.end() ; nsiter++) {
      if (csiter->absolute_best_relerr >= nsiter->absolute_best_relerr) {
	next_swarm.insert(nsiter, (*csiter));
	break;
      }
    }
    if (nsiter == next_swarm.end())
      next_swarm.push_back((*csiter));
  }

  // move particles 
  unsigned int velocity_points = 0; 
  SWARM::iterator nsiter = next_swarm.begin();
  for ( ; nsiter != next_swarm.end() ; nsiter++) {
    for (unsigned int vi = 0 ; vi < velocity_points ; vi++) {
      unsigned int direction = rand() % nsiter->conf.size();
      if (rand() % 2 == 0)
	nsiter->conf[direction].second.index_pars++;
      else 
	nsiter->conf[direction].second.index_pars--;
      if (nsiter->conf[direction].second.index_pars < 0) 
	nsiter->conf[direction].second.index_pars = 0;
      if (nsiter->conf[direction].second.index_pars >= nsiter->conf[direction].second.n_pars)
	nsiter->conf[direction].second.index_pars = nsiter->conf[direction].second.n_pars - 1;
    }
    velocity_points += nsiter->conf.size();
  }
 
  CURR_SWARM.clear();
  CURR_SWARM = next_swarm; 
}

void rtPSO (EvaluationBasis eva_basis, unsigned int n_particles, unsigned int n_vars, unsigned int N) {
  randSWARM(n_particles, n_vars);
  
  while(!TerminationCriterion(TSTART)) {
    grtEvaluateSWARM(eva_basis, N);
  }

  reportResult("PSO");
}
// ======== end of PSO functions ========
  
