#ifndef H_LINKEDLIST
#define H_LINKEDLIST

#include <stdlib.h>


typedef struct LinkedList {
	struct LinkedListNode* head;
	struct LinkedListNode* tail;
} LinkedList;

typedef struct LinkedListNode {
	void* ptr;
	struct LinkedListNode* next;
	struct LinkedListNode* prev;
} LinkedListNode;


void ll_add_node(LinkedList* linkedList, void* ptr);

#endif
