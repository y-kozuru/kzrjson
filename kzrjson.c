#include "kzrjson.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char begin_array = '['; // begin-array = ws %x5B ws  ; [ left square bracket
static const char begin_object = '{'; // begin-object = ws %x7B ws  ; { left curly bracket
static const char end_array = ']'; // end-array = ws %x5D ws  ; ] right square bracket
static const char end_object = '}'; // end-object = ws %x7D ws  ; } right curly bracket
static const char name_separator = ':'; // name-separator = ws %x3A ws  ; : colon
static const char value_separator = ','; // value-separator = ws %x2C ws ; , comma

/*
ws = *(
	%x20 / ; Space
	%x09 / ; Horizontal tab
	%x0A / ; Line feed or New line
	%x0D ) ; Carriage return
*/
static const char white_spaces[] = {' ', 0x09, 0x0A, 0x0D, '\0'};
static const char *literal_false = "false"; // false = %x66.61.6c.73.65 ; false
static const char *literal_null = "null"; // null  = %x6e.75.6c.6c ; null
static const char *literal_true = "true"; // true  = %x74.72.75.65 ; true
static const char decimal_point = '.'; // decimal-point = %x2E ; .
		
static const char *digit0_9 = "0123456789";
static const char e[] = {'e', 'E', '\0'}; // e = %x65 / %x45 ; e E
static const char minus = '-'; // minus = %x2D ; -
static const char plus = '+'; // plus = %x2B ; +
static const char zero = '0'; // zero = %x30 ; 0
static const char escape = 0x5C; // escape = %x5C ; '\'
static const char quotation_mark = 0x22; // quotation-mark = %x22 ; "

typedef enum {
	token_type_begin_array,
	token_type_begin_object,
	token_type_end_array,
	token_type_end_object,
	token_type_name_separator,
	token_type_value_separator,
	token_type_white_space,
	token_type_literal_false,
	token_type_null,
	token_type_literal_true,
	token_type_decimal_point,
	token_type_digit0_9,
	token_type_e,
	token_type_minus,
	token_type_plus,
	token_type_zero,
	token_type_escape,
	token_type_quotation_mark,
	token_type_string,
	token_type_end_of_text,
	token_type_lexer_error,
} token_type;

static const char *token_type_to_string(const token_type type) {
	switch (type) {
	case token_type_begin_array:     return "begin_array";
	case token_type_begin_object:    return "begin_object";
	case token_type_end_array:       return "end_array";
	case token_type_end_object:      return "end_object";
	case token_type_name_separator:  return "name_separator";
	case token_type_value_separator: return "value_separator";
	case token_type_white_space:     return "white_space";
	case token_type_literal_false:   return "literal_false";
	case token_type_null:            return "null";
	case token_type_literal_true:    return "literal_true";
	case token_type_decimal_point:   return "decimal_point";
	case token_type_digit0_9:        return "digit0_9";
	case token_type_e:               return "e";
	case token_type_minus:           return "minus";
	case token_type_plus:            return "plus";
	case token_type_zero:            return "zero";
	case token_type_escape:          return "escape";
	case token_type_quotation_mark:  return "quotation_mark";
	case token_type_string:          return "string";
	case token_type_end_of_text:     return "end_of_text";
	case token_type_lexer_error:     return "lexer_error";
	default:                         return "";
	}
}

static struct {
	token_type type;
	const char *begin;
	size_t length;
} current_token;

static struct {
	const char *text;
	const char *pos;
} lexer;

static void set_lexer(const char *json_text) {
	lexer.text = json_text;
	lexer.pos = lexer.text;
}

static void next() {
	lexer.pos++;
}

static bool end_of_text() {
	return *lexer.pos == '\0';
}

static token_type set_token(token_type type, const char *begin, const size_t length) {
	current_token.type = type;
	current_token.begin = begin;
	current_token.length = length;
	return type;
}

static token_type set_token_char(token_type type) {
	return set_token(type, current_token.begin, 1);
}

static token_type set_token_eot() {
	return set_token(token_type_end_of_text, NULL, 0);
}

static bool consume_if(const char c) {
	if (*lexer.pos == c) {
		next();
		return true;
	}
	return false;
}

static bool contained(const char *chars) {
	return strchr(chars, *lexer.pos) != NULL;
}

