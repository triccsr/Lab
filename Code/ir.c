#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include "ID_trie.h"
#include "def.h"
#include "parse_tree.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#define min(x,y) (((x)<(y))?(x):(y))

extern struct TrieNode *varRoot,*funcRoot, *structNameRoot;
struct TrieNode *defVarRoot;
bool str_equal(const char s1[],const char s2[]);
bool cannotTranslate;
int defVarCount=1,tmpVarCount=1,labelCount=1,funcCount=1;

struct TypeNode* specifier2type(struct TreeNode *now);
void get_size_of(struct TypeNode *type);

void can_not_translate(){
  printf("Cannot translate: Code contains variables or parameters of structure type.");
  cannotTranslate=true;
}

struct IROpr new_tmp_var(){
  struct IROpr ret;
  ret.oprType=IROPR_TMPVAR;
  ret.val=tmpVarCount++;
  ret.addrOf=false;
  return ret;
}
struct IROpr new_def_var(){
  struct IROpr ret;
  ret.oprType=IROPR_DEFVAR;
  ret.val=defVarCount++;
  ret.addrOf=false;
  return ret;
}
struct IROpr new_num(int v){
  return (struct IROpr){IROPR_NUM,v,false};
}
struct IROpr new_label(){
  return (struct IROpr){IROPR_LABEL,labelCount++,false};
}
struct IROpr new_func(){
  return (struct IROpr){IROPR_FUNC,funcCount++,false};
}
struct IROpr get_addr_of_var(struct IROpr varIROpr){
  assert(varIROpr.oprType==IROPR_DEFVAR||varIROpr.oprType==IROPR_TMPVAR);
  varIROpr.addrOf=true;
  return varIROpr;
}
struct IROpr remove_addr(struct IROpr addrIROpr){
  assert(addrIROpr.addrOf==true);
  addrIROpr.addrOf=false;
  return addrIROpr;
}

struct IRListPair append_IRNode(struct IRListPair irList,struct IRNode* irNode){
  if(irList.head==NULL&&irList.tail==NULL){
    irList.head=irList.tail=irNode;
  }
  else{
    irList.tail->nxt=irNode;
    irNode->prv=irList.tail;
  }
  return (struct IRListPair){irList.head,irNode};
}

struct IRListPair new_single_IRList(enum IRType irType,int argc,...){
  struct IRNode* newIRNode=calloc(1,sizeof(struct IRNode));
  newIRNode->prv=newIRNode->nxt=NULL;
  newIRNode->irType=irType;
  va_list irOprs;
  va_start(irOprs, argc);
  switch (irType) {
    case IRTYPE_LABEL:
    case IRTYPE_FUNC:
    case IRTYPE_GOTO:
    case IRTYPE_RETURN:
    case IRTYPE_ARG:
    case IRTYPE_PARAM:
    case IRTYPE_READ:
    case IRTYPE_WRITE:
      assert(argc==1);
      newIRNode->argNum=1;
      newIRNode->x=va_arg(irOprs, struct IROpr);
      break;
    case IRTYPE_ASSIGN:
    //case IRTYPE_VA:
    case IRTYPE_VL:
    case IRTYPE_LV:
    case IRTYPE_DEC:
    case IRTYPE_CALL:
      assert(argc==2);
      newIRNode->argNum=2;
      newIRNode->x=va_arg(irOprs, struct IROpr);
      newIRNode->y=va_arg(irOprs,struct IROpr);
      break;
    case IRTYPE_PLUS:
    case IRTYPE_MINUS:
    case IRTYPE_MUL:
    case IRTYPE_DIV:
      assert(argc==3);
      newIRNode->argNum=3;
      newIRNode->x=va_arg(irOprs, struct IROpr);
      newIRNode->y=va_arg(irOprs, struct IROpr);
      newIRNode->z=va_arg(irOprs, struct IROpr);
      break;
    case IRTYPE_IFGOTO:
      assert(argc==4);
      newIRNode->argNum=3;
      newIRNode->x=va_arg(irOprs, struct IROpr);
      newIRNode->relop=va_arg(irOprs,enum RelopEnum);
      newIRNode->y=va_arg(irOprs, struct IROpr);
      newIRNode->z=va_arg(irOprs, struct IROpr);
      break;
    default:
      assert(0);
  }
  va_end(irOprs);
  struct IRListPair irList;
  irList.head=irList.tail=newIRNode;
  return irList;
}

