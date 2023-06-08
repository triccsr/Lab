#include <assert.h>
#include <bits/types/FILE.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include "def.h"
#include "lab4.h"
#include "parse_tree.h"
#include "cfg.h"

#define REG_COUNT 4 

#define writefile(x) fprintf(f,x);
int regTime[REG_COUNT],currentRegTime;

int get_treg_index(int r){
  if(r<=7)return r+8;
  return r+16;
}

int get_reg(struct IROpr x,bool needOldValue,struct RegInfo *reg,struct Place *varPlace,int *addrPlace,int *offset,FILE* f){
  ++currentRegTime;
  // assert(x.oprType==IROPR_DEFVAR);
  if(x.oprType==IROPR_DEFVAR){
    if(x.addrOf==false&&varPlace[x.val].inReg==true){// already in reg
      regTime[varPlace[x.val].regIndex]=currentRegTime;
      return varPlace[x.val].regIndex;  
    }
    if(x.addrOf==true&&addrPlace[x.val]!=-1){
      regTime[addrPlace[x.val]]=currentRegTime;
      return addrPlace[x.val];
    }
  }
  int regIndex=-1;
  for(int i=0;i<REG_COUNT;++i){
    if(reg[i].isEmpty){
      regIndex=i;
      break;
    }
  }
  if(regIndex==-1){
    for(int i=1;i<REG_COUNT;++i){
      if(regTime[i]<regTime[regIndex]){
        regIndex=i;
      }
    }
    if(reg[regIndex].content==REGCONTENT_VAR){//move out reg, save into memory
      varPlace[reg[regIndex].val].inReg=false;
      varPlace[reg[regIndex].val].regIndex=-1;
      if(varPlace[reg[regIndex].val].frameOffset==-1){
        *offset+=4;
        varPlace[reg[regIndex].val].frameOffset=*offset;
      }
      fprintf(stderr,"save var %d from reg %d to mem %d($fp)\n",reg[regIndex].val,regIndex,-varPlace[reg[regIndex].val].frameOffset);
      fprintf(f,"sw $t%d, %d($fp)\n",regIndex,-varPlace[reg[regIndex].val].frameOffset);
    }
    else if(reg[regIndex].content==REGCONTENT_ADDR){//move out reg
      addrPlace[reg[regIndex].val]=-1;
      fprintf(stderr,"drop addr of var %d from reg %d\n",reg[regIndex].val,regIndex);
    }
  }

  regTime[regIndex]=currentRegTime;
  reg[regIndex].isEmpty=false;
  if(x.oprType==IROPR_NUM){
    reg[regIndex].content=REGCONTENT_I;
    fprintf(f,"li $t%d, %d\n",regIndex,x.val);
    fprintf(stderr,"reg %d <- %d\n",regIndex,x.val);
  }
  else if(x.oprType==IROPR_DEFVAR&&x.addrOf==false){
    reg[regIndex].val=x.val;
    reg[regIndex].content=REGCONTENT_VAR;
    varPlace[x.val].regIndex=regIndex;
    varPlace[x.val].inReg=true;
    fprintf(stderr,"var %d 's value is in reg %d\n",x.val,regIndex);
    //fprintf(f,"lw $t%d, %d($fp)\n",regIndex,varPlace[x.val].frameOffset);
    if(needOldValue){//move new value into reg
      assert(varPlace[x.val].frameOffset!=-1);
      fprintf(f,"lw $t%d, %d($fp)\n",regIndex,-varPlace[x.val].frameOffset);
    }
  }
  else if(x.oprType==IROPR_DEFVAR&&x.addrOf==true){
    reg[regIndex].content=REGCONTENT_ADDR;
    reg[regIndex].val=x.val;
    addrPlace[x.val]=regIndex;
    fprintf(f,"la $t%d, gv%d\n",regIndex,x.val);
  }
  return regIndex;
}
void push_arg(struct IROpr x,struct RegInfo *reg,struct Place *varPlace,int *addrPlace,int *offset, FILE* f){
  if(x.oprType==IROPR_NUM){//instant number
    fprintf(f,"li $v1, %d\n",x.val);
    *offset+=4;
    fprintf(f,"sw $v1 %d($fp)\n",-*offset);
  }
  else if(x.oprType==IROPR_DEFVAR&&x.addrOf==true){//&x
    if(addrPlace[x.val]==-1){
      fprintf(f,"la $v1, gv%d\n",x.val);
      *offset+=4;
      fprintf(f,"sw $v1 %d($fp)\n",-*offset);
    }
    else{
      *offset+=4;
      fprintf(f,"sw $t%d, %d($fp)\n",addrPlace[x.val],-*offset);
    }
  }
  else if(x.oprType==IROPR_DEFVAR&&varPlace[x.val].inReg){
    *offset+=4;
    fprintf(f,"sw $t%d, %d($fp)\n",varPlace[x.val].regIndex,-*offset);
  }
  else if(x.oprType==IROPR_DEFVAR&&varPlace[x.val].frameOffset!=-1){// tmp var not in reg
    fprintf(f,"lw $v1, %d($fp)\n",-varPlace[x.val].frameOffset);
    *offset+=4;
    fprintf(f,"sw $v1, %d($fp)\n",-*offset);
  }
  else{
    assert(0);
  }
}
void save_used_regs_to_memory(struct RegInfo *reg,struct Place *varPlace,int *addrPlace,int *frameSize, FILE *f){
  for(int i=0;i<REG_COUNT;++i){
    if(reg[i].isEmpty)continue;
    if(reg[i].content==REGCONTENT_VAR){
      int varIndex=reg[i].val;
      if(varPlace[varIndex].frameOffset==-1){
        *frameSize+=4;
        varPlace[varIndex].frameOffset=*frameSize;
      } 
      varPlace[varIndex].inReg=false;
      varPlace[varIndex].regIndex=-1;
      fprintf(f,"sw $t%d, %d($fp)\n",i,-varPlace[varIndex].frameOffset);
      fprintf(stderr,"save var %d in reg %d to %d($fp)\n",varIndex,i,-varPlace[varIndex].frameOffset);
    }
    if(reg[i].content==REGCONTENT_ADDR){
      addrPlace[reg[i].val]=-1;
    }
    reg[i].isEmpty=true;
  }
}

