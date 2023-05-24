#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stddef.h>
#include "def.h"
#include "ir.h"
#include "ID_trie.h"
#include "parse_tree.h"

struct TrieNode *lab5VarTrieRoot,*lab5LabelTrieRoot,*lab5FunctionTrieRoot;
int lab5VarCount=0,lab5LabelCount=0,lab5FunctionCount=0,lab5TmpCount;
bool str_equal(const char s1[],const char s2[]);

struct IROpr lab5_new_var(){
  return (struct IROpr){IROPR_DEFVAR,lab5VarCount++,false};
}
struct IROpr lab5_new_tmp(){
  return (struct IROpr){IROPR_TMPVAR,lab5TmpCount++,false};
}
struct IROpr lab5_new_label(){
  return (struct IROpr){IROPR_LABEL,lab5LabelCount++,false};
}
struct IROpr lab5_new_function(){
  return (struct IROpr){IROPR_FUNC,lab5FunctionCount++,false};
}
struct IROpr lab5_new_num(int v){
  return (struct IROpr){IROPR_NUM,v,false};
}

int stoi(const char *s){
  int ret=0;
  for(int i=0;s[i]!='\0';++i){
    ret=ret*10+s[i]-'0';
  }
  return ret;
}

struct IROpr lab5_get_var_IROpr(const char *token){
  if(token[0]=='#'){
    return lab5_new_num(stoi(token+1)); 
  }
  int offset=(token[0]=='&')?1:0;
  struct TrieNode * varName=find_name(lab5VarTrieRoot,token+offset);
  if(varName==NULL){
    varName=insert_name(lab5VarTrieRoot,token+offset);
    if(token[offset]=='t'){
      varName->outPtr.lab5IROpr=lab5_new_tmp();
    }
    else{
      varName->outPtr.lab5IROpr=lab5_new_var();
    }
  }
  struct IROpr ret=varName->outPtr.lab5IROpr;
  ret.addrOf=(bool)offset;
  return ret;
}