struct IRListPair cat_IRList(int listNum,...){
  va_list irLists;
  va_start(irLists,listNum);
  struct IRListPair ret=va_arg(irLists,struct IRListPair);
  for(int i=1;i<listNum;++i){
    struct IRListPair nList=va_arg(irLists, struct IRListPair);
    if(nList.head==NULL && nList.tail==NULL)continue;
    if(nList.head==NULL || nList.tail==NULL){
      assert(0);
    }
    if(ret.head==NULL&&ret.tail==NULL){
      ret.head=nList.head;
      ret.tail=nList.tail;
    }
    else if(ret.tail!=NULL){
      ret.tail->nxt=nList.head;
      nList.head->prv=ret.tail;
    }
    ret.tail=nList.tail;
  }
  va_end(irLists);
  return ret;
}

struct IRListPair Exp2IR(struct TreeNode* exp,struct IROpr place,bool hasPlace);


struct LookupType lookup(struct TreeNode *exp){
  struct LookupType ret;
  if(childrens_are(exp,"ID")){//Exp->ID
    union OutPtr tmpOutPtr=find_name(varRoot,get_kth_child(exp,1)->detail)->outPtr;
    ret.type=tmpOutPtr.varDefHead->type;
    ret.irPos=tmpOutPtr.varDefHead->irPos;
    ret.irList=(struct IRListPair){NULL,NULL};
    return ret;
  }
  else if(childrens_are(exp,"Exp LB Exp RB")){ //array
    ret.irList=(struct IRListPair){NULL,NULL};

    struct IROpr indexIROpr=new_tmp_var();//index
    struct IRListPair indexIRList=Exp2IR(get_kth_child(exp,3),indexIROpr,true);//get index exp,store in indexIROpr
    ret.irList=cat_IRList(2,ret.irList,indexIRList);

    struct LookupType lookupArray=lookup(get_kth_child(exp,1));
    assert(lookupArray.type->kind==ARRAY);
    ret.type=lookupArray.type->u.array.elem;
    ret.irList=cat_IRList(2,ret.irList,lookupArray.irList);

    ret.irPos.type=IRPOS_ADDR;
    ret.irPos.irOpr=new_tmp_var();
    struct IROpr offsetIROpr=new_tmp_var();
    get_size_of(ret.type);
    ret.irList=cat_IRList(2,ret.irList,new_single_IRList(IRTYPE_MUL,3,offsetIROpr,indexIROpr,new_num(ret.type->sizeOf))); //offsetIROpr=indexIROpr*ret.type->sizeOf
    if(lookupArray.irPos.type==IRPOS_VAR){
      assert(0);
      struct IROpr arrayAddrIROpr=new_tmp_var();
      ret.irList=cat_IRList(2,ret.irList,new_single_IRList(IRTYPE_ASSIGN,2,arrayAddrIROpr,get_addr_of_var(lookupArray.irPos.irOpr)));//arrayAddrIROpr=&lookupArray.irPos.irOpr
      ret.irList=cat_IRList(2,ret.irList,new_single_IRList(IRTYPE_PLUS,3,ret.irPos.irOpr,arrayAddrIROpr,offsetIROpr));//ret.irPos.irOpr=arrayAddrIROpr+offsetIROpr
    } 
    else{
      ret.irList=cat_IRList(2,ret.irList,new_single_IRList(IRTYPE_PLUS,3,ret.irPos.irOpr,lookupArray.irPos.irOpr,offsetIROpr));
    }
    return ret;
  }
  else if(childrens_are(exp,"Exp DOT ID")){//struct 
    can_not_translate();
  }
  assert(0);
}

struct ArgsHelperType get_args(struct TreeNode *args){
  struct IROpr t1=new_tmp_var();
  struct IRListPair code1=Exp2IR(get_kth_child(args,1),t1,true);
  struct ArgsHelperType ret;
  ret.irList=code1;
  struct IROprListNode* newArgNode=calloc(1,sizeof(struct IROprListNode));
  newArgNode->irOpr=t1;
  newArgNode->next=NULL;
  if(childrens_are(args,"Exp")){
    ret.argListTail=ret.argListHead=newArgNode;
    return ret;
  }
  else{
    struct ArgsHelperType tmp=get_args(get_kth_child(args,3));
    ret.irList=cat_IRList(2,ret.irList,tmp.irList);
    tmp.argListTail->next=newArgNode;
    ret.argListHead=tmp.argListHead;
    ret.argListTail=newArgNode;
    return ret;
  }
  assert(0);
}

