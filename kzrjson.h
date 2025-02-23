#ifndef KZRJSON_H
#define KZRJSON_H
#include <stdbool.h>

struct kzrjson_data_inner;
typedef struct {
	struct kzrjson_data_inner *inner;
} kzrjson_data;

void kzrjson_print(kzrjson_data data);
void kzrjson_free(kzrjson_data data);
kzrjson_data kzrjson_parse(const char *json_text);

#endif // KZRJSON_H
