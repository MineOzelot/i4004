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

string *string_append_str(string *str, const char *app, size_t len) {
	if(str->len + len > str->size) {
		size_t new_size = (size_t) (str->size + len);
		str = realloc(str, sizeof(string) + new_size + 1);
		str->size = new_size;
	}
	for(size_t i = 0; i < len; i++) {
		str->data[str->len + i] = app[i];
	}
	str->len += len;
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

string *string_unesaped(string *str) {
	string *ns = calloc(1, sizeof(string) + str->len + 1);

	for(size_t i = 0; i < str->len; i++) {
		if(str->data[i] == '\\') {
			switch(str->data[++i]) {
				case 'a': string_append(ns, '\a'); continue;
				case 'b': string_append(ns, '\b'); continue;
				case 'f': string_append(ns, '\f'); continue;
				case 'n': string_append(ns, '\n'); continue;
				case 'r': string_append(ns, '\r'); continue;
				case 't': string_append(ns, '\t'); continue;
				case 'v': string_append(ns, '\v'); continue;
				case '\\': string_append(ns, '\\'); continue;
				case '\'': string_append(ns, '\''); continue;
				case '"': string_append(ns, '"'); continue;
				case '?': string_append(ns, '?'); continue;
				default:
					string_append(ns, '\\');
					string_append(ns, str->data[i]);
					continue;
			}
		}
		string_append(ns, str->data[i]);
	}

	return ns;
}

void string_destroy(string *str) {
	free(str);
}