struct IRListPair cond2IR(struct TreeNode* exp,struct IROpr trueLabel,struct IROpr falseLabel){
  if(childrens_are(exp,"Exp RELOP Exp")){
    struct IROpr t1=new_tmp_var(),t2=new_tmp_var();
    struct IRListPair code1=Exp2IR(get_kth_child(exp,1),t1,true),code2=Exp2IR(get_kth_child(exp,3),t2,true);
    struct IRListPair code3=new_single_IRList(IRTYPE_IFGOTO,4,t1,get_kth_child(exp,2)->value.relopEnum,t2,trueLabel);
    return cat_IRList(4,code1,code2,code3,new_single_IRList(IRTYPE_GOTO,1,falseLabel));
  }
  else if(childrens_are(exp,"NOT Exp")){
    return cond2IR(get_kth_child(exp,2),falseLabel,trueLabel);
  }
  else if(childrens_are(exp,"Exp AND Exp")){
    struct IROpr l1=new_label();
    struct IRListPair code1=cond2IR(get_kth_child(exp,1),l1,falseLabel);
    struct IRListPair code2=cond2IR(get_kth_child(exp,3),trueLabel,falseLabel);
    return cat_IRList(3,code1,new_single_IRList(IRTYPE_LABEL,1,l1),code2);
  }
  else if(childrens_are(exp,"Exp OR Exp")){
    struct IROpr l1=new_label();
    struct IRListPair code1=cond2IR(get_kth_child(exp,1),trueLabel,l1);
    struct IRListPair code2=cond2IR(get_kth_child(exp,3),trueLabel,falseLabel);
    return cat_IRList(3,code1,new_single_IRList(IRTYPE_LABEL,1,l1),code2);
  }
  else{
    struct IROpr t1=new_tmp_var();
    struct IRListPair code1=Exp2IR(exp,t1,true);
    struct IRListPair code2=new_single_IRList(IRTYPE_IFGOTO,4,t1,RELOP_NEQ,new_num(0),trueLabel);
    return cat_IRList(3,code1,code2,new_single_IRList(IRTYPE_GOTO,1,falseLabel));
  }
}

