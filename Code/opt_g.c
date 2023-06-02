#include "def.h"
#include "cfg.h"
#include <bits/stdint-uintn.h>
#include <stdlib.h>
#include <stdbool.h>
#include "static_analysis.h"
#include <assert.h>
#include "bitset.h"

void useful_variables_init(struct Cfg cfg,struct CfgNode* cfgNode){
  cfgNode->outInfoPtr.usefulVarBitSet=calloc(get_bitset_len(cfg.defVarMaxIndex+cfg.tmpVarMaxIndex+2),sizeof(uint64_t));
  cfgNode->inInfoPtr.usefulVarBitSet=calloc(get_bitset_len(cfg.defVarMaxIndex+cfg.tmpVarMaxIndex+2),sizeof(uint64_t));
}
bool useful_variables_update_out(struct Cfg cfg,struct CfgNode* cfgNode){
  bool diff=false;
  int bitsetLen=get_bitset_len(cfg.defVarMaxIndex+cfg.tmpVarMaxIndex+2);
  for(struct CfgEdge *e=cfgNode->firstOutEdge;e!=NULL;e=e->next){
    for(int i=0;i<bitsetLen;++i){
      uint64_t tmp=cfgNode->outInfoPtr.usefulVarBitSet[i];
      cfgNode->outInfoPtr.usefulVarBitSet[i]|=e->to->inInfoPtr.usefulVarBitSet[i];
      if(tmp!=cfgNode->outInfoPtr.usefulVarBitSet[i]){
        diff=true;
      }
    }
  }
  return diff;
}
void useful_variables_update_in_from_out(struct Cfg cfg,struct CfgNode* cfgNode){
  int bitsetLen=get_bitset_len(cfg.defVarMaxIndex+cfg.tmpVarMaxIndex+2);
  memcpy(cfgNode->inInfoPtr.usefulVarBitSet,cfgNode->outInfoPtr.usefulVarBitSet,bitsetLen*sizeof(uint64_t));
  uint64_t *inSet=cfgNode->inInfoPtr.usefulVarBitSet;
  for(struct IRNode *ir=cfgNode->basicBlock.tail;ir!=cfgNode->basicBlock.head->prv;ir=ir->prv){
    switch (ir->irType) {
      case IRTYPE_ARG:
      case IRTYPE_WRITE:
      case IRTYPE_RETURN:
        if(ir->x.oprType==IROPR_DEFVAR){
          bitset_modify(inSet,ir->x.val,1);
        }
        break;
      case IRTYPE_CALL:
      case IRTYPE_READ:
      case IRTYPE_DEC:
      case IRTYPE_PARAM:
        if(ir->x.oprType==IROPR_DEFVAR)
          bitset_modify(inSet,ir->x.val,0);
        break;
      case IRTYPE_IFGOTO:
        if(ir->x.oprType==IROPR_DEFVAR)
          bitset_modify(inSet,ir->x.val,1);
        if(ir->y.oprType==IROPR_DEFVAR)
          bitset_modify(inSet,ir->y.val,1);
        break;
      case IRTYPE_ASSIGN:// x=y,x=&y
      case IRTYPE_VL: //x=*y
        if(ir->x.oprType==IROPR_DEFVAR&&bitset_get(inSet,ir->x.val)==1){//x is useful
          ir->deleted=false;
          bitset_modify(inSet,ir->x.val,0);
          if(ir->y.oprType==IROPR_DEFVAR)
            bitset_modify(inSet,ir->y.val,1);
        }
        else{
          ir->deleted=true;
        }
        break;
      case IRTYPE_LV://*x=y
        if(ir->x.oprType==IROPR_DEFVAR)
          bitset_modify(inSet,ir->x.val,1);
        if(ir->y.oprType==IROPR_DEFVAR)
          bitset_modify(inSet,ir->y.val,1);
        break;
      case IRTYPE_PLUS:
      case IRTYPE_MINUS:
      case IRTYPE_MUL:
      case IRTYPE_DIV:
        if(bitset_get(inSet,ir->x.val)==1){//x is useful
          ir->deleted=false;
          bitset_modify(inSet,ir->x.val,0);
          if(ir->y.oprType==IROPR_DEFVAR)
            bitset_modify(inSet,ir->y.val,1);
          if(ir->z.oprType==IROPR_DEFVAR)
            bitset_modify(inSet,ir->z.val,1);
        }
        else{
          ir->deleted=true;
        }
        break;
      default:
        break;
    }
    
  }
}

