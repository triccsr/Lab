#include "ID_trie.h"
#include "error_def.h"
#include "parse_tree.h"
#include "stddef.h"
#include "stdbool.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

struct TrieNode *varRoot, *funcRoot, *structNameRoot;
struct AnonymousStructListNode* anonymousStructListHead;

bool str_equal(const char s1[],const char s2[]){
  return (strcmp(s1, s2)==0);
}

int prevLine,prevErrorType;
void parse_error(int errorType,int line,char *msg){
  if(errorType==prevErrorType&&line==prevLine)return;
  prevErrorType=errorType;
  prevLine=line;
  printf("Error type %d at Line %d: %s\n",errorType,line,msg);
}

struct TypeNode* specifier2type(struct TreeNode *now);

typedef struct{
  struct VarNameListNode* head;
  struct VarNameListNode* tail;
}VarNamePair;

VarNamePair merge_var_name_list(VarNamePair a,VarNamePair b){
  if(a.head==NULL)return b;
  if(b.head==NULL)return a;
  assert(a.tail!=NULL&&b.tail!=NULL);
  a.tail->next=b.head;
  return (VarNamePair){a.head,b.tail};
}

bool same_type(struct TypeNode *typeA,struct TypeNode* typeB){
  if(typeA==NULL||typeB==NULL)return false;
  if(typeA->kind!=typeB->kind)return false;
  if(typeA->kind==BASIC)return (typeA->u.basic==typeB->u.basic);
  else if(typeA->kind==ARRAY){
    return same_type(typeA->u.array.elem,typeB->u.array.elem);
  }
  else if(typeA->kind==STRUCTURE){
    return typeA->u.structure==typeB->u.structure;
  }
  assert(0);
}

void get_size_of(struct TypeNode *type){
  if(type->sizeOf>0)return;
  if(type->kind==BASIC){
    type->sizeOf=4;
  }
  else if(type->kind==ARRAY){
    get_size_of(type->u.array.elem);
    type->sizeOf=type->u.array.elem->sizeOf*type->u.array.size;
  }
  else{//structure
    for(struct VarNameListNode *field=type->u.structure;field!=NULL;field=field->next){
      get_size_of(field->type);
      type->sizeOf+=field->type->sizeOf;
    }
  }
}

void insert_var(struct VarNameListNode* tmpVar,int env){
    if(find_name(structNameRoot,tmpVar->name)!=NULL){// var name can be found in struct trie
      char msg[256];
      sprintf(msg, "duplicate name %s with struct",tmpVar->name);
      parse_error(3, tmpVar->line, msg);
      //return;
    }
    //if(tmpVar->type==NULL)continue;
    struct TrieNode* varNameTrieNode=find_name(varRoot, tmpVar->name);//find var name in trie
    if(varNameTrieNode==NULL){//not in trie, insert one
      varNameTrieNode=insert_name(varRoot,tmpVar->name);
    }
    if(varNameTrieNode->outPtr.varDefHead!=NULL&&varNameTrieNode->outPtr.varDefHead->envLine==env){//duplicate define var
      char msg[256];
      sprintf(msg, "duplicate define %s",tmpVar->name);
      parse_error(3, tmpVar->line, msg);
      //return (struct IRListPair){NULL,NULL};
    }
    struct VarDefNode* newVarDef=calloc(1,sizeof(struct VarDefNode));
    newVarDef->line=tmpVar->line;
    newVarDef->envLine=env;
    newVarDef->type=tmpVar->type;
    newVarDef->next=varNameTrieNode->outPtr.varDefHead;
    
    //for ir
    //newVarDef->irPos.irOpr=new_def_var();
    //newVarDef->irPos.type=IRPOS_VAR;
    //if(newVarDef->type->sizeOf==0){
    //  get_size_of(newVarDef->type);
    //}
    //return new_single_IRList(IRTYPE_DEC,2,newVarDef->irPos.irOpr,new_num(newVarDef->type->sizeOf)); 
     
    
    varNameTrieNode->outPtr.varDefHead=newVarDef;
}

struct TypeNode* get_Exp_type(struct TreeNode *expTreeNode,int env);

