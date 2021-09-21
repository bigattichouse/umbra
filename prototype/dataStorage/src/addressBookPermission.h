struct Permission {
   char* action;
   int permit;
};

extern struct Permission readOnly[];
extern struct Permission readWrite[];
extern struct Permission readWriteExec[];
