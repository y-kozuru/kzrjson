#ifndef KZRJSON_H
#define KZRJSON_H
#include <stdbool.h>
#include <stdint.h>

/*****************************************************************************
 * Exception
 *****************************************************************************/
/*
 * When erro occurred, kzrjson_ functions throw exception.
 * You can catch and handle exception.
 *
 * example)
 *    kzrjson_t json = kzrjson_parse(json_text);
 *    if (kzrjson_catch_exception()) {
 *       switch (kzrjson_exception_name()) {
 *       case kzrjson_exception_tokenize:
 *           // exception handling
 *           break;
 *       case kzrjson_exception_parse:
 *           // exception handling
 *           break;
 *       case kzrjson_exception_failed_to_allocate_memory:
 *           // exception handling
 *           break;
 *       }
 *    }
 */
typedef enum {
	kzrjson_success = 0,

	// Failed to tokenize input JSON text
	kzrjson_exception_tokenize,

	// Failed to parse input JSON text
	kzrjson_exception_parse,

	// Failed to allocate memory for kzrjson_t
	kzrjson_exception_failed_to_allocate_memory,

	// Failed to parse number because input number token is not parseable number
	kzrjson_exception_not_number,

	// Input kzrjson_t is illegal type
	kzrjson_exception_illegal_type,

	// The index is out of range of the array.
	kzrjson_exception_array_index_out_of_range,

	// There is no member with the specified key in the object.
	kzrjson_exception_object_key_not_found,
} kzrjson_exception_name;

/*
 * Return whether or not an exception was caught.
 */
bool kzrjson_catch_exception(void);
kzrjson_exception_name kzrjson_exception(void);


/*****************************************************************************
 * Data types
 *****************************************************************************/
 /*
 * kzrjson_t is common type of this library.
 * All of JSON types are represented by this type.
 * You can access Internal data using various functions in the library.
 * inner is allocated to heap memory.
 */
struct kzrjson_inner;
typedef struct {
	struct kzrjson_inner *inner;
} kzrjson_t;

/*
 * String representation of kzrjson_t.
 * text is allocated to heap memory.
 */
typedef struct {
	char *text;
	size_t length;
} kzrjson_text_t;

/*
 * Free kzrjson_t.
 * All memory in the kzrjson_t given as an argument is released,
 * so data retrieved from that kzrjson_t (object members, array elements, etc.) are also released.
 * 
 * [no error]
 */
void kzrjson_free(kzrjson_t any);


/*****************************************************************************
 * Parse JSON
 *****************************************************************************/
/*
 * Parse JSON text and construct kzrjson_t.
 * All information required as JSON data is copied at parse time,
 * so this library do not use json_text after parse.
 * 
 * [error] kzrjson_exception_tokenize
 * [error] kzrjson_exception_parse
 * [error] kzrjson_exception_failed_to_allocate_memory
 */
kzrjson_t kzrjson_parse(const char *json_text);

/*****************************************************************************
 * Print JSON
 *****************************************************************************/
/*
 * Print kzrjson_t to stdout with indent.
 * 
 * [no error]
 */
void kzrjson_print(kzrjson_t any);


/*****************************************************************************
 * Operations of kzrjson_t
 *****************************************************************************/
/*
 * Check type of kzrjson_t.
 *
 * [error] kzrjson_exception_illegal_type
 */
bool kzrjson_is_object(kzrjson_t any);
bool kzrjson_is_array(kzrjson_t any);
bool kzrjson_is_string(kzrjson_t any);
bool kzrjson_is_number(kzrjson_t any);
bool kzrjson_is_member(kzrjson_t any);
bool kzrjson_is_boolean(kzrjson_t any);
bool kzrjson_is_null(kzrjson_t any);

/*
 * Get the number of members of the object.
 * 
 * [error] kzrjson_exception_illegal_type
 */
size_t kzrjson_object_size(kzrjson_t object);

/*
 * Get a member from the object by key.
 *
 * [error] kzrjson_exception_illegal_type
 * [error] kzrjson_exception_object_key_not_found
 */
kzrjson_t kzrjson_get_member(kzrjson_t object, const char *key);

/*
 * Get a key of the member.
 *
 * [error] kzrjson_exception_illegal_type
 */