void check_args(struct TreeNode *now,int env,struct VarNameListNode* curVarNameListNode){ //return list tail
  struct TreeNode* curExpTreeNode=get_kth_child(now, 1);
  struct TypeNode* paramType=get_Exp_type(curExpTreeNode,env);
  if(curVarNameListNode==NULL||(paramType!=NULL&&curVarNameListNode->type!=NULL&&!same_type(paramType, curVarNameListNode->type))){
    parse_error(9,curExpTreeNode->line,"");
  }
  if(now->childNum==3){
    check_args(get_kth_child(now, 3), env, (curVarNameListNode==NULL)?NULL:curVarNameListNode->next);
  }
  else if(curVarNameListNode!=NULL&&curVarNameListNode->next!=NULL){//args end, param should end
    parse_error(9, curExpTreeNode->line,"args not enough");
  }
}

struct TypeNode* get_Exp_type(struct TreeNode *expTreeNode,int env){
  assert(str_equal(expTreeNode->defName, "Exp"));
  struct TypeNode* retType=calloc(1,sizeof(struct TypeNode));
  if(expTreeNode->childNum==1){
    if(str_equal(get_kth_child(expTreeNode,1)->defName,"INT")){
      retType->kind=BASIC;
      retType->u.basic=0;//int
      return retType;
    }
    if(str_equal(get_kth_child(expTreeNode,1)->defName,"FLOAT")){
      retType->kind=BASIC;
      retType->u.basic=1;//float
      return retType;
    }
    if(str_equal(get_kth_child(expTreeNode,1)->defName, "ID")){//Var
      struct TrieNode* varTrieNode=find_name(varRoot, get_kth_child(expTreeNode,1)->detail);
      if(varTrieNode==NULL||varTrieNode->outPtr.varDefHead==NULL){
        parse_error(1, expTreeNode->line,"");
        return NULL;
      }
      return varTrieNode->outPtr.varDefHead->type;
    }
  }
  else if(expTreeNode->childNum==2){// MINUS NOT
    struct TypeNode* rightExp=get_Exp_type(get_kth_child(expTreeNode, 2), env);
    if(rightExp==NULL)return NULL;
    if(rightExp->kind!=BASIC){
      parse_error(7, expTreeNode->line,"");
      return NULL;
    }
    return rightExp;
  }
  else if(str_equal(get_kth_child(expTreeNode, 1)->defName,"LP")){//LP EXP RP
    return get_Exp_type(get_kth_child(expTreeNode, 2), env);
  }
  else if (str_equal(get_kth_child(expTreeNode, 1)->defName, "ID")){//Function
    struct TreeNode* funcIDTreeNode=get_kth_child(expTreeNode, 1);
    if(str_equal(funcIDTreeNode->detail,"read")){
      retType->kind=BASIC;
      retType->u.basic=0;//INT
      return retType;
    }
    if(str_equal(funcIDTreeNode->detail,"write")){
      return NULL;
    }
    struct TrieNode* funcTrieNode=find_name(funcRoot, funcIDTreeNode->detail);
    if(funcTrieNode==NULL){// if func ID is not in funcTrie, 
      funcTrieNode=find_name(varRoot, funcIDTreeNode->detail);
      if(funcTrieNode!=NULL){
        parse_error(11, funcIDTreeNode->line,"");
      }
      else{
        parse_error(2, funcIDTreeNode->line,"");
      }
      return NULL;
    }
    //func ID is in func trie
    if(expTreeNode->childNum==3){//ID LP RP
      if(funcTrieNode->outPtr.funcInfo.paramListHead!=NULL){// param is not empty but args is
        parse_error(9, funcIDTreeNode->line,"");
      }
    }
    else{// ID LP Args RP
      check_args(get_kth_child(expTreeNode, 3), env, funcTrieNode->outPtr.funcInfo.paramListHead);
    }
    return funcTrieNode->outPtr.funcInfo.returnType;
  }
  else if(str_equal(get_kth_child(expTreeNode, 2)->defName, "DOT")){//a.b
    struct TreeNode* leftExp=get_kth_child(expTreeNode, 1);
    struct TypeNode* leftExpType=get_Exp_type(leftExp, env);
    if(leftExpType==NULL||leftExpType->kind!=STRUCTURE){//a is not a struct
      if(leftExpType!=NULL){
        parse_error(13, leftExp->line, "");
      }
      return NULL;
    }
    struct TreeNode *IDTreeNode=get_kth_child(expTreeNode, 3);
    struct TypeNode *fieldType=NULL;
    for(struct VarNameListNode* field=leftExpType->u.structure;field!=NULL;field=field->next){
      if(str_equal(field->name, IDTreeNode->detail)){
        fieldType=field->type;
        break;
      }
    }
    if(fieldType==NULL){//unknown field
      char msg[256];
      sprintf(msg, "Unknown field %s",IDTreeNode->detail);
      parse_error(14, IDTreeNode->line,msg);
      return NULL;
    }
    return fieldType;
  }
  else if(str_equal(get_kth_child(expTreeNode, 2)->defName, "LB")){//a[i]
    struct TypeNode* arrayType=get_Exp_type(get_kth_child(expTreeNode, 1),env);
    if(arrayType==NULL)return NULL;
    if(arrayType->kind!=ARRAY){// not an array
      parse_error(10, get_kth_child(expTreeNode, 1)->line, "");
      return NULL;
    }
    struct TypeNode* indexType=get_Exp_type(get_kth_child(expTreeNode, 3), env);
    if(indexType==NULL)return NULL;
    if(indexType->kind!=BASIC||indexType->u.basic!=0){//index is not int
      parse_error(12, get_kth_child(expTreeNode, 3)->line,"");
    }
    return arrayType->u.array.elem;
  }
  else{//Exp sth Exp
    struct TreeNode* leftTreeNode=get_kth_child(expTreeNode, 1);
    struct TypeNode* leftType=get_Exp_type(leftTreeNode, env);
    struct TypeNode* rightType=get_Exp_type(get_kth_child(expTreeNode, 3), env);
    struct TreeNode* op=get_kth_child(expTreeNode, 2);
    if(leftTreeNode==NULL||rightType==NULL)return NULL;
    if(str_equal(op->defName, "ASSIGNOP")){
      if(leftType!=NULL&&rightType!=NULL&&!same_type(leftType, rightType)){
        parse_error(5, expTreeNode->line, "");
        return NULL;
      }
      if((leftTreeNode->childNum==1&&str_equal(get_kth_child(leftTreeNode, 1)->defName,"ID"))||(leftTreeNode->childNum>=2&&(str_equal(get_kth_child(leftTreeNode, 2)->defName, "LB")||str_equal(get_kth_child(leftTreeNode, 2)->defName, "DOT")))){
        return leftType;
      }
      else{// left exp is not a left value
        parse_error(6, expTreeNode->line, "");
        return NULL;
      }
    }
    else{
      if(leftType!=NULL&&rightType!=NULL&&(!same_type(leftType, rightType)||leftType->kind!=BASIC)){//calculate type error
        parse_error(7, expTreeNode->line, "");
        return NULL;
      }
      return leftType;
    }
  }
  assert(0);
}

