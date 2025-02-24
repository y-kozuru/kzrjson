#ifndef KZRJSON_H
#define KZRJSON_H
#include <stdbool.h>
#include <stdint.h>

struct kzrjson_inner;
typedef struct {
	struct kzrjson_inner *inner;
} kzrjson_t;

typedef struct {
	char *text;
	size_t length;
} kzrjson_text_t;

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

const char *kzrjson_get_number_as_string(kzrjson_t number);
int64_t kzrjson_get_number_as_integer(kzrjson_t number);
uint64_t kzrjson_get_number_as_unsigned_integer(kzrjson_t number);
double kzrjson_get_number_as_double(kzrjson_t number);

kzrjson_t kzrjson_make_object(void);
kzrjson_t kzrjson_make_array(void);
void kzrjson_object_add_member(kzrjson_t *object, kzrjson_t member);
void kzrjson_array_add_element(kzrjson_t *array, kzrjson_t element);

kzrjson_t kzrjson_make_member(const char *key, const size_t length, kzrjson_t value);
kzrjson_t kzrjson_make_string(const char *string, const size_t length);
kzrjson_t kzrjson_make_boolean(const bool boolean);
kzrjson_t kzrjson_make_null();

kzrjson_t kzrjson_make_number_double(const double number);
kzrjson_t kzrjson_make_number_unsigned_integer(const uint64_t number);
kzrjson_t kzrjson_make_number_integer(const int64_t number);

kzrjson_text_t kzrjson_to_string(kzrjson_t data);

#endif // KZRJSON_H
