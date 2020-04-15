#include <stdlib.h>
#include "list_head.h"

void list_head_destroy(void *list) {
	list_head *head = list;
	while(head) {
		list_head *l = head;
		head = head->next;
		free(l);
	}
}

size_t list_head_size(void *list) {
	list_head *head = list;
	size_t size = 0;
	while(head) {
		size++;
		head = head->next;
	}
	return size;
}