static bool consume_if_contained(const char *chars) {
	if (contained(chars)) {
		next();
		return true;
	}
	return false;
}

static bool consume_literal(const char *literal) {
	if (strstr(lexer.pos, literal) == lexer.pos) {
		lexer.pos += strlen(literal);
		return true;
	}
	return false;
}

static token_type set_token_string() {
	/*
	string = quotation-mark *char quotation-mark
	char = unescaped /
	escape (
		%x22 /          ; "    quotation mark  U+0022
		%x5C /          ; \    reverse solidus U+005C
		%x2F /          ; /    solidus         U+002F
		%x62 /          ; b    backspace       U+0008
		%x66 /          ; f    form feed       U+000C
		%x6E /          ; n    line feed       U+000A
		%x72 /          ; r    carriage return U+000D
		%x74 /          ; t    tab             U+0009
		%x75 4HEXDIG )  ; uXXXX                U+XXXX
	unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
	*/
	const char *begin = lexer.pos;
	int length = 0;
	for (; *lexer.pos != quotation_mark; next()) {
		if (end_of_text()) {
			fprintf_s(stderr, "failed to tokenize");
			exit(1);
		}
		length++;
	}
	next(); // consume_if quotation_mark
	return set_token(token_type_string, begin, length);
}

// トークンを一つ読み込み、current_tokenにセットし、current_tokenのタイプを返す。
static token_type get_token() {
	if (end_of_text()) return set_token_eot();
	while (consume_if_contained(white_spaces)) {
		if (end_of_text()) return set_token_eot();
	}
	if (consume_if(begin_array)) {
		return set_token_char(token_type_begin_array);
	} else if (consume_if(begin_object)) {
		return set_token_char(token_type_begin_object);
	} else if (consume_if(end_array)) {
		return set_token_char(token_type_end_array);
	} else if (consume_if(end_object)) {
		return set_token_char(token_type_end_object);
	} else if (consume_if(name_separator)) {
		return set_token_char(token_type_name_separator);
	} else if (consume_if(value_separator)) {
		return set_token_char(token_type_value_separator);
	} else if (consume_if(decimal_point)) {
		return set_token_char(token_type_decimal_point);
	} else if (contained(digit0_9)) {
		set_token_char(token_type_digit0_9);
		next();
		return token_type_digit0_9;
	} else if (consume_if_contained(e)) {
		set_token_char(token_type_e);
		next();
		return token_type_e;
	} else if (consume_if(minus)) {
		return set_token_char(token_type_minus);
	} else if (consume_if(plus)) {
		return set_token_char(token_type_plus);
	} else if (consume_if(escape)) {
		return set_token_char(token_type_escape);
	} else if (consume_if(quotation_mark)) {
		return set_token_string();
	} else {
		if (consume_literal(literal_false)) {
			return set_token(token_type_literal_false, literal_false, strlen(literal_false));
		} else if (consume_literal(literal_true)) {
			return set_token(token_type_literal_true, literal_true, strlen(literal_true));
		} else if (consume_literal(literal_null)) {
			return set_token(token_type_null, literal_null, strlen(literal_null));
		} else {
			fprintf_s(stderr, "failed to tokenize");
			exit(1);
		}
	}
}

typedef enum {
	json_type_object,
	json_type_array,
	json_type_string,
	json_type_number,
	json_type_boolean,
	json_type_null,
	json_type_member,
	json_type_end_of_elements,
} json_type;

static const char *json_type_to_string(const json_type type) {
	switch (type) {
	case json_type_object:
		return "object";
	case json_type_array:
		return "array";
	case json_type_string:
		return "string";
	case json_type_number:
		return "number";
	case json_type_boolean:
		return "boolean";
	case json_type_null:
		return "null";
	case json_type_member:
		return "member";
	default:
		return "";
	}
}

typedef enum {
	number_type_integer,
	number_type_unsigned_integer,
	number_type_double,
	number_type_exp,
} json_number_type;

typedef struct kzrjson_inner *kzrjson_inner;
struct kzrjson_inner {
	json_type type;
	kzrjson_inner *elements; // object array
	size_t elements_size;
	kzrjson_inner member_value; // member
	char *member_key;
	char *value; // string number true false null

	json_number_type number_type;
	union {
		int64_t number_integer;
		uint64_t number_unsigned_integer;
		double number_double;
	};
};