struct IRListPair Exp2IR(struct TreeNode* exp,struct IROpr place,bool hasPlace){
  if(childrens_are(exp,"INT")){
    if(hasPlace){
      return new_single_IRList(IRTYPE_ASSIGN,2,place,new_num(get_kth_child(exp,1)->value.intVal));
    }
  }
  else if(childrens_are(exp,"ID")){
    struct LookupType id=lookup(exp);
    if(hasPlace){
      if(id.irPos.type==IRPOS_VAR){
        if(id.type->kind==BASIC){
          id.irList=cat_IRList(2,id.irList,new_single_IRList(IRTYPE_ASSIGN,2,place,id.irPos.irOpr));
        }
        else{
          assert(0);
          // id.irList=cat_IRList(2,id.irList,new_single_IRList(IRTYPE_VA,2,place,id.irPos.irOpr));
        }
      }
      else{//IRPOS_ADDR
        if(id.type->kind==BASIC){
          id.irList=cat_IRList(2,id.irList,new_single_IRList(IRTYPE_VL,2,place,id.irPos.irOpr));
        }
        else{
          id.irList=cat_IRList(2,id.irList,new_single_IRList(IRTYPE_ASSIGN,2,place,id.irPos.irOpr));
        }
      }
      return id.irList;
    }
    else{
      return (struct IRListPair){NULL,NULL};
    }
  }
  else if(childrens_are(exp,"Exp ASSIGNOP Exp")){
    struct IRListPair retList=(struct IRListPair){NULL,NULL};

    struct LookupType dst=lookup(get_kth_child(exp,1));
    retList = cat_IRList(2, retList, dst.irList);
    if (dst.type->kind == BASIC) {
      struct IROpr rightIROpr = new_tmp_var();
      retList = cat_IRList(2,retList,Exp2IR(get_kth_child(exp, 3), rightIROpr, true));
      if (dst.irPos.type == IRPOS_VAR) {  // dst is a var
        retList = cat_IRList(2, retList, new_single_IRList(IRTYPE_ASSIGN, 2, dst.irPos.irOpr, rightIROpr));
      } else {  // dst is addr
        // assert(0);
        retList = cat_IRList(2, retList, new_single_IRList(IRTYPE_LV, 2, dst.irPos.irOpr, rightIROpr));
      }
      if (hasPlace) {
        retList = cat_IRList(2, retList, new_single_IRList(IRTYPE_ASSIGN, 2, place, rightIROpr));
      }
    } else {
      struct LookupType src = lookup(get_kth_child(exp, 3));
      
      get_size_of(dst.type);
      get_size_of(src.type);
      int sz = min(dst.type->sizeOf, src.type->sizeOf);

      /*struct IROpr dstAddrOpr = new_tmp_var(), srcAddrOpr = new_tmp_var();

      if (dst.irPos.type == IRPOS_VAR) {
        assert(0);
        retList = cat_IRList(2, retList, new_single_IRList(IRTYPE_ASSIGN, 2, dstAddrOpr, get_addr_of_var(dst.irPos.irOpr)));
      } else {//dst.irPos.type==IRPOS_ADDR
        retList=cat_IRList(2,retList,new_single_IRList(IRTYPE_ASSIGN,2,dstAddrOpr,dst.irPos.irOpr));
      }

      if (src.irPos.type == IRPOS_VAR) {
        assert(0);
        retList = cat_IRList(2, retList, new_single_IRList(IRTYPE_ASSIGN, 2, srcAddrOpr, get_addr_of_var(src.irPos.irOpr)));
      } else {//src.irPos.type==IRPOS_ADDR
        retList=cat_IRList(2,retList,new_single_IRList(IRTYPE_ASSIGN,2,srcAddrOpr,src.irPos.irOpr));
      }*/

      for (int i = 0; i < (sz >> 2); ++i) {
        struct IROpr t1=new_tmp_var(),t2=new_tmp_var(),t3=new_tmp_var();
        struct IROpr offset=new_num(i<<2);
        retList = cat_IRList(2, retList, new_single_IRList(IRTYPE_PLUS, 3, t1, dst.irPos.irOpr, offset));  // t1=dstAddrOpr + i*4
        retList = cat_IRList(2, retList, new_single_IRList(IRTYPE_PLUS, 3, t2, src.irPos.irOpr, offset));  // t2=srcAddrOpr + i*4
        retList = cat_IRList(2, retList, new_single_IRList(IRTYPE_VL, 2, t3, t2));                      // t3=*t2
        retList = cat_IRList(2, retList, new_single_IRList(IRTYPE_LV, 2, t1, t3));                      //*t1=t3
      }
    }
    return retList;
  }
  else if(childrens_are(exp,"Exp PLUS Exp")){
    //assert(hasPlace);
    //if(!hasPlace)return (struct IRListPair){NULL,NULL};
    struct IROpr t1=new_tmp_var(),t2=new_tmp_var();
    struct IRListPair code1=Exp2IR(get_kth_child(exp,1),t1,true);
    struct IRListPair code2=Exp2IR(get_kth_child(exp,3),t2,true);
    struct IRListPair code3=new_single_IRList(IRTYPE_PLUS,3,place,t1,t2);
    return cat_IRList(3,code1,code2,code3);
  }
  else if (childrens_are(exp,"Exp MINUS Exp")){
    //if(!hasPlace)return (struct IRListPair){NULL,NULL};
    struct IROpr t1=new_tmp_var(),t2=new_tmp_var();
    struct IRListPair  code1=Exp2IR(get_kth_child(exp,1),t1,true);
    struct IRListPair code2=Exp2IR(get_kth_child(exp,3),t2,true);
    if(!hasPlace){
      return cat_IRList(2,code1,code2);
    }
    struct IRListPair code3=new_single_IRList(IRTYPE_MINUS,3,place,t1,t2);
    return cat_IRList(3,code1,code2,code3);
  }
  else if (childrens_are(exp,"Exp STAR Exp")){
    //if(!hasPlace)return (struct IRListPair){NULL,NULL};
    struct IROpr t1=new_tmp_var(),t2=new_tmp_var();
    struct IRListPair code1=Exp2IR(get_kth_child(exp,1),t1,true);
    struct IRListPair code2=Exp2IR(get_kth_child(exp,3),t2,true);
    if(!hasPlace){
      return cat_IRList(2,code1,code2);
    }
    struct IRListPair code3=new_single_IRList(IRTYPE_MUL,3,place,t1,t2);
    return cat_IRList(3,code1,code2,code3);
  }
  else if (childrens_are(exp,"Exp DIV Exp")){
    //if(!hasPlace)return (struct IRListPair){NULL,NULL};
    struct IROpr t1=new_tmp_var(),t2=new_tmp_var();
    struct IRListPair code1=Exp2IR(get_kth_child(exp,1),t1,true);
    struct IRListPair code2=Exp2IR(get_kth_child(exp,3),t2,true);
    if(!hasPlace){
      return cat_IRList(2,code1,code2);
    }
    struct IRListPair code3=new_single_IRList(IRTYPE_DIV,3,place,t1,t2);
    return cat_IRList(3,code1,code2,code3);
  }
  else if (childrens_are(exp,"MINUS Exp")){
    //if(!hasPlace)return (struct IRListPair){NULL,NULL};
    struct IROpr t1=new_tmp_var();
    struct IROpr zero=new_num(0);
    struct IRListPair code1=Exp2IR(get_kth_child(exp,2),t1,true);
    if(!hasPlace){
      return code1;
    }
    struct IRListPair code2=new_single_IRList(IRTYPE_MINUS,3,place,zero,t1);
    return cat_IRList(2,code1,code2);
  }
  else if(childrens_are(exp,"LP Exp RP")){
    return Exp2IR(get_kth_child(exp,2),place,hasPlace);
  }
  else if(childrens_are(exp,"Exp DOT ID")){
    can_not_translate();
  }
  else if(childrens_are(exp,"Exp LB Exp RB")){
    //if(!hasPlace)return (struct IRListPair){NULL,NULL};
    struct LookupType t=lookup(exp);
    if(!hasPlace){
      return t.irList;
    }
    struct IRListPair code;
    if(t.irPos.type==IRPOS_VAR){
      assert(0);
      code=new_single_IRList(IRTYPE_ASSIGN,2,place,t.irPos.irOpr);
    }
    else{//IRPOS_ADDR
      if(t.type->kind==BASIC){
        code=new_single_IRList(IRTYPE_VL,2,place,t.irPos.irOpr);
      }
      else{//Array
        code=new_single_IRList(IRTYPE_ASSIGN,2,place,t.irPos.irOpr);
      }
    }
    return cat_IRList(2,t.irList,code);
  }
  else if(childrens_are(exp,"Exp RELOP Exp")||childrens_are(exp,"Exp AND Exp")||childrens_are(exp,"Exp OR Exp")||childrens_are(exp,"NOT Exp")){
    if(!hasPlace)place=(struct IROpr){IROPR_TMPVAR,0};
    struct IROpr l1=new_label(),l2=new_label();
    struct IRListPair code0=new_single_IRList(IRTYPE_ASSIGN,2,place,new_num(0));
    struct IRListPair code1=cond2IR(exp,l1,l2);
    struct IRListPair code2=cat_IRList(2,new_single_IRList(IRTYPE_LABEL,1,l1),new_single_IRList(IRTYPE_ASSIGN,2,place,new_num(1)));
    return cat_IRList(4,code0,code1,code2,new_single_IRList(IRTYPE_LABEL,1,l2));
  }
  else if(childrens_are(exp,"ID LP RP")){
    if(str_equal(get_kth_child(exp,1)->detail,"read")){
      return new_single_IRList(IRTYPE_READ,1,place);
    }
    struct IROpr funcIROpr=find_name(funcRoot,get_kth_child(exp,1)->detail)->outPtr.funcInfo.irOpr;
    if(!hasPlace){
      place.oprType=IROPR_TMPVAR;
      place.val=0;
    }
    return new_single_IRList(IRTYPE_CALL,2,place,funcIROpr);
  }
  else if(childrens_are(exp,"ID LP Args RP")){
    struct ArgsHelperType argsHelper=get_args(get_kth_child(exp,3));
    if(str_equal(get_kth_child(exp,1)->detail,"write")){
      struct IRListPair code=cat_IRList(2,argsHelper.irList,new_single_IRList(IRTYPE_WRITE,1,argsHelper.argListHead->irOpr));
      if(hasPlace){
        code=cat_IRList(2,code,new_single_IRList(IRTYPE_ASSIGN,2,place,new_num(0)));
      }
      return code;
    }
    struct IROpr funcIROpr=find_name(funcRoot,get_kth_child(exp,1)->detail)->outPtr.funcInfo.irOpr;
    struct IRListPair code=argsHelper.irList;
    for(struct IROprListNode* o=argsHelper.argListHead;o!=NULL;o=o->next){
       code=cat_IRList(2,code,new_single_IRList(IRTYPE_ARG,1,o->irOpr));
    }
    if(!hasPlace){
      place.oprType=IROPR_TMPVAR;
      place.val=0;
    }
    code=cat_IRList(2,code,new_single_IRList(IRTYPE_CALL,2,place,funcIROpr));
    return code;
  }
  //assert(0);
  return (struct IRListPair){NULL,NULL};
}