void free_reg(int index,struct RegInfo *reg,struct Place *varPlace, int* addrPlace){
  if(reg[index].content==REGCONTENT_VAR){
    int varIndex=reg[index].val;
    varPlace[varIndex].regIndex=-1;
    varPlace[varIndex].inReg=false;
  }
  else if(reg[index].content==REGCONTENT_ADDR){
    int varIndex=reg[index].val;
    addrPlace[varIndex]=-1;
  }
   reg[index].isEmpty=true;
}
void free_reg_if_i(int index,struct RegInfo *reg){
  if(reg[index].content==REGCONTENT_I){
    reg[index].isEmpty=true;
  }
}


void fprint_basic_block_asm(struct IRListPair bbIR,struct Place *varPlace,int* addrPlace,int maxVarIndex,int *frameSize,FILE* f){

  struct RegInfo reg[REG_COUNT];// reg[i] is the index of var in reg i, -1 means empty
  for(int i=0;i<REG_COUNT;++i){// when enter a new BB, all regs are empty
    reg[i].isEmpty=true;
  }
  //bool funcNeedFinish=false;
  
  bool regSaved=false;
  for(struct IRNode *ir=bbIR.head;ir!=bbIR.tail->nxt;ir=ir->nxt){
    /*if(ir->irType==IRTYPE_FUNC){//new function
      //finish previous function
      if(funcNeedFinish){
        fprintf(f,"lw $ra, -4($fp)\n");
        fprintf(f,"addu $sp, $fp, 0\n");
        fprintf(f,"lw $fp, -8($fp)\n");
        fprintf(f,"jal $ra\n");
      } 
      //init
      for(int i=0;i<REG_COUNT;++i){
        reg[i].isEmpty=true;
      }
      for(int i=0;i<=maxVarIndex;++i){
        varPlace[i].regIndex=-1;
        varPlace[i].frameOffset=-1;
        varPlace[i].inReg=false;
      }
      frameSize=0;
      
      if(ir->x.val==0){//main
        fprintf(f,"main:\n");
      }
      else{//not main, need start
        fprintf(f,"func%d:\n",ir->x.val);
        frameSize=8;
        fprintf(f,"subu $sp, $sp, 8\n");
        fprintf(f,"sw $ra, 4($sp)\n");
        fprintf(f,"sw $fp, 0($sp)\n");
        fprintf(f,"addi $fp, $sp, 8\n");
        funcNeedFinish=true;
      }
    }
    */
    {//in a function
      switch (ir->irType) {
        case IRTYPE_ARG:
        break;

        case IRTYPE_ASSIGN:{
          int rx=get_reg(ir->x,false,reg,varPlace,addrPlace,frameSize,f);
          if(ir->y.oprType==IROPR_NUM){
            fprintf(f,"li $t%d, %d\n",rx,ir->y.val);
          }
          else if(ir->y.oprType==IROPR_DEFVAR&&ir->y.addrOf==false){
            int ry=get_reg(ir->y,true,reg,varPlace,addrPlace,frameSize,f);
            fprintf(f,"move $t%d, $t%d\n",rx,ry);
          }
          else if(ir->y.oprType==IROPR_DEFVAR&&ir->y.addrOf==true){
            fprintf(f,"la $t%d, gv%d\n",rx,ir->y.val);
          }
        }
        break;

        case IRTYPE_CALL:{
          //int oldFrameSize=*frameSize;
          /*int saveOffset[10];
          for(int i=0;i<REG_COUNT;++i){
            if(!reg[i].isEmpty){
              *frameSize+=4;
              fprintf(f,"sw $t%d, %d($fp)\n",i,*frameSize);
              saveOffset[i]=*frameSize;
              reg[i].isEmpty=true;
            }
            else{
              saveOffset[i]=-1;
            }
          }*/
          int oldRegTime[REG_COUNT];
          for(int i=0;i<REG_COUNT;++i)oldRegTime[i]=regTime[i];
          save_used_regs_to_memory(reg,varPlace,addrPlace,frameSize,f);
          int argCount=0;
          struct IRNode *lastArgIR=NULL;
          for(struct IRNode *argIR=ir->prv;argIR->irType==IRTYPE_ARG;argIR=argIR->prv){
            ++argCount;
            lastArgIR=argIR;
          }
          for(struct IRNode *argIR=lastArgIR;argIR->irType==IRTYPE_ARG;argIR=argIR->nxt){
            push_arg(argIR->x,reg,varPlace,addrPlace,frameSize,f);
          }
          fprintf(f,"subu $sp, $fp, %d\n",*frameSize);
          if(ir->y.val==0){
            fprintf(f,"jal main\n");
          }
          else{
            fprintf(f,"jal func%d\n",ir->y.val);
          }
          fprintf(f,"addi $sp, $sp, %d\n",argCount*4);
          *frameSize-=argCount*4;
          for(int i=0;i<REG_COUNT;++i)regTime[i]=oldRegTime[i];
          /*for(int i=0;i<REG_COUNT;++i){
            if(saveOffset[i]!=-1){
              fprintf(f,"lw $t%d, %d($fp)\n",i,saveOffset[i]);
              reg[i].isEmpty=false;
            }
          } 
          *frameSize=oldFrameSize;*/
          int rx=get_reg(ir->x,false,reg,varPlace,addrPlace,frameSize,f);
          fprintf(f,"move $t%d, $v0\n",rx);
        }
        break;

        case IRTYPE_DEC:
        break;

        case IRTYPE_FUNC:{
          if(ir->x.val==0){
            fprintf(f,"main:\n");
          }
          else{
            fprintf(f,"func%d:\n",ir->x.val);
          }
          for(int i=0;i<REG_COUNT;++i){
            reg[i].isEmpty=true;
          }
          for(int i=0;i<=maxVarIndex;++i){
            varPlace[i].regIndex=-1;
            varPlace[i].frameOffset=-1;
            varPlace[i].inReg=false;
          }
          int paramOffset=0;
          for(struct IRNode *paramIR=ir->nxt;paramIR->irType==IRTYPE_PARAM;paramIR=paramIR->nxt){
            varPlace[paramIR->x.val].inReg=false;
            varPlace[paramIR->x.val].frameOffset=-paramOffset;
            paramOffset+=4;
          }
          fprintf(f,"subu $sp, $sp, 8\n");
          fprintf(f,"sw $ra, 4($sp)\n");
          fprintf(f,"sw $fp, 0($sp)\n");
          fprintf(f,"addi $fp, $sp, 8\n");
          *frameSize=8;
        }
        break;

        case IRTYPE_GOTO:{
          if(!regSaved){
            regSaved=true;
            save_used_regs_to_memory(reg,varPlace,addrPlace,frameSize,f);
          }
          fprintf(f,"j label%d\n",ir->x.val);
        }
        break;

        case IRTYPE_IFGOTO:{
          if(!regSaved){
            regSaved=true;
            save_used_regs_to_memory(reg,varPlace,addrPlace,frameSize,f);
          }
          int rx=get_reg(ir->x,true,reg,varPlace,addrPlace,frameSize,f);
          int ry=get_reg(ir->y,true,reg,varPlace,addrPlace,frameSize,f);
          switch (ir->relop) {
            case RELOP_EQ:
              fprintf(f,"beq $t%d, $t%d, label%d\n",rx,ry,ir->z.val);
              break;
            case RELOP_NEQ:
              fprintf(f,"bne $t%d, $t%d, label%d\n",rx,ry,ir->z.val);
              break;
            case RELOP_GR:
              fprintf(f,"bgt $t%d, $t%d, label%d\n",rx,ry,ir->z.val);
              break;
            case RELOP_LE:
              fprintf(f,"blt $t%d, $t%d, label%d\n",rx,ry,ir->z.val);
              break;
            case RELOP_GEQ:
              fprintf(f,"bge $t%d, $t%d, label%d\n",rx,ry,ir->z.val);
              break;
            case RELOP_LEQ:
              fprintf(f,"ble $t%d, $t%d, label%d\n",rx,ry,ir->z.val);
              break;
          }
          free_reg(rx,reg,varPlace,addrPlace);
          free_reg(ry,reg,varPlace,addrPlace);
        }
        break;

        case IRTYPE_LABEL:
          fprintf(f,"label%d:\n",ir->x.val);
          break;
          
        case IRTYPE_LV:{
          int rx=get_reg(ir->x,true,reg,varPlace,addrPlace,frameSize,f);
          int ry=get_reg(ir->y,true,reg,varPlace,addrPlace,frameSize,f);
          fprintf(f,"sw $t%d, 0($t%d)\n",ry,rx);
          free_reg_if_i(rx,reg);
          free_reg_if_i(ry,reg);
        }
        break;
        
        case IRTYPE_PARAM:
          break;

        case IRTYPE_RETURN:{
          int rx=get_reg(ir->x,true,reg,varPlace,addrPlace,frameSize,f);
          fprintf(f,"move $v0, $t%d\n",rx);
          fprintf(f,"lw $ra, -4($fp)\n");
          fprintf(f,"lw $fp, -8($fp)\n");
          fprintf(f,"addi $sp, $sp, %d\n",*frameSize);
          fprintf(f,"jr $ra\n");
        }
        break;
        
        case IRTYPE_VL:{
          int rx=get_reg(ir->x,false,reg,varPlace,addrPlace,frameSize,f);
          int ry=get_reg(ir->y,true,reg,varPlace,addrPlace,frameSize,f);
          fprintf(f,"lw $t%d, 0($t%d)\n",rx,ry);
        }
        break;
          
        case IRTYPE_READ:
          fprintf(f,"subu $sp, $fp, %d\n",*frameSize+4);
          fprintf(f,"sw $ra, 0($sp)\n");
          fprintf(f,"jal read\n");
          fprintf(f,"lw $ra, 0($sp)\n");
          fprintf(f,"addi $sp, $sp, 4\n");
          int rx=get_reg(ir->x,false,reg,varPlace,addrPlace,frameSize,f);
          fprintf(f,"move $t%d, $v0\n",rx);
          break;
        
        case IRTYPE_WRITE:
          if(varPlace[ir->x.val].inReg){
            fprintf(f,"move $a0, $t%d\n",varPlace[ir->x.val].regIndex);
          }
          else{
            fprintf(f,"lw $a0, %d($fp)\n",-varPlace[ir->x.val].frameOffset);
          }
          fprintf(f,"subu $sp, $fp, %d\n",*frameSize+4);
          fprintf(f,"sw $ra, 0($sp)\n");
          fprintf(f,"jal write\n");
          fprintf(f,"lw $ra, 0($sp)\n");
          fprintf(f,"addi $sp, $sp, 4\n");
          break;

        case IRTYPE_PLUS:{
          if(ir->z.oprType==IROPR_NUM){
            int rx=get_reg(ir->x,false,reg,varPlace,addrPlace,frameSize,f);
            int ry=get_reg(ir->y,true,reg,varPlace,addrPlace,frameSize,f);
            fprintf(f,"addi $t%d, $t%d, %d\n",rx,ry,ir->z.val);
          }
          else{
            int rx=get_reg(ir->x,false,reg,varPlace,addrPlace,frameSize,f);
            int ry=get_reg(ir->y,true,reg,varPlace,addrPlace,frameSize,f);
            int rz=get_reg(ir->z,true,reg,varPlace,addrPlace,frameSize,f);
            fprintf(f,"add $t%d, $t%d, $t%d\n",rx,ry,rz); 
          }
        }
        break; 

        case IRTYPE_MINUS:{
          if(ir->z.oprType==IROPR_NUM){
            int rx=get_reg(ir->x,false,reg,varPlace,addrPlace,frameSize,f);
            int ry=get_reg(ir->y,true,reg,varPlace,addrPlace,frameSize,f);
            fprintf(f,"addi $t%d, $t%d, %d\n",rx,ry,-ir->z.val);
          }
          else{
            int rx=get_reg(ir->x,false,reg,varPlace,addrPlace,frameSize,f);
            int ry=get_reg(ir->y,true,reg,varPlace,addrPlace,frameSize,f);
            int rz=get_reg(ir->z,true,reg,varPlace,addrPlace,frameSize,f);
            fprintf(f,"sub $t%d, $t%d, $t%d\n",rx,ry,rz); 
          }
        }
        break; 
        
        case IRTYPE_MUL:{
          int rx=get_reg(ir->x,false,reg,varPlace,addrPlace,frameSize,f);
          int ry=get_reg(ir->y,true,reg,varPlace,addrPlace,frameSize,f);
          int rz=get_reg(ir->z,true,reg,varPlace,addrPlace,frameSize,f);
          fprintf(f,"mul $t%d, $t%d, $t%d\n",rx,ry,rz); 
        }
        break;

        case IRTYPE_DIV:{
          int rx=get_reg(ir->x,false,reg,varPlace,addrPlace,frameSize,f);
          int ry=get_reg(ir->y,true,reg,varPlace,addrPlace,frameSize,f);
          int rz=get_reg(ir->z,true,reg,varPlace,addrPlace,frameSize,f);
          fprintf(f,"div $t%d, $t%d, $t%d\n",rx,ry,rz); 
          fprintf(f,"mflo $t%d\n",rx);
        }
        break;
      }
    }
  }
  
  if(!regSaved){
    regSaved=true;
    save_used_regs_to_memory(reg,varPlace,addrPlace,frameSize,f);
  }
}

