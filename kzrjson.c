#include "kzrjson.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************
 * Functions to handling exception
 *****************************************************************************/
typedef struct {
	kzrjson_exception_name name;
	const char *what;
} kzrjson_exception_t;

static kzrjson_exception_t g_exception = {
	.name = kzrjson_success
};

static void exception_set_success(void) {
	g_exception.name = kzrjson_success;
}

// [no exception]
bool kzrjson_catch_exception(void) {
	return g_exception.name != kzrjson_success;
}

kzrjson_exception_name kzrjson_exception(void) {
	return g_exception.name;
}

static const char *exception_what() {
	switch (kzrjson_exception()) { 
	case kzrjson_success:
		return "success";
	case kzrjson_exception_tokenize:
		return "failed to tokenize";
	case kzrjson_exception_parse:
		return "failed to parse";
	case kzrjson_exception_failed_to_allocate_memory:
		return "failed to allocate memory";
	case kzrjson_exception_not_number:
		return "input number token is not number";
	case kzrjson_exception_illegal_type:
		return "illegal type";
	case kzrjson_exception_array_index_out_of_range:
		return "array index out of range";
	case kzrjson_exception_object_key_not_found:
		return "object key not found";
	default:
		return "";
	}
}

// [no exception]
static void throw_exception(kzrjson_exception_name type) {
	g_exception.name = type;
	g_exception.what = exception_what();
}

static const char begin_array = '[';
static const char begin_object = '{';
static const char end_array = ']';
static const char end_object = '}';
static const char name_separator = ':';
static const char value_separator = ',';

/*
ws = *(
	%x20 / ; Space
	%x09 / ; Horizontal tab
	%x0A / ; Line feed or New line
	%x0D ) ; Carriage return
*/
static const char white_spaces[] = {' ', 0x09, 0x0A, 0x0D, '\0'};
static const char *literal_false = "false";
static const char *literal_null = "null";
static const char *literal_true = "true";
static const char decimal_point = '.';
		
static const char *digit0_9 = "0123456789";
static const char e[] = {'e', 'E', '\0'};
static const char minus = '-';
static const char plus = '+';
static const char zero = '0';
static const char escape = 0x5C;
static const char quotation_mark = 0x22;

typedef enum {
	kzrjson_token_error = 0,
	kzrjson_token_begin_array,
	kzrjson_token_begin_object,
	kzrjson_token_end_array,
	kzrjson_token_end_object,
	kzrjson_token_name_separator,
	kzrjson_token_value_separator,
	kzrjson_token_white_space,
	kzrjson_token_literal_false,
	kzrjson_token_null,
	kzrjson_token_literal_true,
	kzrjson_token_decimal_point,
	kzrjson_token_digit0_9,
	kzrjson_token_e,
	kzrjson_token_minus,
	kzrjson_token_plus,
	kzrjson_token_zero,
	kzrjson_token_escape,
	kzrjson_token_quotation_mark,
	kzrjson_token_string,
	kzrjson_token_end_of_text,
} kzrjson_token_type;

static struct {
	kzrjson_token_type type;
	const char *begin;
	size_t length;
} current_token;

static struct {
	const char *text;
	const char *pos;
} lexer;

// [no exception]
static void set_lexer(const char *json_text) {
	lexer.text = json_text;
	lexer.pos = lexer.text;
}

// [no exception]
static void next() {
	lexer.pos++;
}

// [no exception]
static bool end_of_text() {
	return *lexer.pos == '\0';
}

// [no exception]
static kzrjson_token_type set_token(
	kzrjson_token_type type,
	const char *begin,
	const size_t length)
{
	current_token.type = type;
	current_token.begin = begin;
	current_token.length = length;
	return type;
}

// [no exception]
static kzrjson_token_type set_token_char(kzrjson_token_type type) {
	return set_token(type, current_token.begin, 1);
}

// [no exception]
static kzrjson_token_type set_token_eot() {
	return set_token(kzrjson_token_end_of_text, NULL, 0);
}