const char *kzrjson_get_member_key(kzrjson_t member);

/*
 * Get a value of the member from member.
 *
 * [error] kzrjson_exception_illegal_type
 */
kzrjson_t kzrjson_get_value_from_member(kzrjson_t member);

/*
 * Get a value of the member from object by member.
 *
 * [error] kzrjson_exception_illegal_type
 * [error] kzrjson_exception_object_key_not_found
 */
kzrjson_t kzrjson_get_value_from_key(kzrjson_t object, const char *key);

/*
 * Get the number of elements of the array.
 *
 * [error] kzrjson_exception_illegal_type
 */
size_t kzrjson_array_size(kzrjson_t array);

/*
 * Get an element of the array by index.
 *
 * [error] kzrjson_exception_illegal_type
 * [error] kzrjson_exception_array_index_out_of_range
 */
kzrjson_t kzrjson_get_element(kzrjson_t array, size_t index);

/*
 * Get string as const char * from kzrjson_t.
 *
 * [error] kzrjson_exception_illegal_type
 */
const char *kzrjson_get_string(kzrjson_t string);

/*
 * Get boolean as bool from kzrjson_t.
 *
 * [error] kzrjson_exception_illegal_type
 */
bool kzrjson_get_boolean(kzrjson_t boolean);

/*
 * Get number as const char * from kzrjson_t.
 *
 * [error] kzrjson_exception_illegal_type
 */
const char *kzrjson_get_number_as_string(kzrjson_t number);

/*
 * Get number as integer from kzrjson_t.
 *
 * [error] kzrjson_exception_illegal_type
 */
int64_t kzrjson_get_number_as_integer(kzrjson_t number);

/*
 * Get number as unsinged integer from kzrjson_t.
 *
 * [error] kzrjson_exception_illegal_type
 */
uint64_t kzrjson_get_number_as_unsigned_integer(kzrjson_t number);

/*
 * Get number as double from kzrjson_t.
 *
 * [error] kzrjson_exception_illegal_type
 */
double kzrjson_get_number_as_double(kzrjson_t number);

/*****************************************************************************
 * Make JSON
 ****************************************************************************/
/*
 * [error] kzrjson_exception_failed_to_allocate_memory
 */
kzrjson_t kzrjson_make_object(void);

/*
 * [error] kzrjson_exception_failed_to_allocate_memory
 */
kzrjson_t kzrjson_make_array(void);

/*
 * Add the member to the object.
 *
 * [error] kzrjson_exception_illegal_type
 */
void kzrjson_object_add_member(kzrjson_t *object, kzrjson_t member);

/*
 * Add the element to the array.
 *
 * [error] kzrjson_exception_illegal_type
 */
void kzrjson_array_add_element(kzrjson_t *array, kzrjson_t element);

/*
 * Make member from key and value.
 *
 * [error] kzrjson_exception_failed_to_allocate_memory
 */
kzrjson_t kzrjson_make_member(const char *key, const size_t key_length, kzrjson_t value);

/*
 * [error] kzrjson_exception_failed_to_allocate_memory
 */
kzrjson_t kzrjson_make_string(const char *string, const size_t length);

/*
 * [error] kzrjson_exception_failed_to_allocate_memory
 */
kzrjson_t kzrjson_make_boolean(const bool boolean);

/*
 * [error] kzrjson_exception_failed_to_allocate_memory
 */
kzrjson_t kzrjson_make_null();

/*
 * [error] kzrjson_exception_not_number
 * [error] kzrjson_exception_failed_to_allocate_memory
 */
kzrjson_t kzrjson_make_number_double(const double number);

 /*
  * [error] kzrjson_exception_not_number
  * [error] kzrjson_exception_failed_to_allocate_memory
  */
kzrjson_t kzrjson_make_number_unsigned_integer(const uint64_t number);

 /*
  * [error] kzrjson_exception_not_number
  * [error] kzrjson_exception_failed_to_allocate_memory
  */
kzrjson_t kzrjson_make_number_integer(const int64_t number);

 /*
  * [error] kzrjson_exception_failed_to_allocate_memory
  */
kzrjson_text_t kzrjson_to_string(kzrjson_t data);

#endif // KZRJSON_H
