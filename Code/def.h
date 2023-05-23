#ifndef error_def_H
#define error_def_H

#include "parse_tree.h"
#include <stddef.h>

struct VarNameListNode;
struct TypeNode {
  enum { BASIC, ARRAY, STRUCTURE } kind;
  union {
    int basic; // int or float
    struct {
      struct TypeNode *elem;
      int size;
    } array;
    struct VarNameListNode *structure;
  } u;
  unsigned sizeOf;
};
struct VarNameListNode {
  char *name;
  struct TypeNode *type;
  int line;
  struct VarNameListNode *next;
};

/*struct ParamListNode{
  char* name;
  struct TypeNode* type;
  struct ParamListNode* nextParam;
};*/
struct IROpr {
  enum { IROPR_DEFVAR = 0, IROPR_TMPVAR, IROPR_NUM, IROPR_LABEL,IROPR_FUNC } oprType;
  int val;
  bool addrOf;
};

struct FuncNode {
  struct TypeNode *returnType;
  struct IROpr irOpr;
  struct VarNameListNode *paramListHead;
};

struct IRPos {
  enum { IRPOS_VAR = 0, IRPOS_ADDR } type; // itself or its address
  struct IROpr irOpr;                      // store in irOpr
};

struct VarDefNode {
  int envLine;
  int line;
  struct IRPos irPos;
  struct TypeNode *type;
  struct VarDefNode *next;
};

struct AnonymousStructListNode {
  struct TypeNode *type;
  struct AnonymousStructListNode *next;
};

union OutPtr {
  struct TypeNode *type;
  struct FuncNode funcInfo;
  struct VarDefNode *varDefHead;
};

enum IRType {
  IRTYPE_LABEL = 0,
  IRTYPE_FUNC,
  IRTYPE_ASSIGN,
  IRTYPE_PLUS,
  IRTYPE_MINUS,
  IRTYPE_MUL,
  IRTYPE_DIV,
  //IRTYPE_VA,
  IRTYPE_VL,
  IRTYPE_LV,
  IRTYPE_GOTO,
  IRTYPE_IFGOTO,
  IRTYPE_RETURN,
  IRTYPE_DEC,
  IRTYPE_ARG,
  IRTYPE_CALL,
  IRTYPE_PARAM,
  IRTYPE_READ,
  IRTYPE_WRITE
};

struct IRNode {
  enum IRType irType;
  struct IROpr x, y, z;
  enum RelopEnum relop;
  struct IRNode *prv;
  struct IRNode *nxt;
};

struct IRListPair {
  struct IRNode *head;
  struct IRNode *tail;
};

struct LookupType{
  struct IRPos irPos;
  struct TypeNode *type;
  struct IRListPair irList;
};

struct IROprListNode{
  struct IROpr irOpr;
  struct IROprListNode* next;
};

struct ArgsHelperType{
  struct IRListPair irList;
  struct IROprListNode *argListHead;
  struct IROprListNode *argListTail;
};

struct IROpr new_num(int v);
struct IROpr new_def_var();
struct IROpr new_tmp_var();
struct IROpr new_label();
struct IROpr new_func();
struct IRListPair new_single_IRList(enum IRType irType,int argc,...);
#endif