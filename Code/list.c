#include <stddef.h>
#include <stdlib.h>
#include "list.h"
struct List new_list(){
  return (struct List){NULL,NULL,0};
}
void list_append(struct List *list,void* infoPtr){
  list->size+=1;
  struct ListNode* newNode=calloc(1,sizeof(struct ListNode));
  newNode->infoPtr=infoPtr;
  newNode->next=NULL;
  newNode->prev=list->listTail;
  if(list->listTail==NULL){
    list->listHead=list->listTail=newNode;
  }
  else{
    list->listTail->next=newNode;
  }
}