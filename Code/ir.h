#ifndef IR_H
#define IR_H
#include "def.h"
struct IRListPair new_single_IRList(enum IRType irType,int argc,...);
struct IRListPair cat_IRList(int listNum,...);
struct IRListPair Program2IR(struct TreeNode *program);
struct IRListPair get_lab3_ir();
void print_IR(struct IRListPair irList);
#endif