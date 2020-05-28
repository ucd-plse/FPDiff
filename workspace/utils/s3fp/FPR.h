#pragma once

#include "definitions.h"
#include "FP_UTILS.h"
using namespace std;

// floating-point range
class FPR {
public:
  HFP_TYPE lb;
  HFP_TYPE ub;
  unsigned int n_pars; // number of partitions
  unsigned int index_pars; // the index of partitions

  FPR () {} // WARNING: this constructor gives you an invalid FPR!!

  FPR (HFP_TYPE inlb, HFP_TYPE inub, unsigned int in_index_pars, unsigned int in_n_pars) {
    assert(inlb <= inub);
    assert(in_n_pars > 0);
    assert(in_index_pars >= 0 && in_index_pars < in_n_pars);
    lb = inlb;
    ub = inub;
    n_pars = in_n_pars;
    index_pars = in_index_pars; 
  }

  FPR & operator = (const FPR &rhs) {
    if (this == &rhs) return *this;
    this->lb = rhs.lb;
    this->ub = rhs.ub;
    this->n_pars = rhs.n_pars;
    this->index_pars = rhs.index_pars;
    return *this;
  }

  HFP_TYPE getLB () const { 
#ifdef S3FP_DDIO
    return lb + (ub - lb) * (dd_real((double)index_pars) / dd_real((double)n_pars)); 
#else 
    return lb + (ub - lb) * ((HFP_TYPE)(index_pars) / (HFP_TYPE)(n_pars)); 
#endif 
  }

  HFP_TYPE getUB () const { 
#ifdef S3FP_DDIO
    return lb + (ub - lb) * (dd_real((double)(index_pars + 1)) / dd_real((double)n_pars)); 
#else
    return lb + (ub - lb) * ((HFP_TYPE)(index_pars + 1) / (HFP_TYPE)(n_pars)); 
#endif 
  }
};

// typdef and function prototype
typedef std::vector<std::pair<unsigned int, FPR> > CONF;

template <class FT>
void sampleCONF (const string &outname, const CONF &conf, const char *fopen_mode="w") {
  FILE *outfile = fopen(outname.c_str(), fopen_mode);
  for (unsigned int vi = 0 ; vi < conf.size() ; vi++) {
    assert(vi == conf[vi].first);

#ifdef S3FP_DDIO
    dd_real frac = (dd_real)(rand() % RSCALE) / (dd_real) RSCALE;
    dd_real this_lb = conf[vi].second.getLB(); 
    dd_real this_ub = conf[vi].second.getUB(); 
    double this_input_double = to_double((this_lb + (this_ub - this_lb) * frac));
    dd_real this_input = (FT) this_input_double;

    fwrite(&this_input, sizeof(dd_real), 1, outfile);

#else 
    HFP_TYPE frac = (HFP_TYPE)(rand() % RSCALE) / (HFP_TYPE) RSCALE;
    HFP_TYPE this_lb = conf[vi].second.getLB(); 
    HFP_TYPE this_ub = conf[vi].second.getUB(); 
    HFP_TYPE this_input = (FT) (this_lb + (this_ub - this_lb) * frac);
    
    fwrite(&this_input, sizeof(HFP_TYPE), 1, outfile);

#endif 
  }

  fclose(outfile);
}
