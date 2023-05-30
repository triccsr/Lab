#include "ID_trie.h"
#include "def.h"
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

//bool can_assign(struct VarListNode *a, struct VarListNode *b) {
  //if (a->typeName != b->typeName)return false;
  //if (a->typeName==FUNC)return false;
  //if(a->typeName==STRUCT&&a->treeDefNode!=b->treeDefNode)return false;
  //return true;
//}
int child_index(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'Z')
    return c-'A'+10;
  if (c >= 'a' && c <= 'z')
    return c-'a'+36;
  return 62;
}


struct TrieNode *
insert_name(struct TrieNode *trieRoot,
            const char s[]) { // insert a word to trie rooted trieRoot, return
                              // the last trieNode
  assert(trieRoot!=NULL);
  struct TrieNode *now = trieRoot;
  int strLen = (int)strlen(s);
  for (int i = 0, childIndex; i < strLen; ++i) {
    childIndex = child_index(s[i]);
    if (now->child[childIndex] == NULL) {
      now->child[childIndex] = calloc(1,sizeof(struct TrieNode));
    }
    now = now->child[childIndex];
  }
  now->isEnd=true;
  return now;
}
struct TrieNode *find_name(struct TrieNode *trieRoot, const char s[]) {
  if(trieRoot==NULL)return  NULL;
  struct TrieNode *now = trieRoot;
  int len = (int)strlen(s);
  for (int i = 0, childIndex; i < len; ++i) {
    childIndex = child_index(s[i]);
    if (now->child[childIndex] == NULL) {
      return NULL;
    }
    now = now->child[childIndex];
  }
  if(now->isEnd==true)return now;
  return NULL;
}

void delete_trie(struct TrieNode *trieRoot){
  if(trieRoot==NULL)return;
  for(int i=0;i<63;++i){
    if(trieRoot->child[i]!=NULL){
      delete_trie(trieRoot->child[i]);
      trieRoot->child[i]=NULL;
    }
  }
  free(trieRoot);
}

/*void insert_var(struct TrieNode *trieNode, struct VarListNode *var) {
  var->next = trieNode->info.varListHead;
  trieNode->info.varListHead = var;
}
void remove_first_var(struct TrieNode *trieNode) {
  if (trieNode->info.varListHead != NULL) {
    struct VarListNode *firstVar = trieNode->info.varListHead;
    trieNode->info.varListHead = trieNode->info.varListHead->next;
    free(firstVar);
  }
}*/