static void add_element(kzrjson_inner data, kzrjson_inner element) {
	// todo: add element to elements last
	data->elements_size++;
	if (data->elements_size == 1) {
		data->elements = calloc(1, sizeof(struct kzrjson_inner));
		*data->elements = element;
	} else {
		data->elements = realloc(data->elements, data->elements_size * sizeof(struct kzrjson_inner));
		*(data->elements + data->elements_size - 1) = element;
	}
}

static void add_value(kzrjson_inner data, kzrjson_inner value) {
	data->member_value = value;
}

kzrjson_inner make_json_data(const json_type type, char *string) {
	kzrjson_inner data = calloc(1, sizeof(struct kzrjson_inner));
	data->type = type;
	data->value = string;
	data->elements = NULL;
	data->elements_size = 0;
	data->member_key = NULL;
	return data;
}

kzrjson_inner make_string(const char *string) {
	return make_json_data(json_type_string, (char *)string);
}

kzrjson_inner make_number(char *number, json_number_type type) {
	kzrjson_inner data = make_json_data(json_type_number, number);
	data->number_type = type;
	switch (data->number_type) {
	case number_type_integer:
		data->number_integer = strtoll(data->value, NULL, 10);
		break;
	case number_type_unsigned_integer:
		data->number_unsigned_integer = strtoull(data->value, NULL, 10);
		break;
	case number_type_double:
	case number_type_exp:
		data->number_double = strtod(data->value, NULL);
		break;
	}
	return data;
}

kzrjson_inner make_boolean(const bool boolean) {
	return make_json_data(json_type_boolean, (char *)(boolean ? literal_true : literal_false));
}

kzrjson_inner make_null() {
	return make_json_data(json_type_null, (char *)literal_null);
}

kzrjson_inner make_object() {
	return make_json_data(json_type_object, NULL);
}

kzrjson_inner make_array() {
	return make_json_data(json_type_array, NULL);
}

kzrjson_inner make_member(const char *key) {
	kzrjson_inner data = make_json_data(json_type_member, NULL);
	data->member_key = (char *)key;
	return data;
}

static kzrjson_inner parse_object();
static kzrjson_inner parse_array();
static kzrjson_inner parse_number();
static kzrjson_inner parse_member();
static kzrjson_inner parse_value();

static bool current_is(const token_type type) {
	return current_token.type == type;
}

static void current_must(const token_type type) {
	if (!current_is(type)) {
		fprintf(stderr, "current must %s but %s\n", token_type_to_string(type), token_type_to_string(current_token.type));
		exit(1);
	}
}

static kzrjson_inner parse_json_text() {
	get_token();
	return parse_value();
}

// value = false / null / true / string / object / array / number
static kzrjson_inner parse_value() {
	if (current_is(token_type_literal_false)) {
		kzrjson_inner data = make_boolean(false);
		get_token();
		return data;
	} else if (current_is(token_type_literal_true)) {
		kzrjson_inner data = make_boolean(true);
		get_token();
		return data;
	} else if (current_is(token_type_null)) {
		kzrjson_inner data = make_null();
		get_token();
		return data;
	} else if (current_is(token_type_string)) {
		char *buffer = calloc(current_token.length + 1, sizeof(char));
		strncpy_s(buffer, current_token.length + 1, current_token.begin, current_token.length);
		kzrjson_inner data = make_string(buffer);
		get_token();
		return data;
	} else if (current_is(token_type_begin_object)) {
		return parse_object();
	} else if (current_is(token_type_begin_array)) {
		return parse_array();
	} else {
		return parse_number();
	}
}

// object = begin-object [ member *( value-separator member ) ] end-object
static kzrjson_inner parse_object() {
	kzrjson_inner object = make_object();
	current_must(token_type_begin_object);
	get_token();
	add_element(object, parse_member());
	while (!current_is(token_type_end_object)) {
		current_must(token_type_value_separator);
		get_token();
		add_element(object, parse_member());
	}
	get_token();
	return object;
}

// array = begin-array [ value *( value-separator value ) ] end-array
static kzrjson_inner parse_array() {
	kzrjson_inner array = make_array();
	current_must(token_type_begin_array);
	get_token();
	add_element(array, parse_value());
	while (!current_is(token_type_end_array)) {
		current_must(token_type_value_separator);
		get_token();
		add_element(array, parse_value());
	}
	get_token();
	return array;
}

