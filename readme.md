# kzrjson
JSON library for C. It can parse JSON text and construct JSON objects in your programs.

# sample
## Parse JSON text

```c
#include "kzrjson.h"

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

int main(void) {
	kzrjson_t data = kzrjson_parse(sample1);
	kzrjson_t member_image = kzrjson_get_member(data, "Image");

	kzrjson_t object = kzrjson_get_value_from_member(member_image);
	kzrjson_t member_title = kzrjson_get_value_from_key(object, "Title");
	const char *title_string = kzrjson_get_string(member_title);
	// => "View from 15th Floor"

	kzrjson_t animated_value = kzrjson_get_value_from_key(object, "Animated");
	const bool animated = kzrjson_get_boolean(animated_value);
	// => false

	kzrjson_t array = kzrjson_get_value_from_key(object, "IDs");
	for (int i = 0; i < kzrjson_array_size(array); i++) {
		kzrjson_t element = kzrjson_get_element(array, i);
		const uint64_t number = kzrjson_get_number_as_unsigned_integer(element);
		// => 116, 943, 234, 38793
	}

	// The other kzrjson_t (such as object and member_title) obtained from the data will also be released.
	kzrjson_free(data);

	return 0;
}

```

## Make JSON object
```c
#include "kzrjson.h"

int main(void) {
	// make object
	kzrjson_t object = kzrjson_make_object();

	// make member
	kzrjson_t boolean_value = kzrjson_make_boolean(true);
	kzrjson_t member = kzrjson_make_member("member1", strlen("member1"), boolean_value);

	// add member to object
	kzrjson_object_add_member(&object, member);
	// => { "member1": true }

	// make array
	kzrjson_t array = kzrjson_make_array();
	kzrjson_array_add_element(&array, kzrjson_make_number_integer(-100));
	kzrjson_array_add_element(&array, kzrjson_make_number_unsigned_integer(0));
	kzrjson_array_add_element(&array, kzrjson_make_number_double(0.5));
	kzrjson_array_add_element(&array, kzrjson_make_string("sample", strlen("sample")));
	kzrjson_array_add_element(&array, kzrjson_make_null());
	// => [-100, 0, 0.5, "sample", null]

	kzrjson_object_add_member(&object, kzrjson_make_member("array1", strlen("array1"), array));
	/* =>
	 * {
	 *   "member1": true,
	 *   "array1": [-100, 0, 0.5, "sample", null]
	 * }
	 */

	kzrjson_free(object);

	return 0;
}

```

# todo
* Correctly handle escape characters in string
* Add unified error handling
* Convert `json_t` to string
