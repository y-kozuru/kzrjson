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
typedef enum {
	kzrjson_object,
	kzrjson_array,
	kzrjson_string,
	kzrjson_number,
	kzrjson_bool,
	kzrjson_null,
	kzrjson_member,
} kzrjson_type;

typedef enum {
	kzrjson_int,
	kzrjson_uint,
	kzrjson_double,
	kzrjson_exp,
} kzrjson_number_type;

 typedef struct kzrjson_t *kzrjson_t;
struct kzrjson_t {
	kzrjson_type type;

	// elements of array or object
	kzrjson_t *elements;
	size_t elements_size;

	// key, value of member
	char *key;
	kzrjson_t value;

	// string presentation for string, number, boolean, null
	char *string;

	// boolean
	bool boolean;

	// number
	kzrjson_number_type number_type;
	union {
		int64_t number_int;
		uint64_t number_uint;
		double number_double;
	};
};

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
 * Get a value of the member from object by member.
 *
 * [error] kzrjson_exception_illegal_type
 * [error] kzrjson_exception_object_key_not_found
 */
kzrjson_t kzrjson_get_value_from_key(kzrjson_t object, const char *key);

/*
 * Get an element of the array by index.
 *
 * [error] kzrjson_exception_illegal_type
 * [error] kzrjson_exception_array_index_out_of_range
 */
kzrjson_t kzrjson_get_element(kzrjson_t array, size_t index);

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
bool kzrjson_object_add_member(kzrjson_t object, kzrjson_t member);

/*
 * Add the element to the array.
 *
 * [error] kzrjson_exception_illegal_type
 */
bool kzrjson_array_add_element(kzrjson_t array, kzrjson_t element);

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
kzrjson_t kzrjson_make_number_uint(const uint64_t number);

 /*
  * [error] kzrjson_exception_not_number
  * [error] kzrjson_exception_failed_to_allocate_memory
  */
kzrjson_t kzrjson_make_number_int(const int64_t number);

/*
 * Make number from exponential anotation number as string.
 *
 * [error] kzrjson_exception_not_number
 * [error] kzrjson_exception_failed_to_allocate_memory
 */
kzrjson_t kzrjson_make_number_exp(const char *exp, const size_t length);

 /*
  * [error] kzrjson_exception_failed_to_allocate_memory
  */
kzrjson_text_t kzrjson_to_string(kzrjson_t data);

#endif // KZRJSON_H
