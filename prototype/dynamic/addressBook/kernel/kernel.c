/*This code manages loading the table scan module*/


char *kernelFileName(char *kernelType){
  size_t nbytes = snprintf(NULL, 0, "bin/%s.so", kernelType) + 1; /* +1 for the '\0' */
  char *str = malloc(nbytes);
  snprintf(str, nbytes, "bin/%s.so", kernelType);
  return str;
}

struct umbraQueryKernel loadKernel(char *kernelType){
     struct umbraQueryKernel loaded;
     loaded.page = page;
     char *filename = kernelFileName(kernelType);
     loaded.handle = dlopen(filename, RTLD_LAZY);
     free(filename);
     if (!loaded.handle) {
          /* fail to load the library */
          fprintf(stderr, "Error: %s\n", dlerror());
          loaded.error = EXIT_FAILURE;
          return loaded;
     }

      *(float **)(&loaded.filter) = dlsym(loaded.handle, "filter");
       if (!loaded.filter) {
           /* no such symbol */
           fprintf(stderr, "Error: %s\n", dlerror());
           dlclose(loaded.handle);
           loaded.error = EXIT_FAILURE;
           return loaded;
       }

       *(float **)(&loaded.compare) = dlsym(loaded.handle, "compare");
        if (!loaded.compare) {
            /* no such symbol */
            fprintf(stderr, "Error: %s\n", dlerror());
            dlclose(loaded.handle);
            loaded.error = EXIT_FAILURE;
            return loaded;
        }
  return loaded;
}

int unloadKernel(struct umbraQueryKernel kernel){
      return dlclose(kernel.handle);
}
