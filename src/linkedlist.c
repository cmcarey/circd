#include "linkedlist.h"

LinkedListNode* ll_add_node(LinkedList* linkedList, void* ptr) {
	LinkedListNode* node = calloc(1, sizeof(LinkedListNode));
	node->ptr = ptr;

	if(linkedList->tail) {
		linkedList->tail->next = node;
		node->prev = linkedList->tail;
		linkedList->tail = node;
	} else {
		linkedList->head = node;
		linkedList->tail = node;
	}

	return node;
}