// [no exception]
static bool consume_if(const char c) {
	if (*lexer.pos == c) {
		next();
		return true;
	}
	return false;
}

// [no exception]
static bool contained(const char *chars) {
	return strchr(chars, *lexer.pos) != NULL;
}

// [no exception]
static bool consume_if_contained(const char *chars) {
	if (contained(chars)) {
		next();
		return true;
	}
	return false;
}

// [no exception]
static bool consume_literal(const char *literal) {
	if (strstr(lexer.pos, literal) == lexer.pos) {
		lexer.pos += strlen(literal);
		return true;
	}
	return false;
}

/*
 * string = quotation-mark *char quotation-mark
 * char = unescaped /
 * escape (
 *   %x22 /          ; "    quotation mark  U+0022
 *   %x5C /          ; \    reverse solidus U+005C
 *   %x2F /          ; /    solidus         U+002F
 *   %x62 /          ; b    backspace       U+0008
 *   %x66 /          ; f    form feed       U+000C
 *   %x6E /          ; n    line feed       U+000A
 *   %x72 /          ; r    carriage return U+000D
 *   %x74 /          ; t    tab             U+0009
 *   %x75 4HEXDIG )  ; uXXXX                U+XXXX
 * unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
 * 
 * [exception] kzrjson_exception_tokenize
 */
static kzrjson_token_type set_token_string() {
	const char *begin = lexer.pos;
	int length = 0;
	for (; *lexer.pos != quotation_mark; next()) {
		if (end_of_text()) {
			throw_exception(kzrjson_exception_tokenize);
			return 0;
		}
		static const char escape_targets[] = {0x22, 0x5C, 0x2F, 0x62, 0x66, 0x6E, 0x72, 0x74, 0x75, '\0'};
		if (*lexer.pos == escape) {
			next();
			if (end_of_text()) {
				throw_exception(kzrjson_exception_tokenize);
				return 0;
			}
			if (strchr(escape_targets, *lexer.pos) == NULL) {
				throw_exception(kzrjson_exception_tokenize);
				return 0;
			}
			length += 2;
		} else {
			length++;
		}
	}
	next(); // consume quotation_mark
	return set_token(kzrjson_token_string, begin, length);
}

/*
 * Get a token and set it to current_token.
 * Return it's token type.
 * 
 * [exception] kzrjson_exception_tokenize
 *    return kzrjson_token_error
 */
static kzrjson_token_type get_token() {
	if (end_of_text()) return set_token_eot();
	while (consume_if_contained(white_spaces)) {
		if (end_of_text()) return set_token_eot();
	}
	if (consume_if(begin_array)) {
		return set_token_char(kzrjson_token_begin_array);
	} else if (consume_if(begin_object)) {
		return set_token_char(kzrjson_token_begin_object);
	} else if (consume_if(end_array)) {
		return set_token_char(kzrjson_token_end_array);
	} else if (consume_if(end_object)) {
		return set_token_char(kzrjson_token_end_object);
	} else if (consume_if(name_separator)) {
		return set_token_char(kzrjson_token_name_separator);
	} else if (consume_if(value_separator)) {
		return set_token_char(kzrjson_token_value_separator);
	} else if (consume_if(decimal_point)) {
		return set_token_char(kzrjson_token_decimal_point);
	} else if (contained(digit0_9)) {
		set_token_char(kzrjson_token_digit0_9);
		next();
		return kzrjson_token_digit0_9;
	} else if (consume_if_contained(e)) {
		set_token_char(kzrjson_token_e);
		next();
		return kzrjson_token_e;
	} else if (consume_if(minus)) {
		return set_token_char(kzrjson_token_minus);
	} else if (consume_if(plus)) {
		return set_token_char(kzrjson_token_plus);
	} else if (consume_if(escape)) {
		return set_token_char(kzrjson_token_escape);
	} else if (consume_if(quotation_mark)) {
		const kzrjson_token_type type = set_token_string();
		if (kzrjson_catch_exception()) return kzrjson_token_error;
		return type;
	} else {
		if (consume_literal(literal_false)) {
			return set_token(kzrjson_token_literal_false, literal_false, strlen(literal_false));
		} else if (consume_literal(literal_true)) {
			return set_token(kzrjson_token_literal_true, literal_true, strlen(literal_true));
		} else if (consume_literal(literal_null)) {
			return set_token(kzrjson_token_null, literal_null, strlen(literal_null));
		} else {
			throw_exception(kzrjson_exception_tokenize);
			return kzrjson_token_error;
		}
	}
}

