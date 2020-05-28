#include "definitions.h"
#include "EvaluationBasis.h"
#include "S3FP_ParseArgs.h"

using namespace std;

// Global Variables 
ENUM_RT_MODE RT_MODE = NA_RT_MODE;
unsigned int TIMEOUT = 0;
ENUM_TRESOURCE T_RESOURCE = NA_TRESOURCE;
unsigned int RSEED = 0;
bool PARALLELRT = true;
ENUM_FP_ERROR T_FP_ERROR = NA_FP_ERROR;
HFP_TYPE REL_DELTA = 0.0;
bool UNIFORM_INPUT = true;
string INPUT_RANGE_FILE;
HFP_TYPE UNIFORM_INPUTLB = 0.0;
HFP_TYPE UNIFORM_INPUTUB = 0.0; 
bool DUMP_CONF_TO_ERRORS = false; 
string CONF_TO_ERRORS_FILE = "";
bool CHECK_UNSTABLE_ERROR = false;
unsigned int N_INPUT_REPEATS = 1;
unsigned int N_OUTPUTS = 1;
bool BACKUP_INPUT_TO_ERRORS = false;
string INPUT_TO_ERRORS_FILE = ""; 
bool DUMP_ERRORS_STREAM = false;
string ERRORS_STREAM_FILE = "";
HFP_TYPE ERROR_SUM = 0.0;
HFP_TYPE ERROR_SUM_2 = 0.0;
HFP_TYPE ERROR_VAR = 0.0;

#ifdef S3FP_DDIO
#define OUTPUT_TYPE dd_real
#else 
#define OUTPUT_TYPE long double
#endif 


/* 
   class definitions
*/

// report of random testing 
class RTReport {
public: 
  unsigned int n_tests; 
  bool has_best_relerr;
  FPR best_relerr; 
  HFP_TYPE absolute_best_relerr; 
  HFP_TYPE best_relerr_ub_vlp; 
  HFP_TYPE best_relerr_ub_vhp; 
  HFP_TYPE best_relerr_lb_vlp; 
  HFP_TYPE best_relerr_lb_vhp; 

  vector<HFP_TYPE> best_input;
  RTReport () { has_best_relerr = false; n_tests = 0; }
  void reset () {
    n_tests = 0;
    has_best_relerr = false;
    absolute_best_relerr = -1;
    best_input.clear();
  }
  // return true if the absolute_best_relerr was update!!
  bool addRelerr (HFP_TYPE vlp, HFP_TYPE vhp, HFP_TYPE rel, const char *input_name) {

    HFP_TYPE absrel = (rel >= 0) ? rel : (-1 * rel);
    n_tests++;
    if (has_best_relerr == false) {
      has_best_relerr = true;
      FPR in_best_relerr(rel, rel, 0, 1);
      best_relerr = in_best_relerr;
      best_relerr_ub_vlp = best_relerr_lb_vlp = vlp;
      best_relerr_ub_vhp = best_relerr_lb_vhp = vhp;
      absolute_best_relerr = absrel;
      saveTheBestInput(input_name);
      return true;
    }
    else {
      bool ret = false;
      if (absrel > absolute_best_relerr) {
	absolute_best_relerr = absrel;
	saveTheBestInput(input_name);
	ret = true;
      }
      if (rel > best_relerr.ub) {
	best_relerr_ub_vlp = vlp;
	best_relerr_ub_vhp = vhp;
	best_relerr.ub = rel;
      }
      if (rel < best_relerr.lb) {
	best_relerr_lb_vlp = vlp;
	best_relerr_lb_vhp = vhp;
	best_relerr.lb = rel;
      }
      return ret; 
    }
  }
  RTReport &operator=(const RTReport &rhs) {
    if (this == &rhs) return *this;
    this->has_best_relerr = rhs.has_best_relerr;
    this->best_relerr = rhs.best_relerr;
    this->best_relerr_lb_vlp = rhs.best_relerr_lb_vlp;
    this->best_relerr_lb_vhp = rhs.best_relerr_lb_vhp;
    this->best_relerr_ub_vlp = rhs.best_relerr_ub_vlp;
    this->best_relerr_ub_vhp = rhs.best_relerr_lb_vhp;
    this->absolute_best_relerr = rhs.absolute_best_relerr;
    return *this;
  }
  void dumpToOStream (ostream &ost) {
    // ost << endl;
    if (has_best_relerr) {
      ost << "Best Relative Error: " << (OUTPUT_TYPE) absolute_best_relerr << " [" << (OUTPUT_TYPE) best_relerr.lb << ", " << (OUTPUT_TYPE) best_relerr.ub << "]" << endl;
      ost << "vLP : vHP when Rel. Error = " << (OUTPUT_TYPE) best_relerr.lb << " :: " << (OUTPUT_TYPE) best_relerr_lb_vlp << " : " << (OUTPUT_TYPE) best_relerr_lb_vhp << endl;
      ost << "vLP : vHP when Rel. Error = " << (OUTPUT_TYPE) best_relerr.ub << " :: " << (OUTPUT_TYPE) best_relerr_ub_vlp << " : " << (OUTPUT_TYPE) best_relerr_ub_vhp << endl;
    }
    else ost << "Best Relative Error: N/A" << endl;
  }
  void dumpToStdout () {
    dumpToOStream(cout);
  } 
  void dumpBestInputToFile (const char *outname) {
    assert(outname != NULL);
    FILE *outfile = fopen(outname, "w");
    assert(outfile != NULL);
    for (unsigned int ii = 0 ; ii < best_input.size() ; ii++) {
      HFP_TYPE out_data = best_input[ii];
      fwrite(&out_data, sizeof(HFP_TYPE), 1, outfile);
    }
    fclose(outfile); 
  }
private:
  HFP_TYPE absoluteRelerr (FPR relerr) {
    HFP_TYPE absub = (relerr.ub >= 0) ? relerr.ub : (-1 * relerr.ub);
    HFP_TYPE abslb = (relerr.lb >= 0) ? relerr.lb : (-1 * relerr.lb);
    return (absub >= abslb) ? absub : abslb;
  }
  void saveTheBestInput (const char *input_name) {
    if (input_name == NULL) return; 
    best_input.clear();
    FILE *infile = fopen(input_name, "r");
    fseek(infile, 0, SEEK_END);
    unsigned int n_inputs = ftell(infile);
    assert(n_inputs % sizeof(HFP_TYPE) == 0);
    n_inputs = n_inputs / sizeof(HFP_TYPE);
    fseek(infile, 0, SEEK_SET);
    for (unsigned int ii = 0 ; ii < n_inputs ; ii++) {
      HFP_TYPE this_input;
      fread(&this_input, sizeof(HFP_TYPE), 1, infile);
      best_input.push_back(this_input);
    }
    fclose(infile);
  } 
}; 




