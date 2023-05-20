#ifndef PARSE_TREE
#define PARSE_TREE 0;
#include <string.h>
#include <stdbool.h>

enum RelopEnum{
  RELOP_GR=0,
  RELOP_LE,
  RELOP_GEQ,
  RELOP_LEQ,
  RELOP_EQ,
  RELOP_NEQ
};
union Value {
  int intVal;
  float floatVal;
  enum RelopEnum relopEnum;
};
struct TreeNode {
  char *defName;
  char *detail;
  int line;
  int childNum;
  union Value value;
  struct LinkNode *sonListHead;
};
struct LinkNode {
  struct TreeNode *to;
  struct LinkNode *next;
};
char* strdup(const char *str);
struct TreeNode *new_token_node(char defName[], int line,int hasDetail, char detail[]);
struct TreeNode *new_tree_node(char defName[], int line, int sonNum, ...);
struct TreeNode *new_int_node(char defName[], int line,int v);
struct TreeNode *new_float_node(char defName[], int line,float v);
struct TreeNode *new_relop_node(char defName[], int line, char r[]);
struct TreeNode *get_kth_child(const struct TreeNode *now,int k);
bool childrens_are(const struct TreeNode *now,const char s[]);
void print_tree(struct TreeNode *treeNode);
extern struct TreeNode *programParseTree;
#endif