/*
 * Add the element to the array or object.
 * 
 * [exception] kzrjson_exception_failed_to_allocate_memory
 */
static void add_element(kzrjson_t array_or_object, kzrjson_t element) {
	if (array_or_object == NULL) return;
	if (element == NULL) return;
	array_or_object->elements_size++;
	if (array_or_object->elements_size == 1) {
		array_or_object->elements = calloc(1, sizeof(struct kzrjson_t));
		if (array_or_object->elements == NULL) {
			goto throw_exp;
		}
		*array_or_object->elements = element;
	} else {
		array_or_object->elements = realloc(array_or_object->elements, array_or_object->elements_size * sizeof(struct kzrjson_t));
		if (array_or_object->elements == NULL) {
			goto throw_exp;
		}
		*(array_or_object->elements + array_or_object->elements_size - 1) = element;
	}
	return;

throw_exp:
	throw_exception(kzrjson_exception_failed_to_allocate_memory);
}

/*
 * [no exception]
 */
static void add_value(kzrjson_t member, kzrjson_t value) {
	if (member == NULL) return;
	if (value == NULL) return;
	member->value = value;
}

/*
 * Make new kzrjson_t.
 * 
 * [exception] kzrjson_exception_failed_to_allocate_memory
 *    return NULL
 */
static kzrjson_t make_json(const kzrjson_type type, char *string) {
	kzrjson_t any = calloc(1, sizeof(struct kzrjson_t));
	if (any == NULL) {
		throw_exception(kzrjson_exception_failed_to_allocate_memory);
		return NULL;
	}
	any->type = type;
	any->string = string;
	any->elements = NULL;
	any->elements_size = 0;
	any->key = NULL;
	return any;
}

/*
 * Make new string kzrjson_t.
 *
 * [exception] kzrjson_exception_failed_to_allocate_memory
 *    return NULL
 */
static kzrjson_t make_string(const char *string) {
	return make_json(kzrjson_string, (char *)string);
}

/*
 * Make new number kzrjson_t.
 *
 * [exception] kzrjson_exception_failed_to_allocate_memory
 *    return NULL
 * [exception] kzrjson_exception_not_number
 *    return NULL
 */
static kzrjson_t make_number(char *number, kzrjson_number_type type) {
	kzrjson_t data = make_json(kzrjson_number, number);
	if (data == NULL) {
		throw_exception(kzrjson_exception_failed_to_allocate_memory);
		return NULL;
	}
	data->number_type = type;
	char *end;
	switch (data->number_type) {
	case kzrjson_int:
		data->number_int = strtoll(data->string, &end, 10);
		if (data->string == end) goto throw_exp;
		break;
	case kzrjson_uint:
		data->number_uint = strtoull(data->string, &end, 10);
		if (data->string == end) goto throw_exp;
		break;
	case kzrjson_double:
	case kzrjson_exp:
		data->number_double = strtod(data->string, &end);
		if (data->string == end) goto throw_exp;
		break;
	}
	return data;

throw_exp:
	throw_exception(kzrjson_exception_not_number);
	return NULL;
}

/*
 * Make new boolean kzrjson_t.
 *
 * [exception] kzrjson_exception_failed_to_allocate_memory
 *    return NULL
 */
static kzrjson_t make_boolean(const bool boolean) {
	kzrjson_t json = make_json(kzrjson_bool,
		(char *)(boolean ? literal_true : literal_false)
	);
	json->boolean = boolean;
	return json;
}

/*
 * Make new null kzrjson_t.
 *
 * [exception] kzrjson_exception_failed_to_allocate_memory
 *    return NULL
 */