void useful_variables_optimization(struct Cfg cfg){
  backward_analysis(cfg,useful_variables_init,useful_variables_init,useful_variables_update_out,useful_variables_update_in_from_out);
}

struct ExpListNode{
  struct IROpr x,y;
  enum IRType expType;
};
struct VarConstInfo{
  enum{
    VAR_UNDEF,VAR_CONST,VAR_NAC
  }type;
  //bool isConst;
  int val;
};

bool same_IROpr(struct IROpr a,struct IROpr b){
  return a.oprType==b.oprType&&a.val==b.val&&a.addrOf==b.addrOf;
}

bool irtype_modify(enum IRType t){
  switch (t) {
    case IRTYPE_ASSIGN:
    case IRTYPE_PLUS:
    case IRTYPE_MINUS:
    case IRTYPE_MUL:
    case IRTYPE_DIV:
    case IRTYPE_VL:
    case IRTYPE_CALL:
    case IRTYPE_READ:
      return true;
    default:
      return false;
  }
  return false;
}

void intra_basicblock_optimization(struct Cfg cfg){
  struct VarConstInfo *varConstInfo=calloc(cfg.defVarMaxIndex+3,sizeof(struct VarConstInfo));
  for(struct CfgNode *cfgNode=cfg.entry;cfgNode!=NULL;cfgNode=cfgNode->nextCfgNode){
    for(int i=0;i<=cfg.defVarMaxIndex;++i){
      varConstInfo[i].type=VAR_UNDEF;
    }
    for(struct IRNode *ir=cfgNode->basicBlock.head;ir!=NULL&&ir->inCfgNode==cfgNode;ir=ir->nxt){
      switch (ir->irType) {
        case IRTYPE_PLUS:
        case IRTYPE_MINUS:
        case IRTYPE_MUL:
        case IRTYPE_DIV:
          for(struct IRNode *tmpIR=ir->prv;tmpIR!=NULL&&tmpIR->inCfgNode==cfgNode;tmpIR=tmpIR->prv){
            if(tmpIR->irType==ir->irType&&same_IROpr(tmpIR->y,ir->y)&&same_IROpr(tmpIR->z,ir->z)){
              ir->irType=IRTYPE_ASSIGN;
              ir->argNum=1;
              ir->y=tmpIR->x;
              break;
            }
            else if(irtype_modify(tmpIR->irType)&&(same_IROpr(tmpIR->x,ir->y)||same_IROpr(tmpIR->x,ir->z))){
              break;
            }
          }
          break;
        case IRTYPE_ASSIGN:
        case IRTYPE_LV:
          for(struct IRNode *tmpIR=ir->prv;tmpIR!=NULL&&tmpIR->inCfgNode==cfgNode;tmpIR=tmpIR->prv){
            if(tmpIR->irType==IRTYPE_ASSIGN&&same_IROpr(tmpIR->x,ir->y)){
              ir->y=tmpIR->y;
              break;
            }
            else if(same_IROpr(tmpIR->x,ir->y)){
              break;
            }
          }
        default:
          break;
      }
      switch (ir->irType) {
        case IRTYPE_PLUS:
        case IRTYPE_MINUS:
        case IRTYPE_MUL:
        case IRTYPE_DIV:{
          if(ir->y.oprType==IROPR_DEFVAR&&varConstInfo[ir->y.val].type==VAR_CONST){
            ir->y.oprType=IROPR_NUM;
            ir->y.val=varConstInfo[ir->y.val].val;
          }
          if(ir->z.oprType==IROPR_DEFVAR&&varConstInfo[ir->z.val].type==VAR_CONST){
            ir->z.oprType=IROPR_NUM;
            ir->z.val=varConstInfo[ir->z.val].val;
          }
          if(ir->y.oprType==IROPR_NUM&&ir->z.oprType==IROPR_NUM){
            int rvalue=0;
            bool mathError=false;
            switch(ir->irType){
              case IRTYPE_PLUS:
                rvalue=(ir->y.val)+(ir->z.val);
                break;
              case IRTYPE_MINUS:
                rvalue=(ir->y.val)-(ir->z.val);
                break;
              case IRTYPE_MUL:
                rvalue=(ir->y.val)*(ir->z.val);
                break;
              case IRTYPE_DIV:{
                int a=ir->y.val,b=ir->z.val;
                if(b==0){
                  mathError=true;
                  break;
                }
                bool op=0;
                if(a<0){
                  op=!op;
                  a=-a;
                }
                if(b<0){
                  op=!op;
                  b=-b;
                }
                if(op){
                  rvalue=-((a+b-1)/b);
                }
                else{
                  rvalue=a/b;
                }
                break;
              }
              default:
                assert(0);
            }
            if(!mathError){
              ir->irType=IRTYPE_ASSIGN;
              ir->argNum=1;
              varConstInfo[ir->x.val].type=VAR_CONST;
              varConstInfo[ir->x.val].val=rvalue;
              ir->y.val=rvalue;
            }
          }
          else{
            varConstInfo[ir->x.val].type=VAR_NAC; 
          }
          break;
        }
        case IRTYPE_ASSIGN:
          if(ir->y.oprType==IROPR_NUM){
            varConstInfo[ir->x.val].type=VAR_CONST;
            varConstInfo[ir->x.val].val=ir->y.val;
          }
          else if(ir->y.oprType==IROPR_DEFVAR&&varConstInfo[ir->y.val].type==VAR_CONST){
            ir->y.oprType=IROPR_NUM;
            ir->y.val=varConstInfo[ir->y.val].val;
            varConstInfo[ir->x.val]=(struct VarConstInfo){VAR_CONST,ir->y.val};
          }
          else{
            varConstInfo[ir->x.val]=(struct VarConstInfo){VAR_NAC,0};
          }
          break;
        case IRTYPE_VL:
          if(ir->y.oprType==IROPR_DEFVAR&&varConstInfo[ir->y.val].type==VAR_CONST){
            ir->y.oprType=IROPR_NUM;
            ir->y.val=varConstInfo[ir->y.val].val;
          }
          varConstInfo[ir->x.val]=(struct VarConstInfo){VAR_NAC,0};
          break;
        case IRTYPE_CALL:
        case IRTYPE_READ:
        case IRTYPE_PARAM:
          varConstInfo[ir->x.val]=(struct VarConstInfo){VAR_NAC,0};
          break; 
        case IRTYPE_LV:
        case IRTYPE_IFGOTO:
          if(ir->x.oprType==IROPR_DEFVAR&&varConstInfo[ir->x.val].type==VAR_CONST){
            ir->x.oprType=IROPR_NUM;
            ir->x.val=varConstInfo[ir->x.val].val;
          }
          if(ir->y.oprType==IROPR_DEFVAR&&varConstInfo[ir->y.val].type==VAR_CONST){
            ir->y.oprType=IROPR_NUM;
            ir->y.val=varConstInfo[ir->y.val].val;
          }
          break;
        case IRTYPE_RETURN:
        case IRTYPE_ARG:
        case IRTYPE_WRITE:
          if(ir->x.oprType==IROPR_DEFVAR&&varConstInfo[ir->x.val].type==VAR_CONST){
            ir->x.oprType=IROPR_NUM;
            ir->x.val=varConstInfo[ir->x.val].val;
          } 
          break;
        default:
          //do nothing 
          break;
      }
    }
  }
    free(varConstInfo);
}
