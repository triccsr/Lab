#ifndef CFG_H
#define CFG_H 0
#include "def.h"
#include <stdint.h>

struct CfgNode{
  struct IRListPair basicBlock;
  struct CfgEdge* firstOutEdge;
  struct CfgEdge* firstInEdge;
  union{
    uint64_t *usefulVarBitSet;
    uint64_t *aliveVarBitSet;
    uint64_t *dominateBitSet;

  }inInfoPtr,outInfoPtr;
  struct CfgNode *nextCfgNode;
  int index; 
};

struct CfgEdge{
  struct CfgNode *to;
  struct CfgEdge *next;
  bool isDeleted;
};

struct Cfg{
  struct CfgNode *entry;
  struct CfgNode *exit;
  int labelNum;
  int defVarMaxIndex;
  int tmpVarMaxIndex;
};

void add_cfg_edge(struct CfgNode* src,struct CfgNode* dst);
struct Cfg function_IR_to_CFG(struct IRListPair funcIR);
#endif