// member = string name-separator value
static kzrjson_inner parse_member() {
	current_must(token_type_string);
	char *buffer = calloc(current_token.length + 1, sizeof(char));
	strncpy_s(buffer, current_token.length + 1, current_token.begin, current_token.length);
	kzrjson_inner member = make_member(buffer);
	get_token();
	current_must(token_type_name_separator);
	get_token();
	add_value(member, parse_value());
	return member;
}

// number = [ minus ] int [ frac ] [ exp ]
static kzrjson_inner parse_number() {
	const char *begin = lexer.pos - 1;
	token_type token = current_token.type;
	json_number_type type = number_type_unsigned_integer;
	if (token == token_type_minus) {
		type = number_type_integer;
		token = get_token();
	}

	// int = zero / ( digit1-9 *DIGIT )
	for (; token == token_type_digit0_9; token = get_token());

	// frac = decimal-point 1*DIGIT
	if (token == token_type_decimal_point) {
		type = number_type_double;
		for (token = get_token(); token == token_type_digit0_9; token = get_token());
	}

	// exp = e [ minus / plus ] 1*DIGIT
	if (token == token_type_e) {
		type = number_type_exp;
		token = get_token();
		if (token == token_type_minus || token == token_type_plus) {
			token = get_token();
		}
		for (; token == token_type_digit0_9; token = get_token());
	}
	size_t length = lexer.pos - begin - 1;
	char *number = calloc(length + 1, sizeof(char));
	strncpy_s(number, length + 1, begin, length);
	return make_number(number, type);
}

static void print_tokens(const char *json_text) {
	set_lexer(json_text);
	while (get_token() != token_type_end_of_text) {
		printf("%-15s\n", token_type_to_string(current_token.type));
	}
}

static int indent = 0;

static void kzrjson_print_inner(kzrjson_inner data) {
	if (data == NULL) return;
	switch (data->type) {
	case json_type_array: {
		printf("array: size %zd\n", data->elements_size);
		for (int i = 0; i < data->elements_size; i++) {
			kzrjson_print_inner(*(data->elements + i));
		}
		break;
	}
	case json_type_object: {
		printf("object: size %zd\n", data->elements_size);
		for (int i = 0; i < data->elements_size; i++) {
			kzrjson_print_inner(*(data->elements + i));
		}
		break;
	}
	case json_type_member:
		printf("member\n");
		printf("key: %s\n", data->member_key);
		kzrjson_print_inner(data->member_value);
		break;
	case json_type_string:
	case json_type_number:
	case json_type_boolean:
	case json_type_null:
		printf("%s: %s\n", json_type_to_string(data->type), data->value);
		break;
	}
}

static void kzrjson_inner_free(kzrjson_inner inner) {
	if (inner == NULL) return;
	switch (inner->type) {
	case json_type_array:
	case json_type_object:
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_free(*(inner->elements + i));
		}
		free(inner->elements);
		break;
	case json_type_member:
		free(inner->member_key);
		kzrjson_inner_free(inner->member_value);
		break;
	case json_type_string:
		free(inner->value);
		break;
	case json_type_number:
		free(inner->value);
		break;
	case json_type_boolean:
	case json_type_null:
		break;
	}
	free(inner);
}

static kzrjson_t null_data = {
	.inner = NULL
};

// interface
void kzrjson_print(kzrjson_t data) {
	indent = 0;
	kzrjson_print_inner(data.inner);
	indent = 0;
}

void kzrjson_free(kzrjson_t data) {
	if (data.inner == NULL) return;
	kzrjson_inner_free(data.inner);
}

kzrjson_t kzrjson_parse(const char *json_text) {
	set_lexer(json_text);
	kzrjson_t data;
	data.inner = parse_json_text();
	return data;
}

bool kzrjson_is_object(kzrjson_t data) {
	return data.inner->type == json_type_object;
}

bool kzrjson_is_array(kzrjson_t data) {
	return data.inner->type == json_type_array;
}

bool kzrjson_is_string(kzrjson_t data) {
	return data.inner->type == json_type_string;
}

bool kzrjson_is_member(kzrjson_t data) {
	return data.inner->type == json_type_member;
}

bool kzrjson_is_number(kzrjson_t data) {
	return data.inner->type == json_type_number;
}

