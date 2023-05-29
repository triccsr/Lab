#include <stddef.h>
#include <stdio.h>
#include "cfg.h"
#include "def.h"
#include "opt_g.h"
#include "opt_input.h"
void print_IR(struct IRListPair ir);

void optimize_func_IR(struct IRListPair funcIR){
  struct Cfg cfg=function_IR_to_CFG(funcIR);
  printf("orginal func IR: -------------\n");
  print_IR(funcIR);

  intra_basicblock_optimization(cfg);
  printf("intra basicblock fuc IR: -------------\n");
  print_IR(funcIR);

  useful_variables_optimization(cfg);
  printf("useful variable func IR: -------------\n");
  print_IR(funcIR);
}
int lab5_work(int argc,char **argv){
  if(argc!=3){
    fprintf(stderr,"args error\n");
    return -1;
  }
  struct IRListPair ir=file_to_IRList(argv[1]);
  printf("input IR: -------------\n");
  print_IR(ir);
  struct IRListPair funcIR=(struct IRListPair){NULL,NULL};
  for(struct IRNode *irNode=ir.head;irNode!=NULL;irNode=irNode->nxt){
    if(irNode->irType==IRTYPE_FUNC){
      if(irNode!=ir.head){
        funcIR.tail=irNode->prv;
        optimize_func_IR(funcIR);
      }
      funcIR.head=irNode;
    }
  }
  optimize_func_IR(funcIR);
  if(argc==3){
    freopen(argv[2],"w",stdout);
    print_IR(ir);
    fclose(stdout);
  }
  return 0;
}