struct TrieNode *define_VarDec(struct TreeNode *vardec){
  if(childrens_are(vardec,"ID")){
    struct TreeNode *id=get_kth_child(vardec,1);
    struct TrieNode *idTrieNode=find_name(varRoot,id->detail);
    if(idTrieNode->outPtr.varDefHead->type->kind==BASIC){
      idTrieNode->outPtr.varDefHead->irPos.type=IRPOS_VAR;
      idTrieNode->outPtr.varDefHead->irPos.irOpr=new_def_var();
    }
    else{
      idTrieNode->outPtr.varDefHead->irPos.type=IRPOS_ADDR;
      idTrieNode->outPtr.varDefHead->irPos.irOpr=get_addr_of_var(new_def_var());
    }
    return idTrieNode;
  }
  else{//VarDec LB INT RB
    return define_VarDec(get_kth_child(vardec,1));
  }
}

struct IRListPair DecList2IR(struct TreeNode *declist){
  //first child must be Dec
  struct TreeNode *dec=get_kth_child(declist,1);
  struct TrieNode *newVar=define_VarDec(get_kth_child(dec,1));
  get_size_of(newVar->outPtr.varDefHead->type);
  struct IRListPair code=(struct IRListPair){NULL,NULL};
  if(newVar->outPtr.varDefHead->type->kind!=BASIC){
    code=new_single_IRList(IRTYPE_DEC,2,remove_addr(newVar->outPtr.varDefHead->irPos.irOpr),new_num(newVar->outPtr.varDefHead->type->sizeOf));
  }
  if(childrens_are(dec,"VarDec ASSIGNOP Exp")){// Dec=VarDec ASSIGNOP Exp
    struct IRListPair code2=Exp2IR(get_kth_child(dec,3),newVar->outPtr.varDefHead->irPos.irOpr,true);
    code=cat_IRList(2,code,code2);
  }
  if(childrens_are(declist,"Dec COMMA DecList")){//Dec COMMA DecList
    struct IRListPair code3=DecList2IR(get_kth_child(declist,3));
    code=cat_IRList(2,code,code3);
  }
  return code;
}

