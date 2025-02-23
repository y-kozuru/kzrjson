#include "kzrjson.h"
#include <stdbool.h>
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
	char *string;
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

static token_type set_token(token_type type, char *string) {
	current_token.type = type;
	current_token.string = string;
	return type;
}

static token_type set_token_char(token_type type, const char c) {
	char *string = calloc(2, sizeof(char));
	*string = c;
	return set_token(type, string);
}

static token_type set_token_eot() {
	return set_token(token_type_end_of_text, NULL);
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
	const char *start = lexer.pos;
	int length = 0;
	for (; *lexer.pos != quotation_mark; next()) {
		if (end_of_text()) {
			fprintf_s(stderr, "failed to tokenize");
			exit(1);
		}
		length++;
	}
	next(); // consume_if quotation_mark
	char *string = calloc(length + 1, sizeof(char));
	strncpy_s(string, (length + 1) * sizeof(char), start, length);
	return set_token(token_type_string, string);
}

// トークンを一つ読み込み、current_tokenにセットし、current_tokenのタイプを返す。
static token_type get_token() {
	if (end_of_text()) return set_token_eot();
	while (consume_if_contained(white_spaces)) {
		if (end_of_text()) return set_token_eot();
	}
	if (consume_if(begin_array)) {
		return set_token_char(token_type_begin_array, begin_array);
	} else if (consume_if(begin_object)) {
		return set_token_char(token_type_begin_object, begin_object);
	} else if (consume_if(end_array)) {
		return set_token_char(token_type_end_array, end_array);
	} else if (consume_if(end_object)) {
		return set_token_char(token_type_end_object, end_object);
	} else if (consume_if(name_separator)) {
		return set_token_char(token_type_name_separator, name_separator);
	} else if (consume_if(value_separator)) {
		return set_token_char(token_type_value_separator, value_separator);
	} else if (consume_if(decimal_point)) {
		return set_token_char(token_type_decimal_point, decimal_point);
	} else if (contained(digit0_9)) {
		set_token_char(token_type_digit0_9, *lexer.pos);
		next();
		return token_type_digit0_9;
	} else if (consume_if_contained(e)) {
		set_token_char(token_type_e, *lexer.pos);
		next();
		return token_type_e;
	} else if (consume_if(minus)) {
		return set_token_char(token_type_minus, minus);
	} else if (consume_if(plus)) {
		return set_token_char(token_type_plus, plus);
	} else if (consume_if(escape)) {
		return set_token_char(token_type_escape, escape);
	} else if (consume_if(quotation_mark)) {
		return set_token_string();
	} else {
		if (consume_literal(literal_false)) {
			return set_token(token_type_literal_false, (char *)literal_false);
		} else if (consume_literal(literal_true)) {
			return set_token(token_type_literal_true, (char *)literal_true);
		} else if (consume_literal(literal_null)) {
			return set_token(token_type_null, (char *)literal_null);
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

typedef struct kzrjson_data_inner *kzrjson_data_inner;
struct kzrjson_data_inner {
	json_type type;
	kzrjson_data_inner *elements; // object array
	size_t elements_size;
	kzrjson_data_inner member_value; // member
	char *member_key;
	char *value; // string number true false null
};

static void add_element(kzrjson_data_inner data, kzrjson_data_inner element) {
	// todo: add element to elements last
	data->elements_size++;
	if (data->elements_size == 1) {
		data->elements = calloc(1, sizeof(struct kzrjson_data_inner));
		*data->elements = element;
	} else {
		data->elements = realloc(data->elements, data->elements_size * sizeof(struct kzrjson_data_inner));
		*(data->elements + data->elements_size - 1) = element;
	}
}

static void add_value(kzrjson_data_inner data, kzrjson_data_inner value) {
	data->member_value = value;
}

kzrjson_data_inner make_json_data(const json_type type, char *string) {
	kzrjson_data_inner data = calloc(1, sizeof(struct kzrjson_data_inner));
	data->type = type;
	data->value = string;
	data->elements = NULL;
	data->elements_size = 0;
	data->member_key = NULL;
	return data;
}

kzrjson_data_inner make_string() {
	return make_json_data(json_type_string, current_token.string);
}

kzrjson_data_inner make_number(char *number) {
	return make_json_data(json_type_number, number);
}

kzrjson_data_inner make_boolean() {
	return make_json_data(json_type_boolean, current_token.string);
}

kzrjson_data_inner make_null() {
	return make_json_data(json_type_null, (char *)literal_null);
}

kzrjson_data_inner make_object() {
	return make_json_data(json_type_object, "");
}

kzrjson_data_inner make_array() {
	return make_json_data(json_type_array, "");
}

kzrjson_data_inner make_member(const char *key) {
	kzrjson_data_inner data = make_json_data(json_type_member, "");
	data->member_key = (char *)key;
	return data;
}

static kzrjson_data_inner parse_object();
static kzrjson_data_inner parse_array();
static kzrjson_data_inner parse_number();
static kzrjson_data_inner parse_member();
static kzrjson_data_inner parse_value();

static bool current_is(const token_type type) {
	return current_token.type == type;
}

static void current_must(const token_type type) {
	if (!current_is(type)) {
		fprintf(stderr, "current must %s but %s %s\n", token_type_to_string(type), token_type_to_string(current_token.type), current_token.string);
		exit(1);
	}
}

static kzrjson_data_inner parse_json_text() {
	get_token();
	return parse_value();
}

// value = false / null / true / string / object / array / number
static kzrjson_data_inner parse_value() {
	if (current_is(token_type_literal_false) || current_is(token_type_literal_true)) {
		kzrjson_data_inner data = make_boolean(current_token.string);
		get_token();
		return data;
	} else if (current_is(token_type_null)) {
		kzrjson_data_inner data = make_null();
		get_token();
		return data;
	} else if (current_is(token_type_string)) {
		kzrjson_data_inner data = make_string(current_token.string);
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
static kzrjson_data_inner parse_object() {
	kzrjson_data_inner object = make_object();
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
static kzrjson_data_inner parse_array() {
	kzrjson_data_inner array = make_array();
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
static kzrjson_data_inner parse_member() {
	current_must(token_type_string);
	kzrjson_data_inner member = make_member(current_token.string);
	get_token();
	current_must(token_type_name_separator);
	get_token();
	add_value(member, parse_value());
	return member;
}

// number = [ minus ] int [ frac ] [ exp ]
static kzrjson_data_inner parse_number() {
	const char *begin = lexer.pos - 1;
	token_type token = current_token.type;
	if (token == token_type_minus) {
		token = get_token();
	}

	// int = zero / ( digit1-9 *DIGIT )
	for (; token == token_type_digit0_9; token = get_token());

	// frac = decimal-point 1*DIGIT
	if (token == token_type_decimal_point) {
		for (token = get_token(); token == token_type_digit0_9; token = get_token());
	}

	// exp = e [ minus / plus ] 1*DIGIT
	if (token == token_type_e) {
		token = get_token();
		if (token == token_type_minus || token == token_type_plus) {
			token = get_token();
		}
		for (; token == token_type_digit0_9; token = get_token());
	}
	size_t length = lexer.pos - begin - 1;
	char *number = calloc(length + 1, sizeof(char));
	strncpy_s(number, length + 1, begin, length);
	return make_number(number);
}

static void print_tokens(const char *json_text) {
	set_lexer(json_text);
	while (get_token() != token_type_end_of_text) {
		printf("%-15s %s\n", token_type_to_string(current_token.type), current_token.string);
	}
}

static int indent = 0;

static void kzrjson_print_inner(kzrjson_data_inner data) {
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

static void kzrjson_inner_free(kzrjson_data_inner inner) {
	if (inner == NULL) return;
	switch (inner->type) {
	case json_type_array:
	case json_type_object:
		free(inner->value);
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
	case json_type_number:
	case json_type_boolean:
	case json_type_null:
		free(inner->member_value);
		break;
	}
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
		kzrjson_data_inner member = *(object.inner->elements + i);
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

const char *kzrjson_get_number(kzrjson_t number) {
	if (!kzrjson_is_number(number)) return NULL;
	return number.inner->value;
}
