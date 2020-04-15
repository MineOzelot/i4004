#include "string.h"

#define STRING_DEF_SIZE 16

string *string_empty() {
	string *str = calloc(1, sizeof(string) + STRING_DEF_SIZE + 1);

	return str;
}

string *string_from(const char *str, size_t len) {
	string *new_str = calloc(1, sizeof(string) + len + 1);

	new_str->len = len;
	new_str->size = len;
	for(size_t i = 0; i < len; i++) new_str->data[i] = str[i];

	return new_str;
}

string *string_append(string *str, char ch) {
	if(str->len >= str->size) {
		size_t new_size = (size_t) (str->size * 1.8 + 1);
		str = realloc(str, sizeof(string) + new_size + 1);
		str->size = new_size;
	}

	str->data[str->len++] = ch;

	return str;
}

bool string_equal(string *str1, string *str2) {
	if(str1->len != str2->len) return false;
	for(size_t i = 0; i < str1->len; i++) {
		if(str1->data[i] != str2->data[i]) return false;
	}
	return true;
}

string *string_clone(string *str) {
	string *clone = calloc(1, sizeof(string) + str->len + 1);

	clone->len = clone->size = str->len;
	for(size_t i = 0; i < str->len; i++) {
		clone->data[i] = str->data[i];
	}

	return clone;
}

void string_destroy(string *str) {
	free(str);
}