static kzrjson_t make_null() {
	return make_json(kzrjson_null, (char *)literal_null);
}

/*
 * Make new object kzrjson_t.
 *
 * [exception] kzrjson_exception_failed_to_allocate_memory
 *    return NULL
 */
static kzrjson_t make_object() {
	return make_json(kzrjson_object, NULL);
}

/*
 * Make new array kzrjson_t.
 *
 * [exception] kzrjson_exception_failed_to_allocate_memory
 *    return NULL
 */
static kzrjson_t make_array() {
	return make_json(kzrjson_array, NULL);
}

/*
 * Make new member kzrjson_t.
 *
 * [exception] kzrjson_exception_failed_to_allocate_memory
 *    return NULL
 */
static kzrjson_t make_member(const char *key) {
	kzrjson_t data = make_json(kzrjson_member, NULL);
	if (kzrjson_catch_exception()) return NULL;
	data->key = (char *)key;
	return data;
}

static kzrjson_t parse_object();
static kzrjson_t parse_array();
static kzrjson_t parse_number();
static kzrjson_t parse_member();
static kzrjson_t parse_value();

/*
 * [no exception]
 */
static bool current_is(const kzrjson_token_type type) {
	return current_token.type == type;
}

/*
 * Current token must be the type. If not, throw exception.
 * 
 * [exception] kzrjson_exception_parse
 */
static void current_must(const kzrjson_token_type type) {
	if (!current_is(type)) {
		throw_exception(kzrjson_exception_parse);
		return;
	}
}

/*
 * Copy string to heap memory.
 * 
 * [exception] kzrjson_exception_failed_to_allocate_memory
 *   return NULL
 */
static char *copy_string(const char *from, const size_t length) {
	char *buffer = calloc(length + 1, sizeof(char));
	if (buffer == NULL) {
		throw_exception(kzrjson_exception_failed_to_allocate_memory);
		return NULL;
	}
	strncpy_s(buffer, length + 1, from, length);
	return buffer;
}

/*
 * parse-JSON-text = ws value ws
 * 
 * [exception] kzrjson_exception_failed_to_allocate_memory
 * [exception] kzrjson_exception_parse
 * [exception] kzrjson_exception_tokenize
 * [exception] kzrjson_exception_not_number
 */
static kzrjson_t parse_json_text() {
	get_token();
	if (kzrjson_catch_exception()) return NULL;
	return parse_value();
}

/*
 * value = false / null / true / string / object / array / number
 * 
 * [exception] kzrjson_exception_failed_to_allocate_memory
 * [exception] kzrjson_exception_parse
 * [exception] kzrjson_exception_tokenize
 * [exception] kzrjson_exception_not_number
 */
static kzrjson_t parse_value() {
	if (current_is(kzrjson_token_literal_false)) {
		kzrjson_t data = make_boolean(false);
		if (kzrjson_catch_exception()) goto throw_exp;
		get_token();
		if (kzrjson_catch_exception()) goto throw_exp;
		return data;
	} else if (current_is(kzrjson_token_literal_true)) {
		kzrjson_t data = make_boolean(true);
		if (kzrjson_catch_exception()) goto throw_exp;
		get_token();
		if (kzrjson_catch_exception()) goto throw_exp;
		return data;
	} else if (current_is(kzrjson_token_null)) {
		kzrjson_t data = make_null();
		if (kzrjson_catch_exception()) goto throw_exp;
		get_token();
		if (kzrjson_catch_exception()) goto throw_exp;
		return data;
	} else if (current_is(kzrjson_token_string)) {
		char *string = copy_string(current_token.begin, current_token.length);
		if (kzrjson_catch_exception()) goto throw_exp;
		kzrjson_t data = make_string(string);
		if (kzrjson_catch_exception()) goto throw_exp;
		get_token();
		if (kzrjson_catch_exception()) goto throw_exp;
		return data;
	} else if (current_is(kzrjson_token_begin_object)) {
		return parse_object();
	} else if (current_is(kzrjson_token_begin_array)) {
		return parse_array();
	} else {
		return parse_number();
	}

throw_exp:
	return NULL;
}