class ThreadWorkQueue {
 public:
  EvaluationBasis t_eva_basis; 
  unsigned int t_N;
  vector<CONF> t_confs;
  bool t_use_cache;
  bool t_cache_local;
  pthread_mutex_t t_lock_exit;
  bool t_exit;
  unsigned int t_pid;
  unsigned int t_tid;

  ThreadWorkQueue () {}

  ThreadWorkQueue (EvaluationBasis in_eva_basis, unsigned int in_N, bool in_use_cache, bool in_cache_local, bool in_exit, unsigned int in_pid, unsigned int in_tid) {
    init(in_eva_basis, in_N, in_use_cache, in_cache_local, in_exit, 
	 in_pid, in_tid);
  }

  void init (EvaluationBasis in_eva_basis, unsigned int in_N, bool in_use_cache, bool in_cache_local, bool in_exit, unsigned int in_pid, unsigned int in_tid) {
    t_eva_basis = in_eva_basis;
    t_N = in_N;
    t_use_cache = in_use_cache;
    t_cache_local = in_cache_local;
    t_exit = in_exit;
    t_pid = in_pid;
    t_tid = in_tid;
  }

  void pushCONF (CONF in_conf) {
    CONF new_conf = in_conf;
    t_confs.push_back(new_conf);
  }

  void popCONF (CONF &ret_conf) {
    assert(t_confs.size() > 0);
    ret_conf = t_confs.front();
    t_confs.erase(t_confs.begin());
  }
};


/* 
   subroutines
*/
void startTimer(time_t *ts) { time(ts); }

bool toRestart (unsigned int &restart_counter) {
  const unsigned int base = 10;

  if (((float)(rand() % base) / (float)base) < RRESTART) {
    restart_counter++;
    return true;
  }
  return false;
}

void rmFile (const string &fname) {
  stringstream ss; 
#ifdef QUIET_RM
  FILE *temp_open = fopen(fname.c_str(), "r");
  if (temp_open == NULL) return;
  else fclose(temp_open);
#endif 
  ss << "rm " << fname;
  system(ss.str().c_str());
  return; 
}

string CONFtoString (CONF conf) {
  assert(conf.size() > 0);
  stringstream ss;
  assert(conf[0].first == 0);
  ss << 0 << ":" << conf[0].second.index_pars << "/" << conf[0].second.n_pars;
  for (unsigned int vi = 1 ; vi < conf.size() ; vi++) {
    assert(conf[vi].first == vi);
    ss << ", " << vi << ":" << conf[vi].second.index_pars << "/" << conf[vi].second.n_pars;
  }
  return ss.str();
}  

void dumpCONFtoStdout (CONF conf) {
  cout << "---- conf size: " << conf.size() << " ----" << endl;
  for (unsigned int di = 0 ; di < conf.size() ; di++) 
    cout << conf[di].first << " : [" << (OUTPUT_TYPE) conf[di].second.getLB() << ", " << (OUTPUT_TYPE) conf[di].second.getUB() << "]" << endl;
  cout << "------------------------------------" << endl;
}
