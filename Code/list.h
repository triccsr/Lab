#ifndef LIST_H
#define LIST_H 0
#include <stddef.h>
struct ListNode{
  void* infoPtr;
  struct ListNode* next;
  struct ListNode* prev;
};
struct List{
  struct ListNode* listHead;
  struct ListNode* listTail;
  size_t size;
};
struct List new_list();
void list_append(struct List *list,void* infoPtr);

#endif