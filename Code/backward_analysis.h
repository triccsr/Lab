#ifndef BACKWARD_ANALYSIS_H
#define BACKWARD_ANALYSIS_H 0
#include "cfg.h"
void backward_analysis(struct Cfg cfg,void (*init_in_out)(struct Cfg,struct CfgNode*),void (*init_exit_in)(struct Cfg,struct CfgNode*),bool (*update_out)(struct Cfg,struct CfgNode*), void (*update_in_from_out)(struct Cfg,struct CfgNode*));
#endif