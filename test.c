#include "kzrjson.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *sample1 = "\
{\n \
	\"Image\": {\n\
		\"Width\":  800,\n \
		\"Height\": 600,\n \
		\"Title\":  \"View from \\\"15th Floor\\\"\",\n \
		\"Thumbnail\": {\n \
			\"Url\":    \"http://www.example.com/image/481989943\",\n \
			\"Height\": 125,\n \
			\"Width\":  100\n \
		},\n \
		\"Animated\" : false,\n \
		\"IDs\": [116, 943, 234, 38793]\n \
	}\n \
}\n";

// todo: add exception handling test.
static void test_parse_sample1(void) {
	kzrjson_t any = kzrjson_parse(sample1);
	if (kzrjson_catch_exception()) {
		puts("exception");
		printf("%d\n", kzrjson_exception());
	}
	assert(kzrjson_is_object(any));
	assert(kzrjson_object_size(any) == 1);

	kzrjson_t member_image = kzrjson_get_member(any, "Image");
	assert(kzrjson_is_member(member_image));

	kzrjson_t object = kzrjson_get_value_from_member(member_image);
	assert(kzrjson_is_object(object));
	assert(kzrjson_object_size(object) == 6);

	kzrjson_t member_title = kzrjson_get_value_from_key(object, "Title");
	assert(kzrjson_is_string(member_title));
	const char *member_title_string = kzrjson_get_string(member_title);
	assert(strcmp(member_title_string, "View from \\\"15th Floor\\\"") == 0);

	const bool member_animated = kzrjson_get_boolean(kzrjson_get_value_from_key(object, "Animated"));
	assert(member_animated == false);

	kzrjson_t array = kzrjson_get_value_from_key(object, "IDs");
	const uint64_t ids[] = {116, 943, 234, 38793};
	for (int i = 0; i < kzrjson_array_size(array); i++) {
		kzrjson_t element = kzrjson_get_element(array, i);
		assert(kzrjson_get_number_as_unsigned_integer(element) == ids[i]);
	}

	kzrjson_free(any);
	puts("test_parse_sample1 done");
}

static const char *sample2 = "\
[\n\
	{\n\
		\"precision\": \"zip\",\n\
		\"Latitude\":  37.7668,\n\
		\"Longitude\": -122.3959,\n\
		\"Address\":   \"\",\n\
		\"City\":      \"SAN FRANCISCO\",\n\
		\"State\":     \"CA\",\n\
		\"Zip\":       \"94107\",\n\
		\"Country\":   \"US\"\n\
	},\n\
	{\n\
		\"precision\": \"zip\",\n\
		\"Latitude\":  37.371991,\n\
		\"Longitude\": -122.026020,\n\
		\"Address\":   \"\",\n\
		\"City\":      \"SUNNYVALE\",\n\
		\"State\":     \"CA\",\n\
		\"Zip\":       \"94085\",\n\
		\"Country\":   \"US\"\n\
	}\n\
]";

static void test_parse_sample2(void) {
	kzrjson_t array = kzrjson_parse(sample2);

	kzrjson_t element1 = kzrjson_get_element(array, 0);
	const double element1_latitude = kzrjson_get_number_as_double(kzrjson_get_value_from_key(element1, "Latitude"));
	assert(element1_latitude == 37.7668);
	const double element1_longitude = kzrjson_get_number_as_double(kzrjson_get_value_from_key(element1, "Longitude"));
	assert(element1_longitude == -122.3959);

	kzrjson_t element2 = kzrjson_get_element(array, 1);
	const char *element2_latitude = kzrjson_get_number_as_string(kzrjson_get_value_from_key(element2, "Latitude"));
	assert(strcmp(element2_latitude, "37.371991") == 0);

	kzrjson_free(array);
	puts("test_parse_sample2 done");
}

static const char *sample3 = "[-1.3e+5, 6e-1]";
static void test_parse_sample3(void) {
	kzrjson_t array = kzrjson_parse(sample3);
	assert(kzrjson_get_number_as_double(kzrjson_get_element(array, 0)) == -130000);
	assert(kzrjson_get_number_as_double(kzrjson_get_element(array, 1)) == 0.6);
	kzrjson_free(array);
	puts("test_parse_sample3 done");
}

static void test_make_json(void) {
	kzrjson_t object = kzrjson_make_object();
	assert(kzrjson_is_object(object));
	assert(kzrjson_object_size(object) == 0);

	kzrjson_t value_boolean = kzrjson_make_boolean(true);
	assert(kzrjson_is_boolean(value_boolean));
	assert(kzrjson_get_boolean(value_boolean) == true);

	kzrjson_t member = kzrjson_make_member("member1", strlen("member1"), value_boolean);
	assert(kzrjson_is_member(member));
	assert(strcmp(kzrjson_get_member_key(member), "member1") == 0);
	assert(kzrjson_get_boolean(kzrjson_get_value_from_member(member)) == true);

	kzrjson_object_add_member(&object, member);
	assert(kzrjson_object_size(object) == 1);
	kzrjson_t value = kzrjson_get_value_from_key(object, "member1");
	assert(kzrjson_get_boolean(value) == true);

	kzrjson_t array = kzrjson_make_array();
	assert(kzrjson_is_array(array));
	assert(kzrjson_array_size(array) == 0);
	kzrjson_array_add_element(&array, kzrjson_make_number_integer(-100));
	kzrjson_array_add_element(&array, kzrjson_make_number_unsigned_integer(0));
	kzrjson_array_add_element(&array, kzrjson_make_number_double(0.5));
	kzrjson_array_add_element(&array, kzrjson_make_number_double(1.3e5));
	kzrjson_array_add_element(&array, kzrjson_make_string("sample", strlen("sample")));
	kzrjson_array_add_element(&array, kzrjson_make_null());
	assert(kzrjson_array_size(array) == 6);
	assert(kzrjson_get_number_as_integer(kzrjson_get_element(array, 0)) == -100);
	assert(kzrjson_get_number_as_unsigned_integer(kzrjson_get_element(array, 1)) == 0);
	assert(kzrjson_get_number_as_double(kzrjson_get_element(array, 2)) == 0.5);
	assert(kzrjson_get_number_as_double(kzrjson_get_element(array, 3)) == 1.3e5);
	assert(strcmp(kzrjson_get_string(kzrjson_get_element(array, 4)), "sample") == 0);
	assert(kzrjson_is_null(kzrjson_get_element(array, 5)));

	kzrjson_object_add_member(&object, kzrjson_make_member("array1", strlen("array1"), array));

	kzrjson_free(object);
	puts("test_make_json done");
}

static void test_kzrjson_to_string(void) {
	const char *text = "{\"member1\":100,\"member2\":[100,\"abc\",true],\"object\":{\"member2\":\"string\",\"member3\":null,\"member4\":-4.7}}";
	kzrjson_t json = kzrjson_parse(text);
	kzrjson_text_t json_text = kzrjson_to_string(json);
	assert(json_text.length == strlen(text));
	assert(strcmp(json_text.text, text) == 0);
	kzrjson_free(json);
	free(json_text.text);
	puts("test_kzrjson_to_string done");
}

static void test_kzrjson_print(void) {
	kzrjson_t json = kzrjson_parse(sample1);
	kzrjson_print(json);
	kzrjson_free(json);
	puts("test_kzrjson_print done");
}

int main(void) {
	test_parse_sample1();
	test_parse_sample2();
	test_parse_sample3();
	test_make_json();
	test_kzrjson_to_string();
	test_kzrjson_print();
	return 0;
}
