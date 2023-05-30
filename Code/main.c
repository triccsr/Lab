#include "syntax.tab.h"
#include "parse_tree.h"
#include "def.h"
#include <stdio.h>
#include "opt_input.h"
#include "ir.h"
#include "lab5.h"
extern int hasError;
extern int prevErrorType;
void check_program();
void lab3();
bool has_struct(struct TreeNode *now);

void opt(const char *inputFileName,const char *outputFileName){
  struct IRListPair ir=file_to_IRList(inputFileName);
  freopen(outputFileName,"w",stdout);
  print_IR(ir);
  fclose(stdout);
}

int main(int argc, char **argv) {
#ifdef YYDEBUG
    extern int yydebug;
    if(YYDEBUG==1)yydebug=1;
#endif
  if (argc !=3)
    return 1;
  FILE *f = fopen(argv[1], "r");
  if (!f) {
    perror(argv[1]);
    return 1;
  }
  yyrestart(f);
  yyparse();
  if(!hasError){
    //print_tree(programParseTree);
    check_program();
    if(prevErrorType!=0){
      return 0;
    }
    if(has_struct(programParseTree)){
      printf("cannot translate:\n");
      return 0;
    }
    freopen("rawIRFile.ir","w",stdout);
    lab3();
    fclose(stdout);
    lab5_work("rawIRFile.ir",argv[2]);
  }
  return 0;
}

/*int main(int argc,char **argv){
  if(argc!=3){
    return 1;
  } 
  opt(argv[1],argv[2]);
  lab5_work(argc,argv);
  return 0;
}*/