struct IRListPair DefList2IR(struct TreeNode *deflist){
  //Def DefList
  if(deflist->line==-1){// empty
    return (struct IRListPair){NULL,NULL};
  }
  struct TreeNode *def=get_kth_child(deflist,1);
  struct TreeNode *declist=get_kth_child(def,2);
  struct IRListPair code1=DecList2IR(declist);
  struct IRListPair code2=DefList2IR(get_kth_child(deflist,2));//Def DefList1
  return cat_IRList(2,code1,code2);
}


struct IRListPair CompSt2IR(struct TreeNode *compst);

struct IRListPair Stmt2IR(struct TreeNode* stmt){
  if(childrens_are(stmt,"Exp SEMI")){
    return Exp2IR(get_kth_child(stmt,1),(struct IROpr){0,0},false);
  }
  if(childrens_are(stmt,"CompSt")){
    return CompSt2IR(get_kth_child(stmt,1));
  }
  if(childrens_are(stmt,"RETURN Exp SEMI")){
    struct IROpr t1=new_tmp_var();
    struct IRListPair code1=Exp2IR(get_kth_child(stmt,2),t1,true);
    return cat_IRList(2,code1,new_single_IRList(IRTYPE_RETURN,1,t1));
  }
  if(childrens_are(stmt,"IF LP Exp RP Stmt")){
    struct IROpr l1=new_label(),l2=new_label();
    struct IRListPair code1=cond2IR(get_kth_child(stmt,3),l1,l2);
    struct IRListPair code2=Stmt2IR(get_kth_child(stmt,5));
    return cat_IRList(4,code1,new_single_IRList(IRTYPE_LABEL,1,l1),code2,new_single_IRList(IRTYPE_LABEL,1,l2));
  }
  if(childrens_are(stmt,"IF LP Exp RP Stmt ELSE Stmt")){
    struct IROpr l1=new_label(),l2=new_label(),l3=new_label();
    struct IRListPair code1=cond2IR(get_kth_child(stmt,3),l1,l2);
    struct IRListPair code2=Stmt2IR(get_kth_child(stmt,5));
    struct IRListPair code3=Stmt2IR(get_kth_child(stmt,7));
    return cat_IRList(7,code1,new_single_IRList(IRTYPE_LABEL,1,l1),code2,new_single_IRList(IRTYPE_GOTO,1,l3),new_single_IRList(IRTYPE_LABEL,1,l2),code3,new_single_IRList(IRTYPE_LABEL,1,l3));
  }
  if(childrens_are(stmt,"WHILE LP Exp RP Stmt")){
    struct IROpr l1=new_label(),l2=new_label(),l3=new_label();
    struct IRListPair code1=cond2IR(get_kth_child(stmt,3),l2,l3);
    struct IRListPair code2=Stmt2IR(get_kth_child(stmt,5));
    return cat_IRList(6,new_single_IRList(IRTYPE_LABEL,1,l1),code1,new_single_IRList(IRTYPE_LABEL,1,l2),code2,new_single_IRList(IRTYPE_GOTO,1,l1),new_single_IRList(IRTYPE_LABEL,1,l3));
  }
  assert(0);
}