// object = begin-object [ member *( value-separator member ) ] end-object
// [exception] kzrjson_exception_failed_to_allocate_memory
static kzrjson_t parse_object() {
	kzrjson_t object = make_object();
	if (kzrjson_catch_exception()) return NULL;
	current_must(kzrjson_token_begin_object);
	if (kzrjson_catch_exception()) return NULL;
	get_token();
	if (kzrjson_catch_exception()) return NULL;
	add_element(object, parse_member());
	while (!current_is(kzrjson_token_end_object)) {
		current_must(kzrjson_token_value_separator);
		if (kzrjson_catch_exception()) return NULL;
		get_token();
		if (kzrjson_catch_exception()) return NULL;
		add_element(object, parse_member());
	}
	get_token();
	if (kzrjson_catch_exception()) return NULL;
	return object;
}

// array = begin-array [ value *( value-separator value ) ] end-array
static kzrjson_t parse_array() {
	kzrjson_t array = make_array();
	if (kzrjson_catch_exception()) return NULL;
	current_must(kzrjson_token_begin_array);
	if (kzrjson_catch_exception()) return NULL;
	get_token();
	if (kzrjson_catch_exception()) return NULL;
	add_element(array, parse_value());
	while (!current_is(kzrjson_token_end_array)) {
		current_must(kzrjson_token_value_separator);
		if (kzrjson_catch_exception()) return NULL;
		get_token();
		if (kzrjson_catch_exception()) return NULL;
		add_element(array, parse_value());
	}
	get_token();
	if (kzrjson_catch_exception()) return NULL;
	return array;
}

// member = string name-separator value
static kzrjson_t parse_member() {
	current_must(kzrjson_token_string);
	if (kzrjson_catch_exception()) return NULL;
	char *buffer = copy_string(current_token.begin, current_token.length);
	kzrjson_t member = make_member(buffer);
	if (kzrjson_catch_exception()) return NULL;
	get_token();
	if (kzrjson_catch_exception()) return NULL;
	current_must(kzrjson_token_name_separator);
	if (kzrjson_catch_exception()) return NULL;
	get_token();
	if (kzrjson_catch_exception()) return NULL;
	add_value(member, parse_value());
	return member;
}

// number = [ minus ] int [ frac ] [ exp ]
static kzrjson_t parse_number() {
	const char *begin = lexer.pos - 1;
	kzrjson_token_type token = current_token.type;
	kzrjson_number_type type = kzrjson_uint;
	if (token == kzrjson_token_minus) {
		type = kzrjson_int;
		token = get_token();
		if (kzrjson_catch_exception()) return NULL;
	}

	// int = zero / ( digit1-9 *DIGIT )
	for (; token == kzrjson_token_digit0_9; token = get_token()) {
		if (kzrjson_catch_exception()) return NULL;
	}

	// frac = decimal-point 1*DIGIT
	if (token == kzrjson_token_decimal_point) {
		type = kzrjson_double;
		for (token = get_token(); token == kzrjson_token_digit0_9; token = get_token()) {
			if (kzrjson_catch_exception()) return NULL;
		}
	}

	// exp = e [ minus / plus ] 1*DIGIT
	if (token == kzrjson_token_e) {
		type = kzrjson_exp;
		token = get_token();
		if (token == kzrjson_token_minus || token == kzrjson_token_plus) {
			token = get_token();
			if (kzrjson_catch_exception()) return NULL;
		}
		for (; token == kzrjson_token_digit0_9; token = get_token()) {
			if (kzrjson_catch_exception()) return NULL;
		}
	}
	size_t length = lexer.pos - begin - 1;
	char *number = copy_string(begin, length);
	if (number == NULL) {
		throw_exception(kzrjson_exception_failed_to_allocate_memory);
		return NULL;
	}
	return make_number(number, type);
}

static int g_indent = 0;
static void print_indent(void) {
	for (int i = 0; i < g_indent; i++) {
		printf("  "); // 2 space
	}
}

