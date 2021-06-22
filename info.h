#ifndef INFORMATION_H
#define INFORMATION_H

#include <math.h> // for log2
#include <stdint.h>

#define FACT_MAX 1000000

double ldfact[FACT_MAX];
void compute_double_factorials(void);
inline double l2rooted(int_fast32_t n_tips);
inline double l2unrooted(int_fast32_t n_tips);

#endif