void fprint_function_asm(struct IRListPair funcIR,FILE* f){
  int maxVarIndex=0;
  for(struct IRNode *ir=funcIR.head;ir!=funcIR.tail->nxt;ir=ir->nxt){
    struct IROpr iroprs[3]={ir->x,ir->y,ir->z};
    for(int i=0;i<ir->argNum;++i){
      if(iroprs[i].oprType==IROPR_DEFVAR&&iroprs[i].val>maxVarIndex){
        maxVarIndex=iroprs[i].val;
      }
    }
  }
  struct Place *varPlace=calloc(maxVarIndex+3,sizeof(struct Place));
  int *addrPlace=calloc(maxVarIndex+3,sizeof(int));
  int frameSize=0;
  struct Cfg cfg=function_IR_to_CFG(funcIR);
  
  for(int i=0;i<=maxVarIndex;++i){
    varPlace[i].regIndex=-1;
    varPlace[i].inReg=false;
    varPlace[i].frameOffset=-1;
    addrPlace[i]=-1;
  }

  for(struct CfgNode *cfgNode=cfg.entry;cfgNode!=cfg.exit;cfgNode=cfgNode->nextCfgNode){
    if(cfgNode->basicBlock.head!=NULL&&cfgNode->basicBlock.tail!=NULL){
      fprint_basic_block_asm((struct IRListPair){cfgNode->basicBlock.head,cfgNode->basicBlock.tail},varPlace,addrPlace,maxVarIndex,&frameSize,f);
    }
  } 
}