VarNamePair declist2var_name_list(struct TreeNode *now,struct TypeNode *type,bool allowAssign,bool modifyEnv,int env){
  if(str_equal(now->defName, "ID")){
    struct VarNameListNode *newVar=calloc(1,sizeof(struct VarNameListNode));
    newVar->name=strdup(now->detail);
    newVar->line=now->line;
    newVar->type=type;
    newVar->next=NULL;
    return (VarNamePair){newVar,newVar};
  }
  if(str_equal(now->defName, "VarDec")&&now->childNum==4){//VarDec LB INT RB
    struct TreeNode *smallerVarDecNode=get_kth_child(now, 1);
    VarNamePair s=declist2var_name_list(smallerVarDecNode, type, allowAssign,modifyEnv,env);
    assert(s.head==s.tail);//only one var
    struct TypeNode *arrayNode=calloc(1,sizeof(struct TypeNode));
    if(s.head->type->kind!=ARRAY){
      arrayNode->kind=ARRAY;
      arrayNode->u.array.size=get_kth_child(now, 3)->value.intVal;
      arrayNode->u.array.elem=s.head->type;
      s.head->type=arrayNode;
    }
    else{
      struct TypeNode* t=s.head->type;
      while(t->kind==ARRAY&&t->u.array.elem->kind==ARRAY)t=t->u.array.elem;
      assert(t->kind==ARRAY&&t->u.array.elem->kind!=ARRAY);
      arrayNode->kind=ARRAY;
      arrayNode->u.array.size=get_kth_child(now, 3)->value.intVal;
      arrayNode->u.array.elem=t->u.array.elem;
      t->u.array.elem=arrayNode;
    }

    return s;
  }
  if(str_equal(now->defName, "Dec")){
    if(now->childNum==3){
      if(!allowAssign){
        parse_error(15, get_kth_child(now, 2)->line,"");//ASSIGNOP
      }
      else{
        struct TypeNode *expType=get_Exp_type(get_kth_child(now, 3),env);
        if(expType!=NULL&&same_type(type, expType)==false){
          parse_error(5, get_kth_child(now, 2)->line,"");// different type assign
        }
      }
    }
    VarNamePair s=declist2var_name_list(now->sonListHead->to, type, allowAssign,modifyEnv,env);
    if(modifyEnv){
      insert_var(s.head,env);
    }
    return s;
  }
  VarNamePair ret=(VarNamePair){NULL,NULL};
  for(struct LinkNode* link=now->sonListHead;link!=NULL;link=link->next){
    VarNamePair s=declist2var_name_list(link->to, type, allowAssign,modifyEnv,env);
    ret=merge_var_name_list(ret, s);
  }
  return ret;
}


