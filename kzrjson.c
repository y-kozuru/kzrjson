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
	kzrjson_token_type_begin_array,
	kzrjson_token_type_begin_object,
	kzrjson_token_type_end_array,
	kzrjson_token_type_end_object,
	kzrjson_token_type_name_separator,
	kzrjson_token_type_value_separator,
	kzrjson_token_type_white_space,
	kzrjson_token_type_literal_false,
	kzrjson_token_type_null,
	kzrjson_token_type_literal_true,
	kzrjson_token_type_decimal_point,
	kzrjson_token_type_digit0_9,
	kzrjson_token_type_e,
	kzrjson_token_type_minus,
	kzrjson_token_type_plus,
	kzrjson_token_type_zero,
	kzrjson_token_type_escape,
	kzrjson_token_type_quotation_mark,
	kzrjson_token_type_string,
	kzrjson_token_type_end_of_text,
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

// no error
static void set_lexer(const char *json_text) {
	lexer.text = json_text;
	lexer.pos = lexer.text;
}

// no error
static void next() {
	lexer.pos++;
}

// no error
static bool end_of_text() {
	return *lexer.pos == '\0';
}

// no error
static kzrjson_token_type set_token(kzrjson_token_type type, const char *begin, const size_t length) {
	current_token.type = type;
	current_token.begin = begin;
	current_token.length = length;
	return type;
}

// no error
static kzrjson_token_type set_token_char(kzrjson_token_type type) {
	return set_token(type, current_token.begin, 1);
}

// no error
static kzrjson_token_type set_token_eot() {
	return set_token(kzrjson_token_type_end_of_text, NULL, 0);
}

// no error
static bool consume_if(const char c) {
	if (*lexer.pos == c) {
		next();
		return true;
	}
	return false;
}

// no error
static bool contained(const char *chars) {
	return strchr(chars, *lexer.pos) != NULL;
}

// no error
static bool consume_if_contained(const char *chars) {
	if (contained(chars)) {
		next();
		return true;
	}
	return false;
}

// no error
static bool consume_literal(const char *literal) {
	if (strstr(lexer.pos, literal) == lexer.pos) {
		lexer.pos += strlen(literal);
		return true;
	}
	return false;
}

static kzrjson_exception_type g_error_type = kzrjson_success;

// no error
static void set_error(kzrjson_exception_type type) {
	g_error_type = type;
}

// no error
bool kzrjson_catch_exception(void) {
	return g_error_type != kzrjson_success;
}

/*
 * [use error]
 *   kzrjson_exception_tokenize
 */
static kzrjson_token_type set_token_string() {
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
			set_error(kzrjson_exception_tokenize);
			return 0;
		}
		length++;
	}
	next(); // consume_if quotation_mark
	return set_token(kzrjson_token_type_string, begin, length);
}

// トークンを一つ読み込み、current_tokenにセットし、current_tokenのタイプを返す。
// [error] kzrjson_exception_tokenize
static kzrjson_token_type get_token() {
	if (end_of_text()) return set_token_eot();
	while (consume_if_contained(white_spaces)) {
		if (end_of_text()) return set_token_eot();
	}
	if (consume_if(begin_array)) {
		return set_token_char(kzrjson_token_type_begin_array);
	} else if (consume_if(begin_object)) {
		return set_token_char(kzrjson_token_type_begin_object);
	} else if (consume_if(end_array)) {
		return set_token_char(kzrjson_token_type_end_array);
	} else if (consume_if(end_object)) {
		return set_token_char(kzrjson_token_type_end_object);
	} else if (consume_if(name_separator)) {
		return set_token_char(kzrjson_token_type_name_separator);
	} else if (consume_if(value_separator)) {
		return set_token_char(kzrjson_token_type_value_separator);
	} else if (consume_if(decimal_point)) {
		return set_token_char(kzrjson_token_type_decimal_point);
	} else if (contained(digit0_9)) {
		set_token_char(kzrjson_token_type_digit0_9);
		next();
		return kzrjson_token_type_digit0_9;
	} else if (consume_if_contained(e)) {
		set_token_char(kzrjson_token_type_e);
		next();
		return kzrjson_token_type_e;
	} else if (consume_if(minus)) {
		return set_token_char(kzrjson_token_type_minus);
	} else if (consume_if(plus)) {
		return set_token_char(kzrjson_token_type_plus);
	} else if (consume_if(escape)) {
		return set_token_char(kzrjson_token_type_escape);
	} else if (consume_if(quotation_mark)) {
		const kzrjson_token_type type = set_token_string();
		if (kzrjson_catch_exception()) return 0;
		return type;
	} else {
		if (consume_literal(literal_false)) {
			return set_token(kzrjson_token_type_literal_false, literal_false, strlen(literal_false));
		} else if (consume_literal(literal_true)) {
			return set_token(kzrjson_token_type_literal_true, literal_true, strlen(literal_true));
		} else if (consume_literal(literal_null)) {
			return set_token(kzrjson_token_type_null, literal_null, strlen(literal_null));
		} else {
			set_error(kzrjson_exception_tokenize);
			return 0;
		}
	}
}

