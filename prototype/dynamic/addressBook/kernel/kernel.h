
struct umbraQueryKernel {
  int error;
  void *handle;
  float *(*filter)(); //read a record and return a score from 0 to 1 (pass in the "threhold", default 0)
  float *(*compare)(); /*given two records, -1,0,1 for use in sort functions*/
};