bool kzrjson_is_boolean(kzrjson_t data) {
	return data.inner->type == json_type_boolean;
}

bool kzrjson_is_null(kzrjson_t data) {
	return data.inner->type == json_type_null;
}

bool kzrjson_error_occured(kzrjson_t result) {
	return result.inner == NULL;
}

size_t kzrjson_object_size(kzrjson_t object) {
	return kzrjson_is_object(object) ? object.inner->elements_size : 0;
}

kzrjson_t kzrjson_get_member(kzrjson_t object, const char *key) {
	if (!kzrjson_is_object(object)) return null_data;
	for (int i = 0; i < object.inner->elements_size; i++) {
		kzrjson_inner member = *(object.inner->elements + i);
		if (strcmp(key, member->member_key) == 0) {
			kzrjson_t data = {
				.inner = member
			};
			return data;
		}
	}
	return null_data;
}

const char *kzrjson_get_member_key(kzrjson_t member) {
	if (!kzrjson_is_member(member)) return NULL;
	return member.inner->member_key;
}

kzrjson_t kzrjson_get_value_from_member(kzrjson_t member) {
	if (!kzrjson_is_member(member)) return null_data;
	kzrjson_t data = {
		.inner = member.inner->member_value
	};
	return data;
}

kzrjson_t kzrjson_get_value_from_key(kzrjson_t object, const char *key) {
	if (!kzrjson_is_object(object)) return null_data;
	kzrjson_t member = kzrjson_get_member(object, key);
	if (kzrjson_error_occured(member)) return null_data;
	return kzrjson_get_value_from_member(member);
}

size_t kzrjson_array_size(kzrjson_t array) {
	if (!kzrjson_is_array(array)) return 0;
	return array.inner->elements_size;
}

kzrjson_t kzrjson_get_element(kzrjson_t array, size_t index) {
	if (!kzrjson_is_array(array)) return null_data;
	if (index < 0 || index >= array.inner->elements_size) return null_data;
	kzrjson_t element = {
		.inner = *(array.inner->elements + index)
	};
	return element;
}

const char *kzrjson_get_string(kzrjson_t string) {
	if (!kzrjson_is_string(string)) return NULL;
	return string.inner->value;
}

bool kzrjson_get_boolean(kzrjson_t boolean) {
	if (!kzrjson_is_boolean(boolean)) return false;
	return strcmp(boolean.inner->value, literal_true) == 0;
}

const char *kzrjson_get_number_as_string(kzrjson_t number) {
	if (!kzrjson_is_number(number)) return NULL;
	return number.inner->value;
}

int64_t kzrjson_get_number_as_integer(kzrjson_t number) {
	if (!kzrjson_is_number(number)) return 0;
	return number.inner->number_integer;
}

uint64_t kzrjson_get_number_as_unsigned_integer(kzrjson_t number) {
	if (!kzrjson_is_number(number)) return 0;
	return number.inner->number_unsigned_integer;
}

double kzrjson_get_number_as_double(kzrjson_t number) {
	if (!kzrjson_is_number(number)) return 0;
	return number.inner->number_double;
}

kzrjson_t make_kzrjson(kzrjson_inner inner) {
	kzrjson_t data = {
		.inner = inner
	};
	return data;
}

kzrjson_t kzrjson_make_object(void) {
	return make_kzrjson(make_object());
}

kzrjson_t kzrjson_make_array(void) {
	return make_kzrjson(make_array());
}

void kzrjson_object_add_member(kzrjson_t *object, kzrjson_t member) {
	if (!kzrjson_is_object(*object)) return;
	add_element(object->inner, member.inner);
}

void kzrjson_array_add_element(kzrjson_t *array, kzrjson_t element) {
	if (!kzrjson_is_array(*array)) return;
	add_element(array->inner, element.inner);
}

kzrjson_t kzrjson_make_member(const char *key, const size_t length, kzrjson_t value) {
	char *buffer = calloc(length + 1, sizeof(char));
	strncpy_s(buffer, length + 1, key, length);
	kzrjson_t member = make_kzrjson(make_member(buffer));
	add_value(member.inner, value.inner);
	return member;
}

kzrjson_t kzrjson_make_string(const char *string, const size_t length) {
	char *buffer = calloc(length + 1, sizeof(char));
	strncpy_s(buffer, length + 1, string, length);
	return make_kzrjson(make_string(buffer));
}

