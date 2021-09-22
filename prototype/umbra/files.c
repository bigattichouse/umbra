#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "files.h"

int create_directory(char *path){
  struct stat st = {0};
  if (stat(path, &st) == -1) {
      mkdir(path, 0700);
  }
}

int create_boilerplate (char *path, char *tableName, struct umbraFieldDefs definitions[], int size){
    //printf("Length of array: %d\n", size);

}
