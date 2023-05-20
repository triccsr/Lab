#include "parse_tree.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

char *strdup(const char *str) {
  char *ret = malloc(strlen(str) + 1);
  if (ret == NULL)
    return NULL;
  strcpy(ret, str);
  return ret;
}
struct TreeNode *new_tree_node(char defName[], int line, int sonNum, ...) {
  struct TreeNode *treeNode = calloc(1,sizeof(struct TreeNode));
  treeNode->defName = strdup(defName);
  treeNode->detail = NULL;
  treeNode->line = line;
  treeNode->sonListHead = NULL;
  treeNode->childNum=sonNum;
  va_list sons;
  va_start(sons, sonNum);
  struct LinkNode *sonListTail = NULL;
  for (int i = 0; i < sonNum; ++i) {
    struct TreeNode *sonNode = va_arg(sons, struct TreeNode *);
    struct LinkNode *newLink = calloc(1,sizeof(struct LinkNode));
    newLink->to = sonNode;
    newLink->next = NULL;
    if (i == 0) {
      treeNode->sonListHead = newLink;
    } else {
      sonListTail->next = newLink;
    }
    sonListTail = newLink;
  }
  va_end(sons);
  return treeNode;
}
struct TreeNode *new_int_node(char defName[], int line,int v) {
  struct TreeNode *typeNode = calloc(1,sizeof(struct TreeNode));
  typeNode->defName = strdup(defName);
  typeNode->value.intVal = v;
  typeNode->detail = NULL;
  typeNode->sonListHead = NULL;
  typeNode->childNum=0;
  typeNode->line=line;
  return typeNode;
}
struct TreeNode *new_float_node(char defName[], int line,float v) {
  struct TreeNode *typeNode = calloc(1,sizeof(struct TreeNode));
  typeNode->defName = strdup(defName);
  typeNode->value.floatVal = v;
  typeNode->detail = NULL;
  typeNode->sonListHead = NULL;
  typeNode->childNum=0;
  typeNode->line=line;
  return typeNode;
}
struct TreeNode *new_relop_node(char defName[], int line, char r[]){
  struct TreeNode *typeNode = calloc(1,sizeof(struct TreeNode));
  typeNode->defName = strdup(defName);
  typeNode->detail = NULL;
  typeNode->sonListHead = NULL;
  typeNode->childNum=0;
  typeNode->line=line;
  if(strcmp(r,">")==0){
    typeNode->value.relopEnum=RELOP_GR; 
  }
  else if(strcmp(r,"<")==0){
    typeNode->value.relopEnum=RELOP_LE;
  }
  else if(strcmp(r,">=")==0){
    typeNode->value.relopEnum=RELOP_GEQ;
  }
  else if(strcmp(r,"<=")==0){
    typeNode->value.relopEnum=RELOP_LEQ;
  }
  else if(strcmp(r,"==")==0){
    typeNode->value.relopEnum=RELOP_EQ;
  }
  else if(strcmp(r,"!=")==0){
    typeNode->value.relopEnum=RELOP_NEQ;
  }
  else{
    assert(0);
  }
  return typeNode;
}
struct TreeNode *new_token_node(char defName[], int line,int hasDetail, char detail[]) {
  struct TreeNode *typeNode = calloc(1,sizeof(struct TreeNode));
  typeNode->defName = strdup(defName);
  typeNode->detail = hasDetail ? strdup(detail) : NULL;
  typeNode->sonListHead = NULL;
  typeNode->childNum=0;
  typeNode->line=line;
  return typeNode;
}

struct TreeNode* get_kth_child(const struct TreeNode* now,int k){//from 1
  struct LinkNode* link=now->sonListHead;
  if(k>now->childNum){
    return NULL;
  }
  while(--k){
    assert(link!=NULL);
    link=link->next;
  }
  return link->to;
}

bool childrens_are(const struct TreeNode *now,const char s[]){
  struct LinkNode *link=now->sonListHead;
  char tmp[16];
  int ptrS=0,ptrTmp;
  while(s[ptrS]==' ')++ptrS;
  for(int i=0;i<now->childNum;++i){
    ptrTmp=0;
    while(s[ptrS]!=' '&&s[ptrS]!='\0'){
      tmp[ptrTmp++]=s[ptrS];
      ++ptrS;
    }
    tmp[ptrTmp]='\0';
    if(strcmp(link->to->defName,tmp)!=0){
      return false;
    }
    link=link->next;
    while(s[ptrS]==' ')++ptrS;
  }
  return s[ptrS]=='\0';
}

void print_tree_node(struct TreeNode *treeNode) {
  printf("%s", treeNode->defName);
  if (treeNode->sonListHead != NULL) {
    printf(" (%d)", treeNode->line);
  } else if (treeNode->detail != NULL) {
    printf(": %s", treeNode->detail);
  } else if (strcmp(treeNode->defName, "INT") == 0) {
    printf(": %d", treeNode->value.intVal);
  } else if (strcmp(treeNode->defName, "FLOAT") == 0) {
    printf(": %.6f", treeNode->value.floatVal);
  }
  // printf(" %p",treeNode);
  putchar('\n');
}
void print_space(int n) {
  for (int i = 0; i < n; ++i)
    putchar(' ');
}
void print_tree_helper(struct TreeNode *treeNode, int depth) {
  if (strcmp(treeNode->detail, "empty") == 0 && treeNode->line==-1) {
    return;
  }
  print_space(depth * 2);
  print_tree_node(treeNode);
  for (struct LinkNode *ptr = treeNode->sonListHead; ptr != NULL;
       ptr = ptr->next) {
    print_tree_helper(ptr->to, depth + 1);
  }
}
void print_tree(struct TreeNode *treeNode) { print_tree_helper(treeNode, 0); }