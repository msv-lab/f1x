#include <stdio.h>
#include "F1X.h"
#include "SearchEngine.h"

int main(int argc, char* argv[]){
  struct C_SearchEngine * engine;
  c_repair_main(argc, argv, &engine);
  printf("current execution progress is : %ld\n", c_temp_getProgress(engine));

  int * locs; int length;
  c_getPatchLoc(engine, &length, &locs);
  for(int i=0; i< length; i++)
    printf("patch location is : %d\n", locs[i]);

  char * test="long";
  //c_temp_getProgress(engine);
  printf("the current number of plausible patches is: %ld\n", c_fuzzPatch(engine, test));
  return 0;
}
