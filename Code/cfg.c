#include "cfg.h"
#include <stdlib.h>
#include "def.h"
#include <stdbool.h>

void add_cfg_edge(struct CfgNode *src,struct CfgNode *dst){
  struct CfgEdge *e=calloc(1,sizeof(struct CfgEdge));
  e->to=dst;
  e->next=src->firstOutEdge;
  src->firstOutEdge=e;
  struct CfgEdge *reverseEdge=calloc(1,sizeof(struct CfgEdge));
  reverseEdge->to=src;
  reverseEdge->next=dst->firstInEdge;
  dst->firstInEdge=reverseEdge;
}

bool is_jump(struct IRNode *irNode){
  return irNode->irType==IRTYPE_GOTO||irNode->irType==IRTYPE_IFGOTO||irNode->irType==IRTYPE_RETURN;
}

struct Cfg function_IR_to_CFG(struct IRListPair funcIR){
  struct Cfg res=(struct Cfg){NULL,NULL,0,0,0};
  res.exit=calloc(1,sizeof(struct CfgNode));
  res.labelNum=0;
  for(struct IRNode *irNode=funcIR.head;irNode!=funcIR.tail->nxt;irNode=irNode->nxt){
    if(irNode->irType==IRTYPE_LABEL){
      if(res.labelNum<irNode->x.val+1){
        res.labelNum=irNode->x.val+1;
      } 
    }
    struct IROpr iroprs[3]={irNode->x,irNode->y,irNode->z};
    for(int i=0;i<irNode->argNum;++i){
      if(iroprs[i].oprType==IROPR_DEFVAR&&iroprs[i].val>res.defVarMaxIndex){
        res.defVarMaxIndex=iroprs[i].val;
      }
      if(iroprs[i].oprType==IROPR_TMPVAR&&iroprs[i].val>res.tmpVarMaxIndex){
        res.tmpVarMaxIndex=iroprs[i].val;
      }
    }
  }

  struct CfgNode *nowCfgNode=NULL;
  struct CfgNode* *labelIsIn=calloc(res.labelNum,sizeof(struct CfgNode*));
  for(struct IRNode *irNode=funcIR.head;irNode!=funcIR.tail->nxt;irNode=irNode->nxt){
    if(nowCfgNode==NULL){// the first ir token of function
      nowCfgNode=calloc(1,sizeof(struct CfgNode));
      nowCfgNode->basicBlock.head=irNode;
      nowCfgNode->index=0;
      res.entry=nowCfgNode;
    }
    irNode->inCfgNode=nowCfgNode;
    if(irNode->irType==IRTYPE_LABEL){
      labelIsIn[irNode->x.val]=nowCfgNode;
    }
    if(irNode==funcIR.tail||(irNode->irType!=IRTYPE_LABEL&&irNode->nxt->irType==IRTYPE_LABEL)||(is_jump(irNode)&&!is_jump(irNode->nxt))){// end of a BB
      nowCfgNode->basicBlock.tail=irNode;
      if(irNode!=funcIR.tail){
        struct CfgNode *nextCfgNode=calloc(1,sizeof(struct CfgNode));
        nextCfgNode->basicBlock.head=irNode->nxt;
        nextCfgNode->index=nowCfgNode->index+1;
        nowCfgNode->nextCfgNode=nextCfgNode;
        if(irNode->irType!=IRTYPE_GOTO&&irNode->irType!=IRTYPE_RETURN){
          add_cfg_edge(nowCfgNode,nextCfgNode);
        }
        nowCfgNode=nextCfgNode;
      }
      else {
        nowCfgNode->nextCfgNode=res.exit;
      }
      if (irNode==funcIR.tail||irNode->irType == IRTYPE_RETURN) {
        add_cfg_edge(nowCfgNode, res.exit);
      }
    }
  }
  for(struct IRNode *irNode=funcIR.head;irNode!=funcIR.tail->nxt;irNode=irNode->nxt){//GOTO label or IFGOTO label
    if(irNode->irType==IRTYPE_GOTO){// GOTO x
      add_cfg_edge(irNode->inCfgNode,labelIsIn[irNode->x.val]);
    }
    else if(irNode->irType==IRTYPE_IFGOTO){// IF x [relop] y GOTO z
      add_cfg_edge(irNode->inCfgNode,labelIsIn[irNode->z.val]);
    }
  }
  return res;
}