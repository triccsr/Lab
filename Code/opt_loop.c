#include "bitset.h"
#include <bits/stdint-uintn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stddef.h>
#include "cfg.h"
#include "def.h"
#include "static_analysis.h"
#include "list.h"
#include "bitset.h"
#include "ir.h"
#include "opt_input.h"
void dominate_init_entry_out(struct Cfg cfg,struct CfgNode* cfgNode){
  cfgNode->dominatedBitSet=calloc(get_bitset_len(cfg.size),sizeof(uint64_t));
  bitset_modify(cfgNode->dominatedBitSet,cfgNode->index,1);
}
void dominate_init(struct Cfg cfg,struct CfgNode* cfgNode){
  cfgNode->dominatedBitSet=calloc(get_bitset_len(cfg.size),sizeof(uint64_t));
  for(int i=0;i<get_bitset_len(cfg.size);++i){
    cfgNode->dominatedBitSet[i]=(uint64_t)-1;
  }
}

bool dominate_update_in(struct Cfg cfg,struct CfgNode* cfgNode){
  int len=get_bitset_len(cfg.size);
  bool modified=false;
  for(struct CfgEdge *e=cfgNode->firstInEdge;e!=NULL;e=e->next){
    if(e->isDeleted)continue;
    for(int i=0;i<len;++i){
      uint64_t tmp=cfgNode->dominatedBitSet[i];
      cfgNode->dominatedBitSet[i]&=e->to->dominatedBitSet[i];
      bitset_modify(cfgNode->dominatedBitSet,cfgNode->index,1);
      if(cfgNode->dominatedBitSet[i]!=tmp){
        modified=true;
      }
    }
  }
  return modified;
}
void dominate_update_out_from_in(struct Cfg cfg,struct CfgNode* cfgNode){
  bitset_modify(cfgNode->dominatedBitSet,cfgNode->index,1);
}

void get_dominated_sets(struct Cfg cfg){
  freopen("/dev/tty","w",stdout);
  forward_analysis(cfg,dominate_init,dominate_init_entry_out,dominate_update_in,dominate_update_out_from_in);
  /*for(struct CfgNode* cfgNode=cfg.entry;cfgNode!=cfg.exit->nextCfgNode;cfgNode=cfgNode->nextCfgNode){
    printf("%p index:%d, prev:%p next:%p\n",cfgNode,cfgNode->index,cfgNode->prevCfgNode,cfgNode->nextCfgNode);
    printf("Code=\n");
    print_IR(cfgNode->basicBlock);
    printf("\n");
    printf("outEdges:");
    for(struct CfgEdge *e=cfgNode->firstOutEdge;e!=NULL;e=e->next){
      if(e->isDeleted)continue;
      printf("(%p,%d)",e->to,e->to->index);
    }
    printf("\n");
    printf("inEdges:");
    for(struct CfgEdge *e=cfgNode->firstInEdge;e!=NULL;e=e->next){
      if(e->isDeleted)continue;
      printf("(%p,%d)",e->to,e->to->index);
    }
    printf("\n");
    printf("dominatedBy:");
    for(int i=0;i<get_bitset_len(cfg.size);++i){
      for(int j=0;j<63;++j){
        if(cfgNode->dominatedBitSet[i]&(1ull<<j)){
          printf("%d,",i*64+j);
        }
      }
    }
    printf("\n----------------\n");
  }*/
  fclose(stdout);
}

bool is_dominated_by(struct CfgNode* slave,struct CfgNode *master){
  if(master==NULL||master->dominatedBitSet==NULL){
    return false;
  }
  if(slave==NULL||slave->dominatedBitSet==NULL){//cannot reach slave
    return true;
  }
  return bitset_get(slave->dominatedBitSet,master->index);
}