typedef enum {
	kzrjson_json_type_object,
	kzrjson_json_type_array,
	kzrjson_json_type_string,
	kzrjson_json_type_number,
	kzrjson_json_type_boolean,
	kzrjson_json_type_null,
	kzrjson_json_type_member,
	kzrjson_json_type_end_of_elements,
} kzrjson_json_type;

typedef enum {
	kzrjson_number_type_integer,
	kzrjson_number_type_unsigned_integer,
	kzrjson_number_type_double,
	kzrjson_number_type_exp,
} kzrjson_number_type;

typedef struct kzrjson_inner *kzrjson_inner;
struct kzrjson_inner {
	kzrjson_json_type type;
	kzrjson_inner *elements; // object array
	size_t elements_size;
	kzrjson_inner member_value; // member
	char *member_key;
	char *value; // string number true false null

	kzrjson_number_type number_type;
	union {
		int64_t number_integer;
		uint64_t number_unsigned_integer;
		double number_double;
	};
};

// [error] kzrjson_exception_failed_to_allocate_memory
static void add_element(kzrjson_inner data, kzrjson_inner element) {
	if (data == NULL) return;
	if (element == NULL) return;
	data->elements_size++;
	if (data->elements_size == 1) {
		data->elements = calloc(1, sizeof(struct kzrjson_inner));
		if (data->elements == NULL) goto error_occurred;
		*data->elements = element;
	} else {
		data->elements = realloc(data->elements, data->elements_size * sizeof(struct kzrjson_inner));
		if (data->elements == NULL) goto error_occurred;
		*(data->elements + data->elements_size - 1) = element;
	}
error_occurred:
	set_error(kzrjson_exception_failed_to_allocate_memory);
}

static void add_value(kzrjson_inner data, kzrjson_inner value) {
	if (data == NULL) return;
	if (value == NULL) return;
	data->member_value = value;
}

// [error] kzrjson_exception_failed_to_allocate_memory
static kzrjson_inner make_json_data(const kzrjson_json_type type, char *string) {
	kzrjson_inner data = calloc(1, sizeof(struct kzrjson_inner));
	if (data == NULL) {
		set_error(kzrjson_exception_failed_to_allocate_memory);
		return NULL;
	}
	data->type = type;
	data->value = string;
	data->elements = NULL;
	data->elements_size = 0;
	data->member_key = NULL;
	return data;
}

// [error] kzrjson_exception_failed_to_allocate_memory
static kzrjson_inner make_string(const char *string) {
	return make_json_data(kzrjson_json_type_string, (char *)string);
}

