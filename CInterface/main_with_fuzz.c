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
#include <dirent.h>

int main(int argc, char* argv[]){

  struct C_SearchEngine * engine;
  //run original f1x
  c_repair_main(argc, argv, &engine);

  if(engine != NULL){
    //get all the locations of plausible patches
    int * locs; int length;
    c_getPatchLoc(engine, &length, &locs);
    char location[4096]="";
    for(int i=0; i< length; i++){
      //encode location to one string("loc1", "loc2", "loc3"), which will be used in aflgo
      sprintf(location, "%s\"%d\",", location, locs[i]);
    }
    printf("locations of plausible patches are : %s\n", location);
    printf("the working directory is : %s\n", c_getWorkingDir(engine));
    //invoke aflgo to generate new test
    pid_t id = fork();

    if(id == 0)
    {
      char *argv[] = { "executeAFLGO", location, c_getWorkingDir(engine), NULL };
      execvp("executeAFLGO", argv);
      printf("Children Done!!!");
      exit(0);
    } else {
      waitpid(id, NULL, 0);

      DIR *dp;
      struct dirent *ep;
      dp = opendir ("/out2/ef709ce2/interestedTest/");
      if (dp != NULL)
      {
        while (ep = readdir (dp))
          //given one test, c_fuzzPatch will return the current partition size.
          printf("the current number of plausible patches is: %ld\n", c_fuzzPatch(engine, ep->d_name));
        (void) closedir (dp);
      }
      else
        perror ("Couldn't open the directory");
      printf("DONE\n");
    }
  }
  return 0;
}