VarNamePair deflist2var_name_list(struct TreeNode *now,bool allowAssign,bool modifyEnv,int env){
  /*if(str_equal(now->defName, "empty")){
    return (VarNamePair){NULL,NULL};
  }*/
  if(now->line==-1){
    return (VarNamePair){NULL,NULL};
  }
  assert(str_equal(now->defName, "DefList"));
  struct TreeNode* defNode=get_kth_child(now, 1);
  assert(str_equal(defNode->defName, "Def"));
  struct TreeNode* speNode=get_kth_child(defNode, 1);
  assert(str_equal(speNode->defName, "Specifier"));
  VarNamePair ret=(VarNamePair){NULL,NULL};
  struct TypeNode* type=specifier2type(speNode);
  /*if(type!=NULL){//no type error
    struct TreeNode* decListNode=get_kth_child(defNode, 2);
    ret=declist2var_name_list(decListNode, type, allowAssign,env);   
  }*/
  struct TreeNode* decListNode=get_kth_child(defNode, 2);
  ret=declist2var_name_list(decListNode, type, allowAssign,modifyEnv,env);   
  
  struct TreeNode* smallerDefListNode=get_kth_child(now, 2); 
  VarNamePair ss=deflist2var_name_list(smallerDefListNode, allowAssign,modifyEnv,env);
  return merge_var_name_list(ret, ss);
}

struct TypeNode* specifier2type(struct TreeNode *now){
  assert(str_equal(now->defName, "Specifier"));
  struct TreeNode* firstChild=get_kth_child(now, 1);
  if(str_equal(firstChild->defName,"TYPE")){
    struct TypeNode *ret=calloc(1,sizeof(struct TypeNode));
    ret->kind=BASIC;
    if(str_equal(firstChild->detail, "int")){//int
      ret->u.basic=0;//int
    }
    else{
      ret->u.basic=1;//float
    }
    return ret;
  }
  else if(str_equal(firstChild->defName, "StructSpecifier")){//struct specifier
    if(firstChild->childNum==2){//struct tag
      struct TreeNode* tag=get_kth_child(firstChild, 2);
      assert(str_equal(tag->defName, "Tag"));
      struct TrieNode* tagNode=find_name(structNameRoot, tag->sonListHead->to->detail);
      if(tagNode==NULL){
        parse_error(17,firstChild->line,"");
        return NULL;
      }
      else{
        return tagNode->outPtr.type;
      }
    }
    else if(firstChild->childNum==5){//STRUCT OptTag LB  
      struct TreeNode* opttag=get_kth_child(firstChild, 2);
      struct TreeNode* deflist=get_kth_child(firstChild, 4);

      VarNamePair fieldPair=deflist2var_name_list(deflist,false,false,0);

      //check duplicate field name
      struct TrieNode *tmpFieldRoot=calloc(1,sizeof(struct TrieNode));
      for(struct VarNameListNode* varName=fieldPair.head;varName!=NULL;varName=varName->next){
        if(find_name(tmpFieldRoot, varName->name)==NULL){
          insert_name(tmpFieldRoot, varName->name);
        }
        else{
          parse_error(15, varName->line,"duplicate field name");
        }
      }
      delete_trie(tmpFieldRoot);

      struct TypeNode* structType=calloc(1,sizeof(struct TypeNode));
      structType->kind=STRUCTURE;
      structType->u.structure=fieldPair.head;

      if(opttag->line!=-1){//otherwise empty
        struct TrieNode* tagNode=find_name(structNameRoot, opttag->sonListHead->to->detail);
        if(tagNode==NULL){
          tagNode=find_name(varRoot, opttag->sonListHead->to->detail);
        }
        if(tagNode!=NULL){
          parse_error(16,firstChild->line,"");
          return NULL;
        }
        struct TrieNode* newStructNode= insert_name(structNameRoot, opttag->sonListHead->to->detail);
        newStructNode->outPtr.type=structType;
      }
      return structType;
    }
  }
  assert(0);
}