static void kzrjson_inner_print(kzrjson_t inner) {
	if (inner == NULL) return;
	switch (inner->type) {
	case kzrjson_object: {
		printf("%c\n", begin_object);
		g_indent++;
		for (int i = 0; i < inner->elements_size; i++) {
			print_indent();
			kzrjson_inner_print(*(inner->elements + i));
			if (i + 1 != inner->elements_size) {
				printf("%c\n", value_separator);
			}
		}
		puts("");
		g_indent--;
		print_indent();
		printf("%c", end_object);
		break;
	}
	case kzrjson_array: {
		printf("%c\n", begin_array);
		g_indent++;
		for (int i = 0; i < inner->elements_size; i++) {
			print_indent();
			kzrjson_inner_print(*(inner->elements + i));
			if (i + 1 != inner->elements_size) {
				printf("%c\n", value_separator);
			}
		}
		puts("");
		g_indent--;
		print_indent();
		printf("%c", end_array);
		break;
	}
	case kzrjson_member:
		printf("%c%s%c%c ", quotation_mark, inner->key, quotation_mark, name_separator);
		kzrjson_inner_print(inner->value);
		break;
	case kzrjson_string:
		printf("%c%s%c", quotation_mark, inner->string, quotation_mark);
		break;
	case kzrjson_number:
	case kzrjson_bool:
	case kzrjson_null:
		printf("%s", inner->string);
		break;
	}
}

static void kzrjson_inner_free(kzrjson_t inner) {
	if (inner == NULL) return;
	switch (inner->type) {
	case kzrjson_array:
	case kzrjson_object:
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_free(*(inner->elements + i));
		}
		free(inner->elements);
		break;
	case kzrjson_member:
		free(inner->key);
		kzrjson_inner_free(inner->value);
		break;
	case kzrjson_string:
		free(inner->string);
		break;
	case kzrjson_number:
		free(inner->string);
		break;
	case kzrjson_bool:
	case kzrjson_null:
		break;
	}
	free(inner);
}

static struct {
	size_t length;
	char *begin;
	char *pos;
} g_converter;

static size_t kzrjson_inner_length(kzrjson_t inner) {
	switch (inner->type) {
	case kzrjson_object:
		g_converter.length++; // begin-object
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_length(*(inner->elements + i));
			if (i + 1 != inner->elements_size) {
				g_converter.length++; // value-separator
			}
		}
		g_converter.length++; // end-object
		break;
	case kzrjson_array:
		g_converter.length++; // begin-array
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_length(*(inner->elements + i));
			if (i + 1 != inner->elements_size) {
				g_converter.length++; // value-separator
			}
		}
		g_converter.length++; // end-array
		break;
	case kzrjson_member:
		g_converter.length++; // quatation-mark
		g_converter.length += strlen(inner->key);
		g_converter.length++; // quatation-mark
		g_converter.length++; // name-separator
		kzrjson_inner_length(inner->value);
		break;
	case kzrjson_string:
		g_converter.length++; // quatation-mark
		g_converter.length += strlen(inner->string);
		g_converter.length++; // quatation-mark
		break;
	case kzrjson_number:
		g_converter.length += strlen(inner->string);
		break;
	case kzrjson_bool:
		g_converter.length += strlen(inner->string);
		break;
	case kzrjson_null:
		g_converter.length += strlen(inner->string);
		break;
	}
	return g_converter.length;
}

static void converter_add_char(const char c) {
	*g_converter.pos = c;
	g_converter.pos++;
}

static void converter_add_string(const char *string) {
	const size_t remain = g_converter.length - (g_converter.pos - g_converter.begin);
	strcpy_s(g_converter.pos, remain, string);
	g_converter.pos += strlen(string);
}