// [error] kzrjson_exception_not_number
static kzrjson_inner make_number(char *number, kzrjson_number_type type) {
	kzrjson_inner data = make_json_data(kzrjson_json_type_number, number);
	if (data == NULL) {
		set_error(kzrjson_exception_failed_to_allocate_memory);
		return NULL;
	}
	data->number_type = type;
	char *end;
	switch (data->number_type) {
	case kzrjson_number_type_integer:
		data->number_integer = strtoll(data->value, &end, 10);
		if (data->value == end) goto error_occurred;
		break;
	case kzrjson_number_type_unsigned_integer:
		data->number_unsigned_integer = strtoull(data->value, &end, 10);
		if (data->value == end) goto error_occurred;
		break;
	case kzrjson_number_type_double:
	case kzrjson_number_type_exp:
		data->number_double = strtod(data->value, &end);
		if (data->value == end) goto error_occurred;
		break;
	}
	return data;

error_occurred:
	set_error(kzrjson_exception_not_number);
	return NULL;
}

// [error] kzrjson_exception_failed_to_allocate_memory
static kzrjson_inner make_boolean(const bool boolean) {
	return make_json_data(kzrjson_json_type_boolean, (char *)(boolean ? literal_true : literal_false));
}

// [error] kzrjson_exception_failed_to_allocate_memory
static kzrjson_inner make_null() {
	return make_json_data(kzrjson_json_type_null, (char *)literal_null);
}

// [error] kzrjson_exception_failed_to_allocate_memory
static kzrjson_inner make_object() {
	return make_json_data(kzrjson_json_type_object, NULL);
}

// [error] kzrjson_exception_failed_to_allocate_memory
static kzrjson_inner make_array() {
	return make_json_data(kzrjson_json_type_array, NULL);
}

// [error] kzrjson_exception_failed_to_allocate_memory
static kzrjson_inner make_member(const char *key) {
	kzrjson_inner data = make_json_data(kzrjson_json_type_member, NULL);
	if (kzrjson_catch_exception()) return NULL;
	data->member_key = (char *)key;
	return data;
}

static kzrjson_inner parse_object();
static kzrjson_inner parse_array();
static kzrjson_inner parse_number();
static kzrjson_inner parse_member();
static kzrjson_inner parse_value();

static bool current_is(const kzrjson_token_type type) {
	return current_token.type == type;
}

static void current_must(const kzrjson_token_type type) {
	if (!current_is(type)) {
		set_error(kzrjson_exception_parse);
		return;
	}
}

static char *copy_string(const char *from, const size_t length) {
	char *buffer = calloc(length + 1, sizeof(char));
	strncpy_s(buffer, length + 1, from, length);
	return buffer;
}

static kzrjson_inner parse_json_text() {
	get_token();
	if (kzrjson_catch_exception()) return NULL;
	return parse_value();
}

// value = false / null / true / string / object / array / number
static kzrjson_inner parse_value() {
	if (current_is(kzrjson_token_type_literal_false)) {
		kzrjson_inner data = make_boolean(false);
		if (kzrjson_catch_exception()) goto error_occurred;
		get_token();
		if (kzrjson_catch_exception()) goto error_occurred;
		return data;
	} else if (current_is(kzrjson_token_type_literal_true)) {
		kzrjson_inner data = make_boolean(true);
		if (kzrjson_catch_exception()) goto error_occurred;
		get_token();
		if (kzrjson_catch_exception()) goto error_occurred;
		return data;
	} else if (current_is(kzrjson_token_type_null)) {
		kzrjson_inner data = make_null();
		if (kzrjson_catch_exception()) goto error_occurred;
		get_token();
		if (kzrjson_catch_exception()) goto error_occurred;
		return data;
	} else if (current_is(kzrjson_token_type_string)) {
		char *string = copy_string(current_token.begin, current_token.length);
		if (kzrjson_catch_exception()) goto error_occurred;
		kzrjson_inner data = make_string(string);
		if (kzrjson_catch_exception()) goto error_occurred;
		get_token();
		if (kzrjson_catch_exception()) goto error_occurred;
		return data;
	} else if (current_is(kzrjson_token_type_begin_object)) {
		return parse_object();
	} else if (current_is(kzrjson_token_type_begin_array)) {
		return parse_array();
	} else {
		return parse_number();
	}
error_occurred:
	return NULL;
}

