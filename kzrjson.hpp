#ifndef kzrjson_hpp
#define kzrjson_hpp
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

namespace kzr {

enum class json_type {
	array,
	object,
	string,
	number_double,
	number_int,
	boolean,
	null,
	member,
};

struct json;
using json_pointer = std::unique_ptr<json>;

struct json {
	explicit json(const json_type type);
	explicit json(const json_type type, const std::int64_t value);
	explicit json(const json_type type, const double value);
	explicit json(const json_type type, const bool value);
	explicit json(const json_type type, const std::string& value);
	explicit json(const json_type type, const std::string& key, json_pointer&& value);

	json_type type;
	std::vector<json_pointer> elements;
	std::variant<std::int64_t, double, bool, std::string> value;
};

// Parse JSON text and construct JSON tree.
json_pointer parse(const std::string& json_text);

std::string to_text(const json& head);

} // namespace kzr

#endif // kzrjson_hpp
