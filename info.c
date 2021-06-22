#include "info.h"

void compute_double_factorials(void)
{
        int_fast32_t i;
        ldfact[0] = 0;
        ldfact[1] = 0;
        ldfact[2] = 1;


        for (i = 3; i != FACT_MAX; i++)
                ldfact[i] = ldfact[i - 2] + log2(i);

        return ;
}

inline double l2rooted(int_fast32_t n_tips) {
        return n_tips < 2L ? 0. : ldfact[n_tips + n_tips - 3];
}

inline double l2unrooted(int_fast32_t n_tips) {
        return n_tips < 3 ? 0. : ldfact[n_tips + n_tips - 5];
}