static void kzrjson_inner_to_string(kzrjson_t inner) {
	switch (inner->type) {
	case kzrjson_object:
		converter_add_char(begin_object);
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_to_string(*(inner->elements + i));
			if (i + 1 != inner->elements_size) {
				converter_add_char(value_separator);
			}
		}
		converter_add_char(end_object);
		break;
	case kzrjson_array:
		converter_add_char(begin_array);
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_to_string(*(inner->elements + i));
			if (i + 1 != inner->elements_size) {
				converter_add_char(value_separator);
			}
		}
		converter_add_char(end_array);
		break;
	case kzrjson_member:
		converter_add_char(quotation_mark);
		converter_add_string(inner->key);
		converter_add_char(quotation_mark);
		converter_add_char(name_separator);
		kzrjson_inner_to_string(inner->value);
		break;
	case kzrjson_string:
		converter_add_char(quotation_mark);
		converter_add_string(inner->string);
		converter_add_char(quotation_mark);
		break;
	case kzrjson_number:
		converter_add_string(inner->string);
		break;
	case kzrjson_bool:
		converter_add_string(inner->string);
		break;
	case kzrjson_null:
		converter_add_string(inner->string);
		break;
	}
}

/*****************************************************************************
 * Intefaces
 *****************************************************************************/
void kzrjson_print(kzrjson_t any) {
	exception_set_success();
	g_indent = 0;
	kzrjson_inner_print(any);
	printf("\n");
	g_indent = 0;
}

void kzrjson_free(kzrjson_t any) {
	exception_set_success();
	if (any == NULL) return;
	kzrjson_inner_free(any);
}

kzrjson_t kzrjson_parse(const char *json_text) {
	exception_set_success();

	set_lexer(json_text);

	kzrjson_t any;
	any = parse_json_text();
	if (kzrjson_catch_exception()) {
		const kzrjson_exception_name exception = kzrjson_exception();
		kzrjson_free(any);
		throw_exception(exception);
		return NULL;
	}

	lexer.pos = NULL;
	lexer.text = NULL;

	return any;
}

kzrjson_t kzrjson_get_member(kzrjson_t object, const char *key) {
	exception_set_success();
	if (object->type != kzrjson_object) {
		throw_exception(kzrjson_exception_illegal_type);
		return NULL;
	}
	for (int i = 0; i < object->elements_size; i++) {
		kzrjson_t member = *(object->elements + i);
		if (strcmp(key, member->key) == 0) {
			return member;
		}
	}
	throw_exception(kzrjson_exception_object_key_not_found);
	return NULL;
}

kzrjson_t kzrjson_get_value_from_key(kzrjson_t object, const char *key) {
	exception_set_success();
	if (object->type != kzrjson_object) {
		throw_exception(kzrjson_exception_illegal_type);
		return NULL;
	}
	kzrjson_t member = kzrjson_get_member(object, key);
	if (kzrjson_catch_exception()) {
		return NULL;
	}
	return member->value;
}

kzrjson_t kzrjson_get_element(kzrjson_t array, size_t index) {
	exception_set_success();
	if (array->type != kzrjson_array) {
		throw_exception(kzrjson_exception_illegal_type);
		return NULL;
	}
	if (index < 0 || index >= array->elements_size) {
		throw_exception(kzrjson_exception_array_index_out_of_range);
		return NULL;
	}
	return *(array->elements + index);
}

kzrjson_t kzrjson_make_object(void) {
	exception_set_success();
	kzrjson_t json = make_object();
	if (kzrjson_catch_exception()) return NULL;
	return json;
}

kzrjson_t kzrjson_make_array(void) {
	exception_set_success();
	kzrjson_t json = make_array();
	if (kzrjson_catch_exception()) return NULL;
	return json;
}

bool kzrjson_object_add_member(kzrjson_t object, kzrjson_t member) {
	if (!object || !member) return false;
	exception_set_success();
	if (object->type != kzrjson_object) {
		throw_exception(kzrjson_exception_illegal_type);
		return false;
	}
	if (member->type != kzrjson_member) {
		throw_exception(kzrjson_exception_illegal_type);
		return false;
	}
	add_element(object, member);
	return true;
}

bool kzrjson_array_add_element(kzrjson_t array, kzrjson_t element) {
	if (!array || !element) return false;
	exception_set_success();
	if (array->type != kzrjson_array) {
		throw_exception(kzrjson_exception_illegal_type);
		return false;
	}
	add_element(array, element);
	return true;
}

