#include "kzrjson.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char *sample1 = "\
{\n \
	\"Image\": {\n\
		\"Width\":  800,\n \
		\"Height\": 600,\n \
		\"Title\":  \"View from 15th Floor\",\n \
		\"Thumbnail\": {\n \
			\"Url\":    \"http://www.example.com/image/481989943\",\n \
			\"Height\": 125,\n \
			\"Width\":  100\n \
		},\n \
		\"Animated\" : false,\n \
		\"IDs\": [116, 943, 234, 38793]\n \
	}\n \
}\n";

static void test_sample1(void) {
	kzrjson_t data = kzrjson_parse(sample1);
	assert(kzrjson_is_object(data));
	assert(kzrjson_object_size(data) == 1);

	kzrjson_t member_image = kzrjson_get_member(data, "Image");
	assert(!kzrjson_error_occured(member_image));
	assert(kzrjson_is_member(member_image));

	kzrjson_t object = kzrjson_get_value_from_member(member_image);
	assert(!kzrjson_error_occured(object));
	assert(kzrjson_is_object(object));
	assert(kzrjson_object_size(object) == 6);

	kzrjson_t member_title = kzrjson_get_value_from_key(object, "Title");
	assert(!kzrjson_error_occured(member_title));
	assert(kzrjson_is_string(member_title));
	const char *member_title_string = kzrjson_get_string(member_title);
	assert(strcmp(member_title_string, "View from 15th Floor") == 0);

	const bool member_animated = kzrjson_get_boolean(kzrjson_get_value_from_key(object, "Animated"));
	assert(member_animated == false);

	kzrjson_t array = kzrjson_get_value_from_key(object, "IDs");
	const uint64_t ids[] = {116, 943, 234, 38793};
	for (int i = 0; i < kzrjson_array_size(array); i++) {
		kzrjson_t element = kzrjson_get_element(array, i);
		assert(kzrjson_get_number_as_unsigned_integer(element) == ids[i]);
	}

	kzrjson_free(data);
	puts("test_sample1 done");
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

static void test_sample2(void) {
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
	puts("test_sample2 done");
}

int main(void) {
	test_sample1();
	test_sample2();
	return 0;
}
