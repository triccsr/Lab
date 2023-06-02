#ifndef CFG_H
#define CFG_H 0
#include "def.h"
#include <stdint.h>

struct CfgNode{
  struct IRListPair basicBlock;
  struct CfgEdge* firstOutEdge;
  struct CfgEdge* firstInEdge;
  struct{
    uint64_t *usefulVarBitSet;
    uint64_t *aliveVarBitSet;

  }inInfoPtr,outInfoPtr;
  uint64_t *dominatedBitSet;
  struct CfgNode *prevCfgNode;
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
  int size;
};

struct CfgNode* new_CfgNode(struct Cfg* cfg);
void add_cfg_edge(struct CfgNode* src,struct CfgNode* dst);
struct Cfg function_IR_to_CFG(struct IRListPair funcIR);
#endif