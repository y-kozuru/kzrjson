#ifndef KZRJSON_H
#define KZRJSON_H

struct json_data_inner;
typedef struct json_data {
	struct json_data_inner *data;
} json_data;

void kzrjson_print(json_data data);
void kzrjson_free(json_data data);
json_data kzrjson_parse(const char *json_text);

#endif // KZRJSON_H