kzrjson_t kzrjson_make_member(const char *key, const size_t key_length, kzrjson_t value) {
	exception_set_success();

	char *buffer = copy_string(key, key_length);
	if (kzrjson_catch_exception()) return NULL;

	kzrjson_t member = make_member(buffer);
	if (kzrjson_catch_exception()) return NULL;

	add_value(member, value);
	return member;
}

kzrjson_t kzrjson_make_string(const char *string, const size_t length) {
	exception_set_success();

	char *buffer = copy_string(string, length);
	if (kzrjson_catch_exception()) return NULL;

	kzrjson_t json = make_string(buffer);
	if (kzrjson_catch_exception()) return NULL;

	return json;
}

kzrjson_t kzrjson_make_boolean(const bool boolean) {
	exception_set_success();
	kzrjson_t json = make_boolean(boolean);
	if (kzrjson_catch_exception()) return NULL;
	return json;
}

kzrjson_t kzrjson_make_null() {
	exception_set_success();
	kzrjson_t json = make_null();
	if (kzrjson_catch_exception()) return NULL;
	return json;
}

kzrjson_t kzrjson_make_number_double(const double number) {
	exception_set_success();
	static const size_t double_max_digit = 16;
	char *buffer = calloc(double_max_digit + 1, sizeof(char));
	if (buffer == NULL) {
		throw_exception(kzrjson_exception_failed_to_allocate_memory);
		return NULL;
	}
	snprintf(buffer, double_max_digit + 1, "%lf", number);
	kzrjson_t json = make_number(buffer, kzrjson_double);
	if (kzrjson_catch_exception()) return NULL;
	return json;
}

kzrjson_t kzrjson_make_number_uint(const uint64_t number) {
	exception_set_success();
	static const size_t uint64_max_digit = 20;
	char *buffer = calloc(uint64_max_digit + 1, sizeof(char));
	if (buffer == NULL) {
		throw_exception(kzrjson_exception_failed_to_allocate_memory);
		return NULL;
	}
	snprintf(buffer, uint64_max_digit + 1, "%llu", number);
	kzrjson_t json = make_number(buffer, kzrjson_uint);
	if (kzrjson_catch_exception()) return NULL;
	return json;
}

kzrjson_t kzrjson_make_number_int(const int64_t number) {
	exception_set_success();
	static const size_t int64_t_max_digit = 19;
	char *buffer = calloc(int64_t_max_digit + 1, sizeof(char));
	if (buffer == NULL) {
		throw_exception(kzrjson_exception_failed_to_allocate_memory);
		return NULL;
	}
	snprintf(buffer, int64_t_max_digit + 1, "%lld", number);
	kzrjson_t json = make_number(buffer, kzrjson_int);
	if (kzrjson_catch_exception()) return NULL;
	return json;
}

kzrjson_t kzrjson_make_number_exp(const char *exp, const size_t length) {
	exception_set_success();

	char *end;
	(void) strtod(exp, &end);
	if (exp == end) {
		throw_exception(kzrjson_exception_not_number);
		return NULL;
	}

	char *buffer = copy_string(exp, length);
	if (kzrjson_catch_exception()) {
		return NULL;
	}
	kzrjson_t json = make_number(buffer, kzrjson_exp);
	if (kzrjson_catch_exception()) {
		return NULL;
	}
	return json;
}

kzrjson_text_t kzrjson_to_string(kzrjson_t data) {
	exception_set_success();
	g_converter.length = 0;

	kzrjson_text_t json;
	json.length = kzrjson_inner_length(data);
	g_converter.begin = calloc(g_converter.length + 1, sizeof(char));
	if (g_converter.begin == NULL) {
		throw_exception(kzrjson_exception_failed_to_allocate_memory);
		kzrjson_text_t fail = {
			.text = NULL, 
			.length = 0
		};
		return fail;
	}
	g_converter.pos = g_converter.begin;
	kzrjson_inner_to_string(data);
	json.text = g_converter.begin;

	g_converter.begin = NULL;
	g_converter.length = 0;

	return json;
}
