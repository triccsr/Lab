#include "cfg.h"
#include "def.h"
#include <stdbool.h>

void backward_analysis(struct Cfg cfg,void (*init_in_out)(struct Cfg cfg,struct CfgNode*),void (*init_exit_in)(struct Cfg cfg,struct CfgNode*),bool (*update_out)(struct Cfg cfg,struct CfgNode*), void (*update_in_from_out)(struct Cfg cfg,struct CfgNode*)){
  for(struct CfgNode *cfgNode=cfg.entry->nextCfgNode;cfgNode!=cfg.exit;cfgNode=cfgNode->nextCfgNode){
    if(cfgNode!=cfg.exit){
      init_in_out(cfg,cfgNode);
      update_in_from_out(cfg,cfgNode); 
    }
  }
  init_exit_in(cfg,cfg.exit);
  while(1){
    bool finished=true;
    for(struct CfgNode *cfgNode=cfg.entry->nextCfgNode;cfgNode!=cfg.exit;cfgNode=cfgNode->nextCfgNode){
      bool modified=update_out(cfg,cfgNode);
      if(modified){
        finished=false;
      }
      update_in_from_out(cfg,cfgNode);
    } 
    if(finished)break;
  } 
}

void forward_analysis(struct Cfg cfg,void (*init_in_out)(struct Cfg cfg,struct CfgNode*),void (*init_entry_out)(struct Cfg cfg,struct CfgNode*),bool (*update_in)(struct Cfg cfg,struct CfgNode*),void (*update_out_from_in)(struct Cfg cfg,struct CfgNode*)){
  for(struct CfgNode *cfgNode=cfg.entry->nextCfgNode;cfgNode!=cfg.exit->nextCfgNode;cfgNode=cfgNode->nextCfgNode){
    if(cfgNode!=cfg.entry){
      init_in_out(cfg,cfgNode);
      update_out_from_in(cfg,cfgNode);
    }
  } 
  init_entry_out(cfg,cfg.entry);
  while(1){
    bool finished=true;
    for(struct CfgNode *cfgNode=cfg.entry->nextCfgNode;cfgNode!=cfg.exit->nextCfgNode;cfgNode=cfgNode->nextCfgNode){
      bool modified=update_in(cfg,cfgNode);
      if(modified){
        finished=false;
      } 
      update_out_from_in(cfg,cfgNode);
    }
    if(finished)break;
  }
}