void insert_var_name_list(struct VarNameListNode* varNameListHead,int env){
  for(struct VarNameListNode *tmpVar=varNameListHead;tmpVar!=NULL;tmpVar=tmpVar->next){
    insert_var(tmpVar,env);
  }
}
void delete_var_name_list(struct VarNameListNode* varNameListHead,int env){
  for(struct VarNameListNode* tmpVar=varNameListHead;tmpVar!=NULL;tmpVar=tmpVar->next){
    struct TrieNode* varTrieNode=find_name(varRoot,tmpVar->name);
    //assert(varTrieNode!=NULL);
    if(varTrieNode==NULL)continue;
    if(varTrieNode->outPtr.varDefHead!=NULL&&varTrieNode->outPtr.varDefHead->envLine==env){
      struct VarDefNode* oldVarDef=varTrieNode->outPtr.varDefHead;
      varTrieNode->outPtr.varDefHead=oldVarDef->next;
      free(oldVarDef);
    }
  }
}

/*
bool var_defined(const char *varName,int env){
  struct TrieNode *varTrieNode=find_name(varRoot,varName);
  if(varTrieNode==NULL||varTrieNode->.varDefHead==NULL){
    return false;
  }
  return true;
}
*/


void check_CompSt(struct TreeNode* compstTreeNode,struct TypeNode* returnType);

void check_Stmt(struct TreeNode* stmtTreeNode,int env,struct TypeNode *returnType){
  assert(str_equal(stmtTreeNode->defName,"Stmt"));
  if(str_equal(get_kth_child(stmtTreeNode, 1)->defName,"CompSt")){
    check_CompSt(get_kth_child(stmtTreeNode, 1), returnType);
  }
  else if(str_equal(get_kth_child(stmtTreeNode,1)->defName,"RETURN")){
    struct TypeNode *expType=get_Exp_type(get_kth_child(stmtTreeNode, 2),env);
    if(returnType!=NULL&&expType!=NULL&&!same_type(returnType, expType)){
      parse_error(8, stmtTreeNode->line, "");
    }
  }
  else if(stmtTreeNode->childNum==2){
    get_Exp_type(get_kth_child(stmtTreeNode,1),env);
  }
  else{// if or while
    struct TypeNode *conditionExpType=get_Exp_type(get_kth_child(stmtTreeNode, 3),env);
    if(conditionExpType!=NULL&&(conditionExpType->kind!=BASIC||conditionExpType->u.basic!=0)){// conditions exp is not int
      parse_error(7, stmtTreeNode->line, "if or while's condition exp is not int");
    }
  }
  for(struct LinkNode *link=stmtTreeNode->sonListHead;link!=NULL;link=link->next){
    if(str_equal(link->to->defName,"Stmt")){
      check_Stmt(link->to, env, returnType);
    }
  }
}

void check_StmtList(struct TreeNode *stmtListTreeNode,int env,struct TypeNode* returnType){
  //if(str_equal(stmtListTreeNode->defName, "empty"))return;
  if(stmtListTreeNode->line==-1)return;
  assert(str_equal(stmtListTreeNode->defName,"StmtList"));
  struct TreeNode* stmtTreeNode=get_kth_child(stmtListTreeNode, 1);
  check_Stmt(stmtTreeNode, env, returnType);
  check_StmtList(get_kth_child(stmtListTreeNode, 2), env, returnType);
}

void check_CompSt(struct TreeNode* compstTreeNode,struct TypeNode* returnType){
  assert(str_equal(compstTreeNode->defName, "CompSt"));
  int env=compstTreeNode->line;
  VarNamePair defVarList=deflist2var_name_list(get_kth_child(compstTreeNode, 2), true,true,env);
  //insert_var_name_list(defVarList.head, env);
  check_StmtList(get_kth_child(compstTreeNode, 3), env, returnType);
  //delete_var_name_list(defVarList.head, env); 
}

