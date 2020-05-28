/**
 * Prints out the content of the given input file (e.g., best_input)
 */

#include <stdio.h>
#include <assert.h>
#include <quadmath.h>

int main(int argc, char *argv[])
{
  char *fname = "best_input";

  if (argc >= 2) {
    fname = argv[1];
  }

  FILE *in = fopen(fname, "r");
  if (!in) {
    printf("Input file %s doesn't exist\n", fname);
    return 1;
  }

  // Read input data
  double data;
  int n = 0;
  int i = 0;

  fseek(in, 0, SEEK_END);
  n = ftell(in);
  fseek(in, 0, SEEK_SET);

  assert(n % sizeof(double) == 0);
  n /= sizeof(double);

  for (i = 0; i < n; i++) {
    fread(&data, sizeof(double), 1, in);
    printf("x[%d] = %f\n", i + 1, (double)data);
  }

  fclose(in);
  return 0;
}
