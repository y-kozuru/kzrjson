# kzrjson
JSON library for C.

# todo
* Create a function to build a JSON object.

# sample
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