void fprint_program_asm(struct IRListPair programIR, FILE *f){
  fprintf(f,".data\n");
  fprintf(f,"_prompt: .asciiz \"Enter an integer:\"\n");
  fprintf(f,"_ret: .asciiz \"\\n\"\n");
  for(struct IRNode* ir=programIR.head;ir!=NULL;ir=ir->nxt){
    if(ir->irType==IRTYPE_DEC){
      fprintf(f,"gv%d: .space %d\n",ir->x.val,ir->y.val);
    }
  }
  fprintf(f,".globl main\n");
  fprintf(f,".text\n");
  fprintf(f,"read:\n");
  writefile("li $v0, 4\n");
  writefile("la $a0, _prompt\n");
  writefile("syscall\n");
  writefile("li $v0, 5\n");
  writefile("syscall\n");
  writefile("jr $ra\n");
  writefile("write:\n");
  writefile("li $v0, 1\n");
  writefile("syscall\n");
  writefile("li $v0, 4\n");
  writefile("la $a0, _ret\n");
  writefile("syscall\n");
  writefile("move $v0, $0\n");
  writefile("jr $ra\n");
  for(struct IRNode *ir=programIR.head;ir!=NULL;){
    if(ir->irType==IRTYPE_FUNC){
      struct IRNode *funcEndIR=ir;
      for(;funcEndIR->nxt!=NULL&&funcEndIR->nxt->irType!=IRTYPE_FUNC;funcEndIR=funcEndIR->nxt);
      fprint_function_asm((struct IRListPair){ir,funcEndIR},f);
      ir=funcEndIR->nxt;
    }
    else{
      ir=ir->nxt;
    }
  }
}