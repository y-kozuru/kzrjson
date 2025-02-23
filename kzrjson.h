#ifndef KZRJSON_H
#define KZRJSON_H
#include <stdbool.h>

struct kzrjson_data_inner;
typedef struct {
	struct kzrjson_data_inner *inner;
} kzrjson_t;

typedef struct {
	const char **keys;
	size_t size;
} kzrjson_object_keys;

void kzrjson_print(kzrjson_t data);
void kzrjson_free(kzrjson_t data);
kzrjson_t kzrjson_parse(const char *json_text);

bool kzrjson_is_object(kzrjson_t data);
bool kzrjson_is_array(kzrjson_t data);
bool kzrjson_is_string(kzrjson_t data);
bool kzrjson_is_number(kzrjson_t data);
bool kzrjson_is_member(kzrjson_t data);
bool kzrjson_is_boolean(kzrjson_t data);
bool kzrjson_is_null(kzrjson_t data);

bool kzrjson_error_occured(kzrjson_t result);

size_t kzrjson_object_size(kzrjson_t object);
kzrjson_t kzrjson_get_member(kzrjson_t object, const char *key);
const char *kzrjson_get_member_key(kzrjson_t member);
kzrjson_t kzrjson_get_value_from_member(kzrjson_t member);
kzrjson_t kzrjson_get_value_from_key(kzrjson_t object, const char *key);

size_t kzrjson_array_size(kzrjson_t array);
kzrjson_t kzrjson_get_element(kzrjson_t array, size_t index);


const char *kzrjson_get_string(kzrjson_t string);
bool kzrjson_get_boolean(kzrjson_t boolean);
const char *kzrjson_get_number(kzrjson_t number);

#endif // KZRJSON_H
