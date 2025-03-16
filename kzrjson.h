#ifndef KZRJSON_H
#define KZRJSON_H
#include <stdbool.h>
#include <stdint.h>

/*****************************************************************************
 * error
 *****************************************************************************/
/*
 * When error occurred, kzrjson functions set kzrjson_errno.
 *
 * example)
 *    kzrjson_t json = kzrjson_parse(json_text);
 *    if (json == NULL && kzrjson_errno() != kzrjson_success) {
 *       switch (kzrjson_errno()) {
 *       case kzrjson_err_tokenize:
 *           // error handling
 *           break;
 *       case kzrjson_err_parse:
 *           // error handling
 *           break;
 *       case kzrjson_err_calloc:
 *           // error handling
 *           break;
 *       }
 *    }
 */
typedef enum {
	kzrjson_success = 0,
	kzrjson_err_tokenize,
	kzrjson_err_parse,
	kzrjson_err_calloc,
	kzrjson_err_not_number,
	kzrjson_err_illegal_type,
	kzrjson_err_object_key_not_found,
} kzrjson_errno_t;

kzrjson_errno_t kzrjson_errno(void);

/*****************************************************************************
 * Data types
 *****************************************************************************/
 /*
 * kzrjson_t is common type of this library.
 * All of JSON types are represented by this type.
 * You can get data by accessing member variables or using functions
 * in the library..
 * kzrjson_t is allocated to heap memory.
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
 * [errno] kzrjson_err_tokenize
 * [errno] kzrjson_err_parse
 * [errno] kzrjson_err_calloc
 */
kzrjson_t kzrjson_parse(const char *json_text);

/*****************************************************************************
 * Print JSON
 *****************************************************************************/
/*
 * Print kzrjson_t to stdout with indent.
 */
void kzrjson_print(kzrjson_t any);

/*****************************************************************************
 * Operations of kzrjson_t
 *****************************************************************************/

/*
 * Get a member from the object by key.
 *
 * [errno] kzrjson_err_illegal_type
 * [errno] kzrjson_err_object_key_not_found
 */
kzrjson_t kzrjson_get_member(kzrjson_t object, const char *key);

/*
 * Get a key of the member.
 *
 * [errno] kzrjson_err_illegal_type
 */
const char *kzrjson_get_member_key(kzrjson_t member);

/*
 * Get a value of the member from object by member.
 *
 * [errno] kzrjson_err_illegal_type
 * [errno] kzrjson_err_object_key_not_found
 */
kzrjson_t kzrjson_get_value_from_key(kzrjson_t object, const char *key);

/*****************************************************************************
 * Make JSON
 ****************************************************************************/
/*
 * [errno] kzrjson_err_calloc
 */
kzrjson_t kzrjson_make_object(void);

/*
 * [errno] kzrjson_err_calloc
 */
kzrjson_t kzrjson_make_array(void);

/*
 * Add the member to the object.
 *
 * [errno] kzrjson_err_illegal_type
 */
bool kzrjson_object_add_member(kzrjson_t object, kzrjson_t member);

/*
 * Add the element to the array.
 *
 * [errno] kzrjson_err_illegal_type
 */
bool kzrjson_array_add_element(kzrjson_t array, kzrjson_t element);

/*
 * Make member from key and value.
 *
 * [errno] kzrjson_err_calloc
 */
kzrjson_t kzrjson_make_member(const char *key, const size_t key_length, kzrjson_t value);

/*
 * [errno] kzrjson_err_calloc
 */
kzrjson_t kzrjson_make_string(const char *string, const size_t length);

/*
 * [errno] kzrjson_err_calloc
 */
kzrjson_t kzrjson_make_boolean(const bool boolean);

/*
 * [errno] kzrjson_err_calloc
 */
kzrjson_t kzrjson_make_null(void);

/*
 * [errno] kzrjson_err_not_number
 * [errno] kzrjson_err_calloc
 */
kzrjson_t kzrjson_make_number_double(const double number);

 /*
  * [errno] kzrjson_err_not_number
  * [errno] kzrjson_err_calloc
  */
kzrjson_t kzrjson_make_number_uint(const uint64_t number);

 /*
  * [errno] kzrjson_err_not_number
  * [errno] kzrjson_err_calloc
  */
kzrjson_t kzrjson_make_number_int(const int64_t number);

/*
 * Make number from exponential anotation number as string.
 *
 * [errno] kzrjson_err_not_number
 * [errno] kzrjson_err_calloc
 */
kzrjson_t kzrjson_make_number_exp(const char *exp, const size_t length);

 /*
  * [errno] kzrjson_err_calloc
  */
kzrjson_text_t kzrjson_to_string(kzrjson_t data);

#endif // KZRJSON_H