bool ir_use_var(struct IRNode *ir,int varIndex){
  switch (ir->irType) {
    case IRTYPE_ASSIGN:
    case IRTYPE_VL:
      // list_append(defOfVars[ir->x.val],(void*)ir);
      // list_append(useOfVars[ir->y.val],(void*)ir);
      return varIndex==ir->y.val; 
      break;
    case IRTYPE_PLUS:
    case IRTYPE_MINUS:
    case IRTYPE_MUL:
    case IRTYPE_DIV:
      // list_append(defOfVars[ir->x.val],(void*)ir);
      // list_append(useOfVars[ir->y.val],(void*)ir);
      // list_append(useOfVars[ir->z.val],(void*)ir);
      return (varIndex==ir->y.val)||(varIndex==ir->z.val);
      break;
    case IRTYPE_LV:
    case IRTYPE_IFGOTO:
      // list_append(useOfVars[ir->x.val],ir);
      // list_append(useOfVars[ir->y.val],ir);
      return (varIndex==ir->x.val)||(varIndex==ir->y.val);
      break;
    case IRTYPE_CALL:
    case IRTYPE_ARG:
    case IRTYPE_WRITE:
    case IRTYPE_RETURN:
      // list_append(useOfVars[ir->x.val],ir);
      return (varIndex==ir->x.val);
      break;
    case IRTYPE_FUNC:
    case IRTYPE_READ:
    case IRTYPE_PARAM:
      // list_append(defOfVars[ir->x.val],ir);
      return false;
    default:
      return false;
  }
}

