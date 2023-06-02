#ifndef STATIC_ANALYSIS_H
#define STATIC_ANALYSIS_H 0
#include "cfg.h"
void backward_analysis(struct Cfg cfg,void (*init_in_out)(struct Cfg,struct CfgNode*),void (*init_exit_in)(struct Cfg,struct CfgNode*),bool (*update_out)(struct Cfg,struct CfgNode*), void (*update_in_from_out)(struct Cfg,struct CfgNode*));

void forward_analysis(struct Cfg cfg,void (*init_in_out)(struct Cfg cfg,struct CfgNode*),void (*init_entry_out)(struct Cfg cfg,struct CfgNode*),bool (*update_in)(struct Cfg cfg,struct CfgNode*),void (*update_out_from_in)(struct Cfg cfg,struct CfgNode*));
#endif