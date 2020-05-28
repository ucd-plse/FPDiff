extern "C" {
#include <quadmath.h>
}
#include <stdio.h>

#include <boost/numeric/interval.hpp>
#include <iostream>
#include <string>

using namespace boost::numeric;
using namespace interval_lib;
using namespace std;

typedef interval<double> INTERVAL;

ostream& operator << (ostream &out, const double &x)
{
  char buffer[100];
  quadmath_snprintf(buffer, 100, "%.20Qf", x);
  out << buffer;
}

template <class T>
ostream& operator << (ostream &out, const interval<T> &x)
{
  out << '[' << x.lower() << ',' << x.upper() << ']';
  return out;
}

// Reads a data value from the file
double read_data(const string &fname)
{
  FILE *in = fopen(fname.c_str(), "r");
  if (!in) {
    cout << "File " << fname << " doesn't exist" << endl;
    return 0;
  }

  double data;
  fread(&data, sizeof(double), 1, in);
  fclose(in);

  return data;
}

// Reads an interval from the file
INTERVAL read_interval(const string &fname)
{
  FILE *in = fopen(fname.c_str(), "r");
  if (!in) {
    cout << "File " << fname << " doesn't exist" << endl;
    return INTERVAL(0);
  }

  double data[2];
  fread(&data, sizeof(double), 2, in);
  fclose(in);

  return INTERVAL(data[0], data[1]);
}

int main(int argc, char *argv[])
{
  cout.precision(20);

  // Test that interval functions work.
  // (interval<double> doesn't work and returns a bad interval for this example.)
  INTERVAL x = 3;
  INTERVAL y = 1;
  cout << x << endl;
  cout << (y / x) << endl;

  double rel_delta = 0;
  string fname = "test_output";
  if (argc >= 2) {
    fname = argv[1];
  }

  if (argc >= 3) {
    rel_delta = atof(argv[2]);
  }


  // Read output values
  double data32 = read_data(fname + "_32");
  double data128 = read_data(fname + "_128");
  INTERVAL interval = read_interval(fname + "_interval");

  cout << "data32 = " << data32 << endl;
  cout << "data128 = " << data128 << endl;
  cout << "interval = " << interval << endl;

  // Compute relative errors
  double err128 = (data32 - data128) / data128;
  INTERVAL errI = (INTERVAL(data32, data32) - interval) / interval;

  cout << endl;
  cout << "err128 = " << err128 << endl;
  cout << "errI   = " << errI << endl;
  cout << "errI - err128 = " << (errI - (INTERVAL)err128) << endl;

  double d = data128 < 0 ? -data128 : data128;
  if (d < (double) rel_delta) {
    d = (double) rel_delta;
    if (data128 < 0) {
      d = -d;
    }

    cout << endl;
    cout << "Errors with REL_DELTA = " << rel_delta << endl;

    err128 = (data32 - data128) / d;
    cout << "err128 = " << err128 << endl;
  }

  return 0;
}