struct IRListPair file_to_IRList(const char fileName[]) {
  lab5VarTrieRoot=calloc(1,sizeof(struct TrieNode));
  lab5FunctionTrieRoot=calloc(1,sizeof(struct TrieNode));
  lab5LabelTrieRoot=calloc(1,sizeof(struct TrieNode));
  lab5FunctionCount=1;
  freopen(fileName, "r", stdin);
  struct IRListPair ret=(struct IRListPair){NULL,NULL};
  while(1){
    char inputLine[512];
    bool ok=fgets(inputLine,512,stdin);
    if(!ok)break;
    size_t charNum=strlen(inputLine);
    char token[6][32];
    int tokenIndex=0,tail=0;
    for(int i=0;i<charNum;++i){
      if((inputLine[i]==' '||inputLine[i]=='\n')&&tail!=0){
        token[tokenIndex][tail]='\0';
        ++tokenIndex;
        tail=0;
      }
      else if(inputLine[i]!=' '&&inputLine[i]!='\n'){
        token[tokenIndex][tail++]=inputLine[i];
      }
    }
    if(tail!=0){
      token[tokenIndex][tail]='\0';
      tail=0;
      ++tokenIndex;
    }
    if(str_equal(token[0],"LABEL")){// LABEL l
      struct TrieNode *label=find_name(lab5LabelTrieRoot,token[1]);
      if(label==NULL){
        label=insert_name(lab5LabelTrieRoot,token[1]);
        label->outPtr.lab5IROpr=lab5_new_label();
      }
      ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_LABEL,1,label->outPtr.lab5IROpr));
    }
    else if(str_equal(token[0],"FUNCTION")){// FUNCTION f
      struct TrieNode *funcTrieNode=insert_name(lab5FunctionTrieRoot,token[1]);
      if(str_equal(token[1],"main")){
        funcTrieNode->outPtr.lab5IROpr=(struct IROpr){IROPR_FUNC,0,false};  
      }
      else{
        funcTrieNode->outPtr.lab5IROpr=lab5_new_function();
      }
      ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_FUNC,1,funcTrieNode->outPtr.lab5IROpr));
    }
    else if (str_equal(token[0],"GOTO")){// GOTO l
      struct TrieNode *gotoLabel=find_name(lab5LabelTrieRoot,token[1]);
      if(gotoLabel==NULL){
        gotoLabel=insert_name(lab5LabelTrieRoot,token[1]);
        gotoLabel->outPtr.lab5IROpr=lab5_new_label();
      }
      ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_GOTO,1,gotoLabel->outPtr.lab5IROpr));
    }
    else if(str_equal(token[0],"IF")){ //IF x [relop] y GOTO l
      // struct IROpr x=find_name(lab5VarTrieRoot,token[1])->outPtr.lab5IROpr,y=find_name(lab5VarTrieRoot,token[3])->outPtr.lab5IROpr;
      struct IROpr x=lab5_get_var_IROpr(token[1]),y=lab5_get_var_IROpr(token[3]);

      enum RelopEnum relop;
      if(str_equal(token[2],"==")) relop=RELOP_EQ;
      else if(str_equal(token[2],">="))relop=RELOP_GEQ;
      else if(str_equal(token[2],">"))relop=RELOP_GR;
      else if(str_equal(token[2],"<"))relop=RELOP_LE;
      else if(str_equal(token[2],"<="))relop=RELOP_LEQ;
      else if(str_equal(token[2],"!="))relop=RELOP_NEQ;
      else assert(0);
      
      struct TrieNode *labelName=find_name(lab5LabelTrieRoot,token[5]);
      if(labelName==NULL){
        labelName=insert_name(lab5LabelTrieRoot,token[5]);
        labelName->outPtr.lab5IROpr=lab5_new_label();
      }
      struct IROpr gotoLabel=labelName->outPtr.lab5IROpr;
      ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_IFGOTO,4,x,relop,y,gotoLabel));
    }
    else if(str_equal(token[0],"RETURN")){ //RETURN x
      // struct IROpr x=find_name(lab5VarTrieRoot,token[1])->outPtr.lab5IROpr;
      struct IROpr x=lab5_get_var_IROpr(token[1]);
      ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_RETURN,1,x));
    }
    else if(str_equal(token[0],"DEC")){ //DEC x [size]
      // struct IROpr x=find_name(lab5VarTrieRoot,token[1])->outPtr.lab5IROpr;
      struct IROpr x=lab5_get_var_IROpr(token[1]);
      struct IROpr size=new_num(stoi(token[2]));
      ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_DEC,2,x,size));
    }
    else if(str_equal(token[0],"ARG")){ //ARG x
      struct IROpr x=lab5_get_var_IROpr(token[1]);
      ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_ARG,1,x));
    }
    else if(str_equal(token[2],"CALL")){// x := CALL f
      struct IROpr x=lab5_get_var_IROpr(token[0]);
      struct IROpr f=find_name(lab5FunctionTrieRoot,token[3])->outPtr.lab5IROpr;
      ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_CALL,2,x,f));
    }
    else if(str_equal(token[0],"PARAM")){// PARAM x
      // struct IROpr x=find_name(lab5VarTrieRoot,token[1])->outPtr.lab5IROpr;
      struct IROpr x=lab5_get_var_IROpr(token[1]);
      ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_PARAM,1,x));
    }
    else if(str_equal(token[0],"READ")){//READ x
      // struct IROpr x=find_name(lab5VarTrieRoot,token[1])->outPtr.lab5IROpr;
      struct IROpr x=lab5_get_var_IROpr(token[1]);
      ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_READ,1,x));
    }
    else if(str_equal(token[0],"WRITE")){//WRITE x
      // struct IROpr x=find_name(lab5VarTrieRoot,token[1])->outPtr.lab5IROpr;
      struct IROpr x=lab5_get_var_IROpr(token[1]);
      ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_WRITE,1,x));
    }
    else if(tokenIndex==3){
      if(token[0][0]=='*'){// *x := y
        ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_LV,2,lab5_get_var_IROpr(token[0]+1),lab5_get_var_IROpr(token[2])));
      }
      else if(token[2][0]=='*'){ // x := *y
        ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_VL,2,lab5_get_var_IROpr(token[0]),lab5_get_var_IROpr(token[2]+1)));
      }
      else{// x := y
        ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_ASSIGN,2,lab5_get_var_IROpr(token[0]),lab5_get_var_IROpr(token[2])));
      }
    }
    else if(tokenIndex==5){
      if(str_equal(token[3],"+")){ //x := y + z
        ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_PLUS,3,lab5_get_var_IROpr(token[0]),lab5_get_var_IROpr(token[2]),lab5_get_var_IROpr(token[4])));
      }
      else if(str_equal(token[3],"-")){ //x := y - z
        ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_MINUS,3,lab5_get_var_IROpr(token[0]),lab5_get_var_IROpr(token[2]),lab5_get_var_IROpr(token[4])));
      }
      else if(str_equal(token[3],"*")){ //x := y * z
        ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_MUL,3,lab5_get_var_IROpr(token[0]),lab5_get_var_IROpr(token[2]),lab5_get_var_IROpr(token[4])));
      }
      else if(str_equal(token[3],"/")){ //x := y / z
        ret=cat_IRList(2,ret,new_single_IRList(IRTYPE_DIV,3,lab5_get_var_IROpr(token[0]),lab5_get_var_IROpr(token[2]),lab5_get_var_IROpr(token[4])));
      }
      else assert(0);
    }
    else assert(0);
  }
  fclose(stdin);
  return ret;
}