struct IRListPair StmtList2IR(struct TreeNode *stmtlist){
  if(stmtlist->line==-1){//empty
    return (struct IRListPair){NULL,NULL};
  }
  //StmtList = Stmt StmtList1
  assert(childrens_are(stmtlist,"Stmt StmtList")); 
  struct IRListPair stmtir=Stmt2IR(get_kth_child(stmtlist,1));
  return cat_IRList(2,stmtir,StmtList2IR(get_kth_child(stmtlist,2)));
}

struct IRListPair CompSt2IR(struct TreeNode *compst){
  //Compst = LC DefList StmtList RC 
  struct IRListPair deflistir=DefList2IR(get_kth_child(compst,2));
  struct IRListPair stmtlistir=StmtList2IR(get_kth_child(compst,3));
  return cat_IRList(2,deflistir,stmtlistir);
}

struct IRListPair define_params_and_2IR(struct TreeNode *now){
  if(now->line==-1)return (struct IRListPair){NULL,NULL};
  if(str_equal(now->defName,"ID")){
    struct IROpr p1=new_def_var();
    struct TrieNode *idTrieNode=find_name(varRoot,now->detail);
    assert(idTrieNode!=NULL);
    if(idTrieNode->outPtr.varDefHead->type->kind==BASIC){
      idTrieNode->outPtr.varDefHead->irPos=(struct IRPos){IRPOS_VAR,p1};
    }
    else{
      idTrieNode->outPtr.varDefHead->irPos=(struct IRPos){IRPOS_ADDR,p1};
    }
    return new_single_IRList(IRTYPE_PARAM,1,p1);
  }
  struct IRListPair code=(struct IRListPair){NULL,NULL};
  for(struct LinkNode *link=now->sonListHead;link!=NULL;link=link->next){
    code=cat_IRList(2,code,define_params_and_2IR(link->to));
  }
  return code;
}

struct IRListPair FunDec2IR(struct TreeNode *fundec){
  //first child must be ID
  struct TreeNode *id=get_kth_child(fundec,1);
  struct TrieNode *funcTrieNode=find_name(funcRoot,id->detail);
  if(str_equal(id->detail,"main")){
    funcTrieNode->outPtr.funcInfo.irOpr=(struct IROpr){IROPR_FUNC,0};
  }
  else{
    funcTrieNode->outPtr.funcInfo.irOpr=new_func();
  }
  struct IRListPair code=new_single_IRList(IRTYPE_FUNC,1,funcTrieNode->outPtr.funcInfo.irOpr);
  if(childrens_are(fundec,"ID LP VarList RP")){
    code=cat_IRList(2,code,define_params_and_2IR(get_kth_child(fundec,3)));
  }
  return code;
}

struct IRListPair Func2IR(struct TreeNode *funcExtDef){
  struct IRListPair fundecir=FunDec2IR(get_kth_child(funcExtDef,2));
  struct IRListPair compstir=CompSt2IR(get_kth_child(funcExtDef,3));
  return cat_IRList(2,fundecir,compstir); 
}

struct IRListPair ExtDefList2IR(struct TreeNode *extDefList){
  if(extDefList->line==-1)return (struct IRListPair){NULL,NULL};
  assert(childrens_are(extDefList,"ExtDef ExtDefList"));
  struct TreeNode *extdef=get_kth_child(extDefList,1);
  if(childrens_are(extdef,"Specifier ExtDecList SEMI")){
    can_not_translate();
    assert(0);
  }

  struct IRListPair code=(struct IRListPair){NULL,NULL};
  if(childrens_are(extdef,"Specifier FunDec CompSt")){
    code=Func2IR(extdef);
  }
  code=cat_IRList(2,code,ExtDefList2IR(get_kth_child(extDefList,2)));
  return code;
}

struct IRListPair Program2IR(struct TreeNode *program){
  assert(childrens_are(program,"ExtDefList"));
  return ExtDefList2IR(get_kth_child(program,1));
}