kzrjson_t kzrjson_make_boolean(const bool boolean) {
	return make_kzrjson(make_boolean(boolean));
}

kzrjson_t kzrjson_make_null() {
	return make_kzrjson(make_null());
}

kzrjson_t kzrjson_make_number_double(const double number) {
	static const size_t double_max_digit = 16;
	char *buffer = calloc(double_max_digit + 1, sizeof(char));
	snprintf(buffer, double_max_digit + 1, "%lf", number);
	return make_kzrjson(make_number(buffer, number_type_double));
}

kzrjson_t kzrjson_make_number_unsigned_integer(const uint64_t number) {
	static const size_t uint64_max_digit = 20;
	char *buffer = calloc(uint64_max_digit + 1, sizeof(char));
	snprintf(buffer, uint64_max_digit + 1, "%llu", number);
	return make_kzrjson(make_number(buffer, number_type_unsigned_integer));
}

kzrjson_t kzrjson_make_number_integer(const int64_t number) {
	static const size_t int64_t_max_digit = 19;
	char *buffer = calloc(int64_t_max_digit + 1, sizeof(char));
	snprintf(buffer, int64_t_max_digit + 1, "%lld", number);
	return make_kzrjson(make_number(buffer, number_type_integer));
}

static struct {
	size_t length;
	char *begin;
	char *pos;
} g_converter;

static size_t kzrjson_inner_length(kzrjson_inner inner) {
	switch (inner->type) {
	case json_type_object:
		g_converter.length++; // begin-object
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_length(*(inner->elements + i));
			if (i + 1 != inner->elements_size) {
				g_converter.length++; // value-separator
			}
		}
		g_converter.length++; // end-object
		break;
	case json_type_array:
		g_converter.length++; // begin-array
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_length(*(inner->elements + i));
			if (i + 1 != inner->elements_size) {
				g_converter.length++; // value-separator
			}
		}
		g_converter.length++; // end-array
		break;
	case json_type_member:
		g_converter.length++; // quatation-mark
		g_converter.length += strlen(inner->member_key);
		g_converter.length++; // quatation-mark
		g_converter.length++; // name-separator
		kzrjson_inner_length(inner->member_value);
		break;
	case json_type_string:
		g_converter.length++; // quatation-mark
		g_converter.length += strlen(inner->value);
		g_converter.length++; // quatation-mark
		break;
	case json_type_number:
		g_converter.length += strlen(inner->value);
		break;
	case json_type_boolean:
		g_converter.length += strlen(inner->value);
		break;
	case json_type_null:
		g_converter.length += strlen(inner->value);
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

static void kzrjson_inner_to_string(kzrjson_inner inner) {
	switch (inner->type) {
	case json_type_object:
		converter_add_char(begin_object);
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_to_string(*(inner->elements + i));
			if (i + 1 != inner->elements_size) {
				converter_add_char(value_separator);
			}
		}
		converter_add_char(end_object);
		break;
	case json_type_array:
		converter_add_char(begin_array);
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_to_string(*(inner->elements + i));
			if (i + 1 != inner->elements_size) {
				converter_add_char(value_separator);
			}
		}
		converter_add_char(end_array);
		break;
	case json_type_member:
		converter_add_char(quotation_mark);
		converter_add_string(inner->member_key);
		converter_add_char(quotation_mark);
		converter_add_char(name_separator);
		kzrjson_inner_to_string(inner->member_value);
		break;
	case json_type_string:
		converter_add_char(quotation_mark);
		converter_add_string(inner->value);
		converter_add_char(quotation_mark);
		break;
	case json_type_number:
		converter_add_string(inner->value);
		break;
	case json_type_boolean:
		converter_add_string(inner->value);
		break;
	case json_type_null:
		converter_add_string(inner->value);
		break;
	}
}

kzrjson_text_t kzrjson_to_string(kzrjson_t data) {
	g_converter.length = 0;

	kzrjson_text_t json;
	json.length = kzrjson_inner_length(data.inner);
	g_converter.begin = calloc(g_converter.length + 1, sizeof(char));
	g_converter.pos = g_converter.begin;
	kzrjson_inner_to_string(data.inner);
	json.text = g_converter.begin;

	g_converter.length = 0;

	return json;
}