VarNamePair param_varList2var_name_list(struct TreeNode *varListTreeNode){
  assert(str_equal(varListTreeNode->defName,"VarList"));
  struct TreeNode *paramDecTreeNode=get_kth_child(varListTreeNode, 1);
  struct TypeNode* paramType=specifier2type(get_kth_child(paramDecTreeNode, 1));
  VarNamePair vp=declist2var_name_list(get_kth_child(paramDecTreeNode, 2), paramType, false, false,0);
  if(varListTreeNode->childNum==3){
    struct TreeNode *smallerVarListTreeNode=get_kth_child(varListTreeNode, 3);
    VarNamePair svp=param_varList2var_name_list(smallerVarListTreeNode);
    vp=merge_var_name_list(vp, svp);
  }
  return vp;
}

struct TrieNode* check_FunDec(const struct TreeNode* funcDecTreeNode,struct TypeNode* returnType){
  assert(str_equal(funcDecTreeNode->defName, "FunDec"));
  struct TreeNode* FuncIDTreeNode=get_kth_child(funcDecTreeNode, 1);
  struct TrieNode* funcTrieNode=find_name(funcRoot, FuncIDTreeNode->detail);
  VarNamePair paramList;
  if(funcTrieNode!=NULL){//duplicate define function
    parse_error(4, FuncIDTreeNode->line, "");
    //delete oringinal function
  }
  else{
    funcTrieNode=insert_name(funcRoot,FuncIDTreeNode->detail);
  }
  funcTrieNode->outPtr.funcInfo.returnType=returnType;
  if(funcDecTreeNode->childNum==3){//ID LP RP
    funcTrieNode->outPtr.funcInfo.paramListHead=NULL;
  }
  else{//ID LP VarList RP
    paramList=param_varList2var_name_list(get_kth_child(funcDecTreeNode, 3));
    /*check duplicate
    struct TrieNode *paramRoot=calloc(1,sizeof(struct TrieNode));
    for(struct VarNameListNode *param=paramList.head;param!=NULL;param=param->next){
      if(find_name(paramRoot,param->name)!=NULL||find_name(structNameRoot, param->name)!=NULL){
        parse_error(3, funcDecTreeNode->line, "");
      }
      else{
        insert_name(paramRoot, param->name);
      }
    }
    delete_trie(paramRoot);*/

    funcTrieNode->outPtr.funcInfo.paramListHead=paramList.head;
  }
  return funcTrieNode;
}

bool is_anonymous(const struct TreeNode *structSpe){
  //return str_equal(get_kth_child(structSpe, 2)->defName,"empty");
  return get_kth_child(structSpe,2)->line==-1;
}

void check_ExtDef(const struct TreeNode* extDefTreeNode){
  assert(str_equal(extDefTreeNode->defName, "ExtDef"));
  struct TreeNode *speTreeNode=get_kth_child(extDefTreeNode, 1);
  struct TypeNode *type=specifier2type(speTreeNode);
  if(str_equal(get_kth_child(extDefTreeNode, 2)->defName,"FunDec")){
    struct TrieNode *funTrieNode=check_FunDec(get_kth_child(extDefTreeNode,2),type);
    struct TreeNode *compTreeNode=get_kth_child(extDefTreeNode, 3);
    insert_var_name_list(funTrieNode->outPtr.funcInfo.paramListHead,compTreeNode->line);
    check_CompSt(compTreeNode, type);
    //delete_var_name_list(funTrieNode->outPtr.funcInfo.paramListHead, compTreeNode->line);
  }
  else if(extDefTreeNode->childNum==2){
    if(type!=NULL&&type->kind==STRUCTURE&&is_anonymous(get_kth_child(speTreeNode, 1))){
      //delete type
    }
  }
  else{
    VarNamePair globalVarList=declist2var_name_list(get_kth_child(extDefTreeNode, 2), type, false, false,0);
      insert_var_name_list(globalVarList.head, 0);
  }
}

void check_ExtDefList(const struct TreeNode *extDefList){
  //if(str_equal(extDefList->defName, "empty"))return;
  if(extDefList->line==-1)return;
  assert(str_equal(extDefList->defName,"ExtDefList"));
  check_ExtDef(get_kth_child(extDefList, 1));
  check_ExtDefList(get_kth_child(extDefList, 2));
}

void check_program(){
  varRoot=calloc(1,sizeof(struct TrieNode));
  structNameRoot=calloc(1,sizeof(struct TrieNode));
  funcRoot=calloc(1,sizeof(struct TrieNode));
  check_ExtDefList(get_kth_child(programParseTree,1));
}