void print_IROpr(struct IROpr iropr){
  if(iropr.addrOf==true)printf("&");
  switch (iropr.oprType) {
    case IROPR_DEFVAR:
      printf("v%d",iropr.val);
      break;
    case IROPR_TMPVAR:
      printf("t%d",iropr.val);
      break;
    case IROPR_LABEL:
      printf("label%d",iropr.val);
      break;
    case IROPR_FUNC:
      if(iropr.val==0)printf("main");
      else printf("function%d",iropr.val);
      break;
    case IROPR_NUM:
      printf("#%d",iropr.val);
      break;
    default:
      assert(0); 
  }
}
void print_relop(enum RelopEnum relop){
  switch (relop) {
    case RELOP_EQ:
      printf(" == ");
      break;
    case RELOP_NEQ:
      printf(" != ");
      break;
    case RELOP_GR:
      printf(" > ");
      break;
    case RELOP_LE:
      printf(" < ");
      break;
    case RELOP_GEQ:
      printf(" >= ");
      break;
    case RELOP_LEQ:
      printf(" <= ");
      break;
  }
}

void print_IR(struct IRListPair irList){
  for(struct IRNode *irNode=irList.head;irNode!=NULL;irNode=irNode->nxt){
    if(irNode->deleted==true)continue;
    switch (irNode->irType) {
      case IRTYPE_LABEL:
        printf("LABEL label%d :",irNode->x.val);
        break;
      case IRTYPE_FUNC:
        if(irNode->x.val==0)printf("FUNCTION main :");
        else printf("FUNCTION function%d :",irNode->x.val);
        break;
      case IRTYPE_ASSIGN:
        print_IROpr(irNode->x);
        printf(" := ");
        print_IROpr(irNode->y);
        break;
      case IRTYPE_PLUS:
        print_IROpr(irNode->x);
        printf(" := ");
        print_IROpr(irNode->y);
        printf(" + ");
        print_IROpr(irNode->z);
        break;
      case IRTYPE_MINUS:
        print_IROpr(irNode->x);
        printf(" := ");
        print_IROpr(irNode->y);
        printf(" - ");
        print_IROpr(irNode->z);
        break;
      case IRTYPE_MUL:
        print_IROpr(irNode->x);
        printf(" := ");
        print_IROpr(irNode->y);
        printf(" * ");
        print_IROpr(irNode->z);
        break;
      case IRTYPE_DIV:
        print_IROpr(irNode->x);
        printf(" := ");
        print_IROpr(irNode->y);
        printf(" / ");
        print_IROpr(irNode->z);
        break;
      /*case IRTYPE_VA:
        print_IROpr(irNode->x);
        printf(" := &");
        print_IROpr(irNode->y);
        break;*/
      case IRTYPE_VL:
        print_IROpr(irNode->x);
        printf(" := *");
        print_IROpr(irNode->y);
        break;
      case IRTYPE_LV:
        printf("*");
        print_IROpr(irNode->x);
        printf(" := ");
        print_IROpr(irNode->y);
        break;
      case IRTYPE_GOTO:
        printf("GOTO ");
        print_IROpr(irNode->x); 
        break;
      case IRTYPE_IFGOTO:
        printf("IF ");
        print_IROpr(irNode->x);
        print_relop(irNode->relop);
        print_IROpr(irNode->y);
        printf(" GOTO ");
        print_IROpr(irNode->z);
        break;
      case IRTYPE_RETURN:
        printf("RETURN ");
        print_IROpr(irNode->x);
        break;
      case IRTYPE_DEC:
        printf("DEC ");
        print_IROpr(irNode->x);
        printf(" %d",irNode->y.val);
        break;
      case IRTYPE_ARG:
        printf("ARG ");
        print_IROpr(irNode->x);
        break;
      case IRTYPE_CALL:
        print_IROpr(irNode->x);
        printf(" := CALL ");
        print_IROpr(irNode->y);
        break;
      case IRTYPE_PARAM:
        printf("PARAM ");
        print_IROpr(irNode->x);
        break;
      case IRTYPE_READ:
        printf("READ ");
        print_IROpr(irNode->x);
        break;
      case IRTYPE_WRITE:
        printf("WRITE ");
        print_IROpr(irNode->x);
        break;
      default:
        assert(0);
    }
    printf("\n");
  }
}

void lab3(){
  struct IRListPair irList=Program2IR(programParseTree);
  if(!cannotTranslate){
    print_IR(irList);
  }
}

bool has_struct(struct TreeNode *now){
  if(str_equal(now->defName,"STRUCT")){
    return true;
  }
  for(struct LinkNode* lnk=now->sonListHead;lnk!=NULL;lnk=lnk->next){
    bool tmp=has_struct(lnk->to);
    if(tmp)return true;
  }
  return false;
}