void optimize_a_loop(struct Cfg* cfg,struct CfgNode* loopStart,struct CfgNode *loopEnd){
  // x is only defined once in loop
  // the defination of x dominates all usage of x in loop
  // the defination of x dominates all exits of loop
  struct List  *defOfVars=calloc(cfg->defVarMaxIndex+3,sizeof(struct List));
  struct List exits=new_list();
  struct List *useOfVars=calloc(cfg->defVarMaxIndex+3,sizeof(struct List));
  uint64_t *cfgNodesInLoop=calloc(get_bitset_len(cfg->defVarMaxIndex+3),sizeof(uint64_t));
  //get cfgNode in loop, defs,uses
  for(struct CfgNode *cfgNode=loopStart;cfgNode!=loopEnd->nextCfgNode;cfgNode=cfgNode->nextCfgNode){
    bitset_modify(cfgNodesInLoop,cfgNode->index,1);
    for(struct IRNode *ir=cfgNode->basicBlock.head;ir!=cfgNode->basicBlock.tail->nxt;ir=ir->nxt){
      if(ir->deleted)continue;
      switch (ir->irType) {
        case IRTYPE_ASSIGN:
        case IRTYPE_VL:
          if(ir->x.oprType==IROPR_DEFVAR)
            list_append(&defOfVars[ir->x.val],(void*)ir);
          if(ir->y.oprType==IROPR_DEFVAR)
            list_append(&useOfVars[ir->y.val],(void*)ir);
          break;
        case IRTYPE_PLUS:
        case IRTYPE_MINUS:
        case IRTYPE_MUL:
        case IRTYPE_DIV:
          if(ir->x.oprType==IROPR_DEFVAR)
            list_append(&defOfVars[ir->x.val],(void*)ir);
          if(ir->y.oprType==IROPR_DEFVAR)
          list_append(&useOfVars[ir->y.val],(void*)ir);
          if(ir->z.oprType==IROPR_DEFVAR)
          list_append(&useOfVars[ir->z.val],(void*)ir);
          break;
        case IRTYPE_LV:
        case IRTYPE_IFGOTO:
          if(ir->x.oprType==IROPR_DEFVAR)
            list_append(&useOfVars[ir->x.val],ir);
          if(ir->y.oprType==IROPR_DEFVAR)
            list_append(&useOfVars[ir->y.val],ir);
          break;
        case IRTYPE_CALL:
        case IRTYPE_ARG:
        case IRTYPE_WRITE:
        case IRTYPE_RETURN:
          if(ir->x.oprType==IROPR_DEFVAR)
            list_append(&useOfVars[ir->x.val],ir);
          break;
        case IRTYPE_FUNC:
        case IRTYPE_READ:
        case IRTYPE_PARAM:
          if(ir->x.oprType==IROPR_DEFVAR)
            list_append(&defOfVars[ir->x.val],ir);
          break; 
        default:
          break;
      }
    }
  }
  //get exits
  for(struct CfgNode* cfgNode=loopStart;cfgNode!=loopEnd->nextCfgNode;cfgNode=cfgNode->nextCfgNode){
    bool isExit=false;
    for(struct CfgEdge *e=cfgNode->firstOutEdge;e!=NULL;e=e->next){
      if(e->isDeleted)continue;
      if(!bitset_get(cfgNodesInLoop,e->to->index)){
        isExit=true;
        break;
      }
    }
    if(isExit){
      list_append(&exits,(void*)cfgNode);
    }
  }
  struct List moveFrontIRs=new_list();
  for(int var=0;var<=cfg->defVarMaxIndex;++var){
    if(defOfVars[var].size==1){
      struct IRNode *ir=(struct IRNode*)defOfVars[var].listHead->infoPtr;
      if(ir->deleted==true||ir->irType==IRTYPE_FUNC||ir->irType==IRTYPE_READ||ir->irType==IRTYPE_PARAM){
        continue;
      }
      bool ok=1;
      // dominate all exits
      for(struct ListNode *exit=exits.listHead;exit!=NULL;exit=exit->next){
        if(!is_dominated_by((struct CfgNode*)exit->infoPtr,ir->inCfgNode)){
          ok=0;
          break;
        }
      }
      if(ok==0)continue;
      //dominate all uses
      for(struct IRNode* tmpIR=ir->prv;tmpIR->inCfgNode==ir->inCfgNode;tmpIR=tmpIR->prv){
        if(ir_use_var(tmpIR,var)){
          ok=0;
          break;
        }
      }
      if(ok==0)continue;
      for(struct ListNode *use=useOfVars[var].listHead;use!=NULL;use=use->next){
        if(!is_dominated_by(((struct IRNode*)use->infoPtr)->inCfgNode,ir->inCfgNode)){
          ok=0;
          break;
        }
      }
      if(ok==0)continue;
      switch (ir->irType) {
        case IRTYPE_ASSIGN:
        case IRTYPE_VL:{
          if(ir->y.oprType==IROPR_NUM||defOfVars[ir->y.val].size==0){
            list_append(&moveFrontIRs,ir);
          }
          break;
        }
        case IRTYPE_PLUS:
        case IRTYPE_MINUS:
        case IRTYPE_MUL:
        case IRTYPE_DIV:
          if((ir->y.oprType==IROPR_NUM||defOfVars[ir->y.val].size==0)&&(ir->z.oprType==IROPR_NUM||defOfVars[ir->z.val].size==0)){
            list_append(&moveFrontIRs,ir);
          }
          break;
        default:
          break;
      } 
    }
  }
  if(moveFrontIRs.size>0){
    struct CfgNode *newCfgNode=new_CfgNode(cfg);

    struct IRNode *newLabelIRNode=calloc(1,sizeof(struct IRNode));
    newLabelIRNode->inCfgNode=newCfgNode;
    newLabelIRNode->argNum=1;
    newLabelIRNode->deleted=false;
    newLabelIRNode->irType=IRTYPE_LABEL;
    newLabelIRNode->nxt=newLabelIRNode->prv=NULL;
    newLabelIRNode->x=lab5_new_label();
    cfg->labelNum+=1;

    struct IRNode *newIRHead=newLabelIRNode,*newIRTail=newLabelIRNode;
    bool inserted=false;
    /*for(struct IRNode *ir=loopStart->basicBlock.head;ir!=loopStart->basicBlock.tail->nxt;ir=ir->nxt){
      if(ir->irType!=IRTYPE_LABEL){//insert in front of ir
        inserted=true;
        newIRHead->prv=ir->prv;
        ir->prv->nxt=newIRHead;
        newIRTail->nxt=ir;
        ir->prv=newIRTail;
        break;
      }
    }
    if(!inserted){//then insert them behind tail
      newIRTail->nxt=loopStart->basicBlock.tail->nxt;
      loopStart->basicBlock.tail->nxt->prv=newIRTail;
      loopStart->basicBlock.tail->nxt=newIRHead;
      newIRHead->prv=loopStart->basicBlock.tail;
    }*/
    for(struct ListNode *listNode=moveFrontIRs.listHead;listNode!=NULL;listNode=listNode->next){
      struct IRNode *ir=(struct IRNode*)listNode->infoPtr;
      struct IRNode *newIR=calloc(1,sizeof(struct IRNode));
      memcpy(newIR,ir,sizeof(struct IRNode)); 
      ir->deleted=true;
      newIR->inCfgNode=newCfgNode;
      //newIR->inCfgNode=loopStart;
      newIR->deleted=false;
      if(newIRHead==NULL){
        newIR->nxt=newIR->prv=NULL;
        newIRHead=newIRTail=newIR;
      }
      else{
        newIR->nxt=NULL;
        newIR->prv=newIRTail;
        newIRTail->nxt=newIR;
        newIRTail=newIR;
      }
    }
    //cat IR
    newIRHead->prv=loopStart->basicBlock.head->prv;
    if(loopStart->basicBlock.head->prv!=NULL){
      loopStart->basicBlock.head->prv->nxt=newIRHead;
    }
    newIRTail->nxt=loopStart->basicBlock.head;
    loopStart->basicBlock.head->prv=newIRTail;
    //cat CFGNode
    newCfgNode->basicBlock.head=newIRHead;
    newCfgNode->basicBlock.tail=newIRTail;
    if(loopStart->prevCfgNode!=NULL){
      loopStart->prevCfgNode->nextCfgNode=newCfgNode;
    }
    newCfgNode->prevCfgNode=loopStart->prevCfgNode;
    newCfgNode->nextCfgNode=loopStart;
    loopStart->prevCfgNode=newCfgNode;
    //CfgEdges
    for(struct CfgEdge *e=loopStart->firstInEdge;e!=NULL;e=e->next){
      if(e->isDeleted)continue;
      struct CfgNode *pred=e->to;
      if(bitset_get(cfgNodesInLoop,pred->index)==0){//pred is not in loop
        for(struct CfgEdge *re=pred->firstOutEdge;re!=NULL;re=re->next){
          if(re->to==loopStart){
            re->isDeleted=true;
          }
        }
        e->isDeleted=true;
        add_cfg_edge(pred,newCfgNode);
        for(struct IRNode *jIR=pred->basicBlock.tail;jIR->irType==IRTYPE_GOTO||jIR->irType==IRTYPE_IFGOTO;jIR=jIR->prv){// all jumps of pred cfgNode
          if(jIR->irType==IRTYPE_GOTO){
            for(struct IRNode *lIR=loopStart->basicBlock.head;lIR->irType==IRTYPE_LABEL;lIR=lIR->nxt){
              if(jIR->x.val==lIR->x.val){
                jIR->x.val=newLabelIRNode->x.val;
                break;
              }
            }
          }
          else{//IFGOTO
            for(struct IRNode *lIR=loopStart->basicBlock.head;lIR->irType==IRTYPE_LABEL;lIR=lIR->nxt){
              if(jIR->z.val==lIR->x.val){
                jIR->z.val=newLabelIRNode->x.val;
                break;
              }
            }
          }
        }
      }
    }
    add_cfg_edge(newCfgNode,loopStart);
  }
  
}
void loop_optimization(struct Cfg *cfg){
  get_dominated_sets(*cfg);
  uint64_t *cfgNodeSet=calloc(cfg->size+3,sizeof(uint64_t));
  for(struct CfgNode *cfgNode=cfg->entry;cfgNode!=NULL;cfgNode=cfgNode->nextCfgNode){
    for(struct CfgEdge *e=cfgNode->firstOutEdge;e!=NULL;e=e->next){
      if(e->isDeleted)continue;
      if(bitset_get(cfgNodeSet,e->to->index)==1){
        optimize_a_loop(cfg,e->to,cfgNode);
      }
    }
    bitset_modify(cfgNodeSet,cfgNode->index,1);
  }   
}