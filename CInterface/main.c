/* this file shows an example that invoking f1x using the c interface.
 * Those c interfaces show be invoked inside aflgo
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "F1X.h"
#include "SearchEngine.h"
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>

int main(int argc, char* argv[]){

  struct C_SearchEngine * engine;
  //run original f1x
  c_repair_main(argc, argv, &engine);

  if(engine != NULL){
    //get all the locations of plausible patches
    int * locs; int length;
    c_getPatchLoc(engine, &length, &locs);

    FILE * fp;
    fp = fopen("location.txt", "w");
    if (fp == NULL){
      exit(1);
    }
    for(int i=0; i< length; i++){
      //encode location to one string("loc1", "loc2", "loc3"), which will be used in aflgo
      fprintf(fp, "%d ", locs[i]);
    }
    fprintf(fp, "\n");
    fclose(fp);
  }
  return 0;
}
