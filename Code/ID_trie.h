#ifndef ID_TRIE_H
#define ID_TRIE_H
#include <string.h>
#include "error_def.h"
#include "parse_tree.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct TrieNode;

struct TrieNode {
  struct TrieNode *child[63];
  /*union{
    struct VarListNode* varListHead;
    struct TreeNode* structDef;
  }info;*/
  bool isEnd;
  union OutPtr outPtr;
};
//bool can_assign(struct VarListNode *a,struct VarListNode *b);

int child_index(char c);
struct TrieNode *insert_name(struct TrieNode* trieRoot,const char s[]);
struct TrieNode *find_name(struct TrieNode* trieRoot,const char s[]);
void delete_trie(struct TrieNode* trieRoot);
//void insert_var(struct TrieNode* trieNode,struct VarListNode* var);//insert a var to a trieNode's varList
//void remove_first_var(struct TrieNode* trieNode);

#endif//ID_TRIE_H