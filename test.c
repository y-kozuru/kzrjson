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

int main(void) {
	const char *test_text = sample1;
	kzrjson_data data = kzrjson_parse(test_text);
	kzrjson_print(data);
	kzrjson_free(data);
	return 0;
}
