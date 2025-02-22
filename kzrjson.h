#ifndef KZRJSON_H
#define KZRJSON_H

struct json_data_inner;
typedef struct json_data {
	struct json_data_inner *data;
} json_data;

void print_json(json_data data);
json_data parse(const char *json_text);

#endif // KZRJSON_H
