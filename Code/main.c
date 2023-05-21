#include "syntax.tab.h"
#include "parse_tree.h"
#include "def.h"
#include <stdio.h>
extern int hasError;
extern int prevErrorType;
void check_program();
void lab3();
bool has_struct(struct TreeNode *now);
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
    freopen(argv[2],"w",stdout);
    lab3();
    fclose(stdout);
  }
  return 0;
}