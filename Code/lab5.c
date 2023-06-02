#include <stddef.h>
#include <stdio.h>
#include "cfg.h"
#include "def.h"
#include "opt_g.h"
#include "opt_input.h"
#include "ir.h"
#include "opt_loop.h"
void print_IR(struct IRListPair ir);

void optimize_func_IR(struct IRListPair funcIR){
  freopen("/dev/tty","w",stdout);
  struct Cfg cfg=function_IR_to_CFG(funcIR);
  // printf("orginal func IR: -------------\n");
  // print_IR(funcIR);

  intra_basicblock_optimization(cfg);
  // printf("intra basicblock fuc IR: -------------\n");
  // print_IR(funcIR);

  useful_variables_optimization(cfg);
  // printf("useful variable func IR: -------------\n");
  // print_IR(funcIR);
  loop_optimization(&cfg);
  fclose(stdout);
}
int lab5_work(const char* srcIRFile,const char *dstIRFile){
  struct IRListPair ir=file_to_IRList(srcIRFile);
  //printf("input IR: -------------\n");
  freopen("unoptimized.ir","w",stdout);
  print_IR(ir);
  fclose(stdout);
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
  funcIR.tail=ir.tail;
  optimize_func_IR(funcIR);
  if(dstIRFile!=NULL){
    freopen(dstIRFile,"w",stdout);
    print_IR(ir);
    fclose(stdout);
  }
  return 0;
}