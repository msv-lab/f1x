#include <stdlib.h>
#include "runtime.h"

int __f1x_id = -1; /* -1 means disabled */

void __attribute__ ((constructor)) __f1x_init() {
  __f1x_id = atoi(getenv("F1X_ID"));
}

