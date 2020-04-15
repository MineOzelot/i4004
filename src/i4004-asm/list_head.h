#ifndef I4004_LIST_HEAD_H
#define I4004_LIST_HEAD_H

#include <stddef.h>

typedef struct list_head list_head;
struct list_head {
	list_head *next;
};

void list_head_destroy(void *list);
size_t list_head_size(void *list);

#endif //I4004_LIST_HEAD_H
