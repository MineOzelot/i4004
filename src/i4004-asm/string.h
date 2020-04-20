#ifndef I4004_STRING_H
#define I4004_STRING_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
	size_t size;
	size_t len;
	char data[0];
} string;

string *string_empty();

string *string_from(const char *str, size_t len);

string *string_append(string *str, char ch);
string *string_append_str(string *str, const char *app, size_t len);

bool string_equal(string *str1, string *str2);

string *string_clone(string *str);

void string_destroy(string *str);

#endif //I4004_STRING_H
