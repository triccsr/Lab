#ifndef LAB4_H
#define LAB4_H 0
#include <stdbool.h>
#include <stdio.h>
#include <bits/types/FILE.h>
struct Place{
  int regIndex;
  int frameOffset;// -1 means no place in frame, place=%fp-frameIndex
  //int addr;//if it is in heap
  bool inReg;
};

struct RegInfo{
  enum{REGCONTENT_VAR,REGCONTENT_ADDR,REGCONTENT_I}content;  
  bool isEmpty;
  int val;
};
void fprint_program_asm(struct IRListPair programIR,FILE* f);


#endif