// object = begin-object [ member *( value-separator member ) ] end-object
// [error] kzrjson_exception_failed_to_allocate_memory
static kzrjson_inner parse_object() {
	kzrjson_inner object = make_object();
	if (kzrjson_catch_exception()) return NULL;
	current_must(kzrjson_token_type_begin_object);
	if (kzrjson_catch_exception()) return NULL;
	get_token();
	if (kzrjson_catch_exception()) return NULL;
	add_element(object, parse_member());
	while (!current_is(kzrjson_token_type_end_object)) {
		current_must(kzrjson_token_type_value_separator);
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
static kzrjson_inner parse_array() {
	kzrjson_inner array = make_array();
	if (kzrjson_catch_exception()) return NULL;
	current_must(kzrjson_token_type_begin_array);
	if (kzrjson_catch_exception()) return NULL;
	get_token();
	if (kzrjson_catch_exception()) return NULL;
	add_element(array, parse_value());
	while (!current_is(kzrjson_token_type_end_array)) {
		current_must(kzrjson_token_type_value_separator);
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
static kzrjson_inner parse_member() {
	current_must(kzrjson_token_type_string);
	if (kzrjson_catch_exception()) return NULL;
	char *buffer = copy_string(current_token.begin, current_token.length);
	kzrjson_inner member = make_member(buffer);
	if (kzrjson_catch_exception()) return NULL;
	get_token();
	if (kzrjson_catch_exception()) return NULL;
	current_must(kzrjson_token_type_name_separator);
	if (kzrjson_catch_exception()) return NULL;
	get_token();
	if (kzrjson_catch_exception()) return NULL;
	add_value(member, parse_value());
	return member;
}

// number = [ minus ] int [ frac ] [ exp ]
static kzrjson_inner parse_number() {
	const char *begin = lexer.pos - 1;
	kzrjson_token_type token = current_token.type;
	kzrjson_number_type type = kzrjson_number_type_unsigned_integer;
	if (token == kzrjson_token_type_minus) {
		type = kzrjson_number_type_integer;
		token = get_token();
		if (kzrjson_catch_exception()) return NULL;
	}

	// int = zero / ( digit1-9 *DIGIT )
	for (; token == kzrjson_token_type_digit0_9; token = get_token()) {
		if (kzrjson_catch_exception()) return NULL;
	}

	// frac = decimal-point 1*DIGIT
	if (token == kzrjson_token_type_decimal_point) {
		type = kzrjson_number_type_double;
		for (token = get_token(); token == kzrjson_token_type_digit0_9; token = get_token()) {
			if (kzrjson_catch_exception()) return NULL;
		}
	}

	// exp = e [ minus / plus ] 1*DIGIT
	if (token == kzrjson_token_type_e) {
		type = kzrjson_number_type_exp;
		token = get_token();
		if (token == kzrjson_token_type_minus || token == kzrjson_token_type_plus) {
			token = get_token();
			if (kzrjson_catch_exception()) return NULL;
		}
		for (; token == kzrjson_token_type_digit0_9; token = get_token()) {
			if (kzrjson_catch_exception()) return NULL;
		}
	}
	size_t length = lexer.pos - begin - 1;
	char *number = copy_string(begin, length);
	if (number == NULL) {
		set_error(kzrjson_exception_failed_to_allocate_memory);
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

static void kzrjson_inner_print(kzrjson_inner inner) {
	if (inner == NULL) return;
	switch (inner->type) {
	case kzrjson_json_type_object: {
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
	case kzrjson_json_type_array: {
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
	case kzrjson_json_type_member:
		printf("%c%s%c%c ", quotation_mark, inner->member_key, quotation_mark, name_separator);
		kzrjson_inner_print(inner->member_value);
		break;
	case kzrjson_json_type_string:
		printf("%c%s%c", quotation_mark, inner->value, quotation_mark);
		break;
	case kzrjson_json_type_number:
	case kzrjson_json_type_boolean:
	case kzrjson_json_type_null:
		printf("%s", inner->value);
		break;
	}
}

static void kzrjson_inner_free(kzrjson_inner inner) {
	if (inner == NULL) return;
	switch (inner->type) {
	case kzrjson_json_type_array:
	case kzrjson_json_type_object:
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_free(*(inner->elements + i));
		}
		free(inner->elements);
		break;
	case kzrjson_json_type_member:
		free(inner->member_key);
		kzrjson_inner_free(inner->member_value);
		break;
	case kzrjson_json_type_string:
		free(inner->value);
		break;
	case kzrjson_json_type_number:
		free(inner->value);
		break;
	case kzrjson_json_type_boolean:
	case kzrjson_json_type_null:
		break;
	}
	free(inner);
}

static kzrjson_t kzrjson_null = {
	.inner = NULL
};

static struct {
	size_t length;
	char *begin;
	char *pos;
} g_converter;

static size_t kzrjson_inner_length(kzrjson_inner inner) {
	switch (inner->type) {
	case kzrjson_json_type_object:
		g_converter.length++; // begin-object
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_length(*(inner->elements + i));
			if (i + 1 != inner->elements_size) {
				g_converter.length++; // value-separator
			}
		}
		g_converter.length++; // end-object
		break;
	case kzrjson_json_type_array:
		g_converter.length++; // begin-array
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_length(*(inner->elements + i));
			if (i + 1 != inner->elements_size) {
				g_converter.length++; // value-separator
			}
		}
		g_converter.length++; // end-array
		break;
	case kzrjson_json_type_member:
		g_converter.length++; // quatation-mark
		g_converter.length += strlen(inner->member_key);
		g_converter.length++; // quatation-mark
		g_converter.length++; // name-separator
		kzrjson_inner_length(inner->member_value);
		break;
	case kzrjson_json_type_string:
		g_converter.length++; // quatation-mark
		g_converter.length += strlen(inner->value);
		g_converter.length++; // quatation-mark
		break;
	case kzrjson_json_type_number:
		g_converter.length += strlen(inner->value);
		break;
	case kzrjson_json_type_boolean:
		g_converter.length += strlen(inner->value);
		break;
	case kzrjson_json_type_null:
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
	case kzrjson_json_type_object:
		converter_add_char(begin_object);
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_to_string(*(inner->elements + i));
			if (i + 1 != inner->elements_size) {
				converter_add_char(value_separator);
			}
		}
		converter_add_char(end_object);
		break;
	case kzrjson_json_type_array:
		converter_add_char(begin_array);
		for (int i = 0; i < inner->elements_size; i++) {
			kzrjson_inner_to_string(*(inner->elements + i));
			if (i + 1 != inner->elements_size) {
				converter_add_char(value_separator);
			}
		}
		converter_add_char(end_array);
		break;
	case kzrjson_json_type_member:
		converter_add_char(quotation_mark);
		converter_add_string(inner->member_key);
		converter_add_char(quotation_mark);
		converter_add_char(name_separator);
		kzrjson_inner_to_string(inner->member_value);
		break;
	case kzrjson_json_type_string:
		converter_add_char(quotation_mark);
		converter_add_string(inner->value);
		converter_add_char(quotation_mark);
		break;
	case kzrjson_json_type_number:
		converter_add_string(inner->value);
		break;
	case kzrjson_json_type_boolean:
		converter_add_string(inner->value);
		break;
	case kzrjson_json_type_null:
		converter_add_string(inner->value);
		break;
	}
}

static kzrjson_t make_kzrjson(kzrjson_inner inner) {
	kzrjson_t data = {
		.inner = inner
	};
	return data;
}


/*
 * Interfaces
 */
void kzrjson_print(kzrjson_t data) {
	set_error(kzrjson_success);
	g_indent = 0;
	kzrjson_inner_print(data.inner);
	printf("\n");
	g_indent = 0;
}

void kzrjson_free(kzrjson_t data) {
	set_error(kzrjson_success);
	if (data.inner == NULL) return;
	kzrjson_inner_free(data.inner);
}

// [error] kzrjson_exception_tokenize
// [error] kzrjson_exception_parse
// [error] kzrjson_exception_failed_to_allocate_memory
kzrjson_t kzrjson_parse(const char *json_text) {
	set_error(kzrjson_success);
	set_lexer(json_text);
	kzrjson_t data;
	data.inner = parse_json_text();
	if (kzrjson_catch_exception()) {
		kzrjson_free(data);
		return kzrjson_null;
	}
	return data;
}

bool kzrjson_is_object(kzrjson_t data) {
	if (data.inner == NULL) return false;
	return data.inner->type == kzrjson_json_type_object;
}

bool kzrjson_is_array(kzrjson_t data) {
	if (data.inner == NULL) return false;
	return data.inner->type == kzrjson_json_type_array;
}

bool kzrjson_is_string(kzrjson_t data) {
	if (data.inner == NULL) return false;
	return data.inner->type == kzrjson_json_type_string;
}

bool kzrjson_is_member(kzrjson_t data) {
	if (data.inner == NULL) return false;
	return data.inner->type == kzrjson_json_type_member;
}

bool kzrjson_is_number(kzrjson_t data) {
	if (data.inner == NULL) return false;
	return data.inner->type == kzrjson_json_type_number;
}

bool kzrjson_is_boolean(kzrjson_t data) {
	if (data.inner == NULL) return false;
	return data.inner->type == kzrjson_json_type_boolean;
}

bool kzrjson_is_null(kzrjson_t data) {
	if (data.inner == NULL) return false;
	return data.inner->type == kzrjson_json_type_null;
}

// [error] kzrjson_exception_illegal_type
size_t kzrjson_object_size(kzrjson_t object) {
	if (!kzrjson_is_object(object)) {
		set_error(kzrjson_exception_illegal_type);
		return 0;
	}
	return object.inner->elements_size;
}

// [error] kzrjson_exception_illegal_type
// [error] kzrjson_exception_object_key_not_found
kzrjson_t kzrjson_get_member(kzrjson_t object, const char *key) {
	set_error(kzrjson_success);
	if (!kzrjson_is_object(object)) {
		set_error(kzrjson_exception_illegal_type);
		return kzrjson_null;
	}
	for (int i = 0; i < object.inner->elements_size; i++) {
		kzrjson_inner member = *(object.inner->elements + i);
		if (strcmp(key, member->member_key) == 0) {
			kzrjson_t data = {
				.inner = member
			};
			return data;
		}
	}
	set_error(kzrjson_exception_object_key_not_found);
	return kzrjson_null;
}

// [error] kzrjson_exception_illegal_type
const char *kzrjson_get_member_key(kzrjson_t member) {
	set_error(kzrjson_success);
	if (!kzrjson_is_member(member)) {
		set_error(kzrjson_exception_illegal_type);
		return NULL;
	}
	return member.inner->member_key;
}

// [error] kzrjson_exception_illegal_type
kzrjson_t kzrjson_get_value_from_member(kzrjson_t member) {
	set_error(kzrjson_success);
	if (!kzrjson_is_member(member)) {
		set_error(kzrjson_exception_illegal_type);
		return kzrjson_null;
	}
	kzrjson_t data = {
		.inner = member.inner->member_value
	};
	return data;
}

// [error] kzrjson_exception_illegal_type
// [error] kzrjson_exception_object_key_not_found
kzrjson_t kzrjson_get_value_from_key(kzrjson_t object, const char *key) {
	set_error(kzrjson_success);
	if (!kzrjson_is_object(object)) {
		set_error(kzrjson_exception_illegal_type);
		return kzrjson_null;
	}
	kzrjson_t member = kzrjson_get_member(object, key);
	if (kzrjson_exception_occured(member)) return kzrjson_null;
	return kzrjson_get_value_from_member(member);
}

// [error] kzrjson_exception_illegal_type
size_t kzrjson_array_size(kzrjson_t array) {
	set_error(kzrjson_success);
	if (!kzrjson_is_array(array)) {
		set_error(kzrjson_exception_illegal_type);
		return 0;
	}
	return array.inner->elements_size;
}

// [error] kzrjson_exception_illegal_type
// [error] kzrjson_exception_array_index_out_of_range
kzrjson_t kzrjson_get_element(kzrjson_t array, size_t index) {
	set_error(kzrjson_success);
	if (!kzrjson_is_array(array)) {
		set_error(kzrjson_exception_illegal_type);
		return kzrjson_null;
	}
	if (index < 0 || index >= array.inner->elements_size) {
		set_error(kzrjson_exception_array_index_out_of_range);
		return kzrjson_null;
	}
	kzrjson_t element = {
		.inner = *(array.inner->elements + index)
	};
	return element;
}

// [error] kzrjson_exception_illegal_type
const char *kzrjson_get_string(kzrjson_t string) {
	set_error(kzrjson_success);
	if (!kzrjson_is_string(string)) {
		set_error(kzrjson_exception_illegal_type);
		return NULL;
	}
	return string.inner->value;
}

// [error] kzrjson_exception_illegal_type
bool kzrjson_get_boolean(kzrjson_t boolean) {
	set_error(kzrjson_success);
	if (!kzrjson_is_boolean(boolean)) {
		set_error(kzrjson_exception_illegal_type);
		return false;
	}
	return strcmp(boolean.inner->value, literal_true) == 0;
}

// [error] kzrjson_exception_illegal_type
const char *kzrjson_get_number_as_string(kzrjson_t number) {
	set_error(kzrjson_success);
	if (!kzrjson_is_number(number)) {
		set_error(kzrjson_exception_illegal_type);
		return NULL;
	}
	return number.inner->value;
}

// [error] kzrjson_exception_illegal_type
int64_t kzrjson_get_number_as_integer(kzrjson_t number) {
	set_error(kzrjson_success);
	if (!kzrjson_is_number(number)) {
		set_error(kzrjson_exception_illegal_type);
		return 0;
	}
	return number.inner->number_integer;
}

// [error] kzrjson_exception_illegal_type
uint64_t kzrjson_get_number_as_unsigned_integer(kzrjson_t number) {
	set_error(kzrjson_success);
	if (!kzrjson_is_number(number)) {
		set_error(kzrjson_exception_illegal_type);
		return 0;
	}
	return number.inner->number_unsigned_integer;
}

// [error] kzrjson_exception_illegal_type
double kzrjson_get_number_as_double(kzrjson_t number) {
	set_error(kzrjson_success);
	if (!kzrjson_is_number(number)) {
		set_error(kzrjson_exception_illegal_type);
		return 0;
	}
	return number.inner->number_double;
}

// [error] kzrjson_exception_failed_to_allocate_memory
kzrjson_t kzrjson_make_object(void) {
	set_error(kzrjson_success);
	kzrjson_t json = make_kzrjson(make_object());
	if (kzrjson_catch_exception()) return kzrjson_null;
	return json;
}

// [error] kzrjson_exception_failed_to_allocate_memory
kzrjson_t kzrjson_make_array(void) {
	set_error(kzrjson_success);
	kzrjson_t json = make_kzrjson(make_array());
	if (kzrjson_catch_exception()) return kzrjson_null;
	return json;
}

// [error] kzrjson_exception_illegal_type
void kzrjson_object_add_member(kzrjson_t *object, kzrjson_t member) {
	set_error(kzrjson_success);
	if (!kzrjson_is_object(*object)) {
		set_error(kzrjson_exception_illegal_type);
		return;
	}
	if (!kzrjson_is_member(member)) {
		set_error(kzrjson_exception_illegal_type);
		return;
	}
	add_element(object->inner, member.inner);
}

// [error] kzrjson_exception_illegal_type
void kzrjson_array_add_element(kzrjson_t *array, kzrjson_t element) {
	set_error(kzrjson_success);
	if (!kzrjson_is_array(*array)) {
		set_error(kzrjson_exception_illegal_type);
		return;
	}
	if (kzrjson_is_member(element)) {
		set_error(kzrjson_exception_illegal_type);
		return;
	}
	add_element(array->inner, element.inner);
}

// [error] kzrjson_exception_failed_to_allocate_memory
kzrjson_t kzrjson_make_member(const char *key, const size_t length, kzrjson_t value) {
	set_error(kzrjson_success);

	char *buffer = copy_string(key, length);
	if (kzrjson_catch_exception()) return kzrjson_null;

	kzrjson_t member = make_kzrjson(make_member(buffer));
	if (kzrjson_catch_exception()) return kzrjson_null;

	add_value(member.inner, value.inner);
	return member;
}

// [error] kzrjson_exception_failed_to_allocate_memory
kzrjson_t kzrjson_make_string(const char *string, const size_t length) {
	set_error(kzrjson_success);

	char *buffer = copy_string(string, length);
	if (kzrjson_catch_exception()) return kzrjson_null;

	kzrjson_t json = make_kzrjson(make_string(buffer));
	if (kzrjson_catch_exception()) return kzrjson_null;

	return json;
}

// [error] kzrjson_exception_failed_to_allocate_memory
kzrjson_t kzrjson_make_boolean(const bool boolean) {
	set_error(kzrjson_success);
	kzrjson_t json = make_kzrjson(make_boolean(boolean));
	if (kzrjson_catch_exception()) return kzrjson_null;
	return json;
}

// [error] kzrjson_exception_failed_to_allocate_memory
kzrjson_t kzrjson_make_null() {
	set_error(kzrjson_success);
	kzrjson_t json = make_kzrjson(make_null());
	if (kzrjson_catch_exception()) return kzrjson_null;
	return json;
}

// [error] kzrjson_exception_not_number
// [error] kzrjson_exception_failed_to_allocate_memory
kzrjson_t kzrjson_make_number_double(const double number) {
	set_error(kzrjson_success);
	static const size_t double_max_digit = 16;
	char *buffer = calloc(double_max_digit + 1, sizeof(char));
	if (buffer == NULL) {
		set_error(kzrjson_exception_failed_to_allocate_memory);
		return kzrjson_null;
	}
	snprintf(buffer, double_max_digit + 1, "%lf", number);
	kzrjson_t json = make_kzrjson(make_number(buffer, kzrjson_number_type_double));
	if (kzrjson_catch_exception()) return kzrjson_null;
	return json;
}

// [error] kzrjson_exception_not_number
// [error] kzrjson_exception_failed_to_allocate_memory
kzrjson_t kzrjson_make_number_unsigned_integer(const uint64_t number) {
	set_error(kzrjson_success);
	static const size_t uint64_max_digit = 20;
	char *buffer = calloc(uint64_max_digit + 1, sizeof(char));
	if (buffer == NULL) {
		set_error(kzrjson_exception_failed_to_allocate_memory);
		return kzrjson_null;
	}
	snprintf(buffer, uint64_max_digit + 1, "%llu", number);
	kzrjson_t json = make_kzrjson(make_number(buffer, kzrjson_number_type_unsigned_integer));
	if (kzrjson_catch_exception()) return kzrjson_null;
	return json;
}

// [error] kzrjson_exception_not_number
// [error] kzrjson_exception_failed_to_allocate_memory
kzrjson_t kzrjson_make_number_integer(const int64_t number) {
	set_error(kzrjson_success);
	static const size_t int64_t_max_digit = 19;
	char *buffer = calloc(int64_t_max_digit + 1, sizeof(char));
	if (buffer == NULL) {
		set_error(kzrjson_exception_failed_to_allocate_memory);
		return kzrjson_null;
	}
	snprintf(buffer, int64_t_max_digit + 1, "%lld", number);
	kzrjson_t json = make_kzrjson(make_number(buffer, kzrjson_number_type_integer));
	if (kzrjson_catch_exception()) return kzrjson_null;
	return json;
}

// [error] kzrjson_exception_failed_to_allocate_memory
kzrjson_text_t kzrjson_to_string(kzrjson_t data) {
	set_error(kzrjson_success);
	g_converter.length = 0;

	kzrjson_text_t json;
	json.length = kzrjson_inner_length(data.inner);
	g_converter.begin = calloc(g_converter.length + 1, sizeof(char));
	if (g_converter.begin == NULL) {
		set_error(kzrjson_exception_failed_to_allocate_memory);
		kzrjson_text_t fail = {
			.text = NULL, 
			.length = 0
		};
		return fail;
	}
	g_converter.pos = g_converter.begin;
	kzrjson_inner_to_string(data.inner);
	json.text = g_converter.begin;

	g_converter.begin = NULL;
	g_converter.length = 0;

	return json;
}
