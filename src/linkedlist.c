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


void ll_delete_node(LinkedList* linkedList, LinkedListNode* node) {
	if (node->prev) {
		node->prev->next = node->next;
	}
	if (node->next) {
		node->next->prev = node->prev;
	}
	if (linkedList->head == node) {
		linkedList->head = node->next;
	}
	if (linkedList->tail == node) {
		linkedList->tail = node->prev;
	}
	free(node);
}
