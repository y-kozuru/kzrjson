#include "kzrjson.hpp"
#include <map>
#include <set>
#include <stdexcept>
#include <string>
using namespace std::string_literals;

namespace {

enum class token_type {
	begin_array,
	begin_object,
	end_array,
	end_object,
	name_separator,
	value_separator,
	white_space,
	true_,
	false_,
	null,
	decimal_point,
	zero,
	digit1_9,
	e,
	minus,
	plus,
	escape,
	quotation_mark,
	string,
	end_of_text,
	invalid,
};

const std::map<char, token_type> single_chars = {
	{'[', token_type::begin_array},
	{']', token_type::end_array},
	{'{', token_type::begin_object},
	{'}', token_type::end_object},
	{':', token_type::name_separator},
	{',', token_type::value_separator},
	{'.', token_type::decimal_point},
	{'-', token_type::minus},
	{'+', token_type::plus},
	{'0', token_type::zero},
};

const std::map<std::set<char>, token_type> multi_chars = {
	{std::set<char>{'1', '2', '3', '4', '5', '6', '7', '8', '9'}, token_type::digit1_9},
	{std::set<char>{'e', 'E'}, token_type::e},
};

const std::map<std::string, token_type> keywords = {
	{"false", token_type::false_},
	{"true", token_type::true_},
	{"null", token_type::null},
};

struct token {
	token_type type;
	std::string string;
};
	
struct lexer {
	const std::string text;
	std::string::const_iterator pos;
	token current_token;

	lexer(const std::string& text)
		: text(text)
		, pos(this->text.begin())
		, current_token()
	{}

	void next_token() {
		if (pos >= text.end()) {
			return set_token(token_type::end_of_text);
		}
		while (consume_contained(std::set<char>{' ', 0x09, 0x0A, 0x0D})) {
			if (pos >= text.end()) {
				return set_token(token_type::end_of_text);
			}
		}
		for (const auto& single : single_chars) {
			if (consume(single.first)) {
				return set_token(single.second, single.first);
			}
		}
		for (const auto& multi : multi_chars) {
			if (consume_contained(multi.first)) {
				return set_token(multi.second, *(pos - 1));
			}
		}
		for (const auto& keyword : keywords) {
			if (consume_keyword(keyword.first)) {
				return set_token(keyword.second, keyword.first);
			}
		}
		if (consume(quotation_mark)) {
			return set_token_string();
		}
		throw std::runtime_error("Invalid token");
	}

private:
	void set_token(const token_type type, const char c = '\0') {
		current_token = token(type, std::string{c});
	}
	void set_token(const token_type type, const std::string& s) {
		current_token = token(type, s);
	}

	bool consume(const char c) {
		if (*pos == c) {
			pos++;
			return true;
		}
		return false;
	}

	bool contained(const std::set<char> chars) const {
		return chars.find(*pos) != chars.end();
	}

	bool consume_contained(const std::set<char>& chars) {
		if (contained(chars)) {
			pos++;
			return true;
		}
		return false;
	}

	bool consume_keyword(const std::string& keyword) {
		for (size_t i = 0; i < keyword.size(); i++) {
			if (*(pos + i) != keyword[i]) {
				return false;
			}
		}
		pos += keyword.size();
		return true;
	}

	/*
	* string = quotation-mark *char quotation-mark
	* char = unescaped /
	* escape (
	*   %x22 /          ; "    quotation mark  U+0022
	*   %x5C /          ; \    reverse solidus U+005C
	*   %x2F /          ; /    solidus         U+002F
	*   %x62 /          ; b    backspace       U+0008
	*   %x66 /          ; f    form feed       U+000C
	*   %x6E /          ; n    line feed       U+000A
	*   %x72 /          ; r    carriage return U+000D
	*   %x74 /          ; t    tab             U+0009
	*   %x75 4HEXDIG )  ; uXXXX                U+XXXX
	* unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
	*/
	const char escape = 0x5C;
	const char quotation_mark = 0x22;
	const std::set<char> escape_targets = {0x22, 0x5C, 0x2F, 0x62, 0x66, 0x6E, 0x72, 0x74, 0x75};
	void set_token_string() {
		const int begin = pos - text.begin();
		int length = 0;
		while (*pos != quotation_mark) {
			if (pos >= text.end()) {
				throw std::runtime_error("wrong string");
			}
			if (consume(escape)) {
				if (!consume_contained(escape_targets)) {
					throw std::runtime_error("Wring string");
				}
				length += 2;
			} else {
				length++;
			}
			pos++;
		}
		pos++;
		const auto s = text.substr(begin, length);
		set_token(token_type::string, s);
	}
};

template <typename... arguments>
kzr::json_pointer make_json(arguments... args) {
	return std::make_unique<kzr::json>(std::forward<arguments>(args)...);
}

struct parser {
	parser(const std::string& text)
		: lexer(text)
	{}

	kzr::json_pointer parse() {
		next();
		return value();
	}

	// value = false / null / true / string / object / array / number
	kzr::json_pointer value() {
		if (is(token_type::false_)) {
			next();
			return make_json(kzr::json_type::boolean, false);
		} else if (is(token_type::true_)) {
			next();
			return make_json(kzr::json_type::boolean, true);
		} else if (is(token_type::null)) {
			next();
			return make_json(kzr::json_type::null);
		} else if (is(token_type::string)) {
			const auto string = lexer.current_token.string;
			next();
			return make_json(kzr::json_type::string, string);
		} else if (is(token_type::begin_array)) {
			return array();
		} else if (is(token_type::begin_object)) {
			return object();
		} else {
			return number();
		}
	}

	// array = begin-array [ value *( value-separator value ) ] end-array
	kzr::json_pointer array() {
		auto n = make_json(kzr::json_type::array);
		must(token_type::begin_array);
		next();
		if (!is(token_type::end_array)) {
			n->elements.push_back(value());
			while (is(token_type::value_separator)) {
				next();
				n->elements.push_back(value());
			}
		}
		must(token_type::end_array);
		next();
		return n;
	}

	// object = begin-object [ member *( value-separator member ) ] end-object
	kzr::json_pointer object() {
		auto n = make_json(kzr::json_type::object);
		must(token_type::begin_object);
		next();
		if (!is(token_type::end_object)) {
			n->elements.push_back(member());
			while (is(token_type::value_separator)) {
				next();
				n->elements.push_back(member());
			}
		}
		must(token_type::end_object);
		next();
		return n;
	}

	// number = [ minus ] int [ frac ] [ exp ]
	// int = zero / (digit1-9 *DIGIT)
	// frac = "." 1*DIGIT
	// exp = "e" ["-" / "+"] 1*DIGIT
	kzr::json_pointer number() {
		const auto begin = lexer.pos;
		//auto value = std::variant<int, double>{};
		auto type = kzr::json_type::number_int;
		if (is(token_type::minus)) {
			next();
		}

		// int
		if (is(token_type::zero)) {
			next();
		} else {
			must(token_type::digit1_9);
			next();
			while (is(token_type::digit1_9) || is(token_type::zero)) {
				next();
			}
		}

		// frac
		if (is(token_type::decimal_point)) {
			type = kzr::json_type::number_double;
			next();
			if (!is(token_type::digit1_9) && !is(token_type::zero)) {
				throw std::runtime_error("Parse Error");
			}
			next();
			while (is(token_type::digit1_9) || is(token_type::zero)) {
				next();
			}
		}

		// exp
		if (is(token_type::e)) {
			type = kzr::json_type::number_double;
			if (!is(token_type::plus) && !is(token_type::minus)) {
				throw std::runtime_error("Parse Error");
			}
			next();
			while (is(token_type::digit1_9) || is(token_type::zero)) {
				next();
			}
		}
		const auto length = lexer.pos - begin;
		const auto s = lexer.text.substr(begin - lexer.text.begin() - 1, length);
		return make_json(type, s);
	}

	// member = string name-separator value
	kzr::json_pointer member() {
		must(token_type::string);
		const auto key = lexer.current_token.string;
		next();
		must(token_type::name_separator);
		next();
		return make_json(kzr::json_type::member, key, value());
	}

private:
	lexer lexer;

	bool is(const token_type type) const {
		return lexer.current_token.type == type;
	}

	void must(const token_type type) {
		if (!is(type)) {
			throw std::runtime_error("Parse Error");
		}
	}

	void next() {
		lexer.next_token();
	}
};

} // unnamed

namespace kzr {

json::json(const json_type type, const std::int64_t value)
	: type(type)
	, value(value)
{}

json::json(const json_type type, const double value)
	: type(type)
	, value(value)
{}

json::json(const json_type type, const bool value)
	: type(type)
	, value(value)
{}

json::json(const json_type type, const std::string& value)
	: type(type)
	, value(value)
{}

json::json(const json_type type, const std::string& key, json_pointer&& value)
	: type(type)
	, value(key)
{
	elements.push_back(std::move(value));
}

json::json(const json_type type)
	: type(type)
{}

json_pointer parse(const std::string& json_text) {
	auto a_parser = parser(json_text);
	return a_parser.parse();
}

auto g_indent = 0;

std::string add_indent() {
	return std::string(g_indent * 4, ' ');
}

std::string to_text_recursive(const json& node) {
	auto s = ""s;
	switch (node.type) {
	case json_type::array:
		s += "[\n";
		g_indent++;
		for (auto i = 0u; i < node.elements.size(); i++) {
			const auto& e = *node.elements[i];
			s += add_indent();
			s += to_text_recursive(e);
			if (i + 1 != node.elements.size()) {
				s += ",\n";
			}
		}
		s += "\n"s;
		g_indent--;
		s += add_indent();
		s += "]"s;
		break;
	case json_type::object:
		s += "{\n";
		g_indent++;
		for (auto i = 0u; i < node.elements.size(); i++) {
			const auto& e = *node.elements[i];
			s += add_indent();
			s += to_text_recursive(e);
			if (i + 1 != node.elements.size()) {
				s += ",\n";
			}
		}
		s += "\n"s;
		g_indent--;
		s += add_indent();
		s += "}"s;
		break;
	case json_type::member:
		s += "\""s + std::get<std::string>(node.value) + "\""s;
		s += ": "s;
		s += to_text_recursive(*node.elements[0]);
		break;
	case json_type::string:
		s += "\""s + std::get<std::string>(node.value) + "\""s;
		break;
	case json_type::number_double:
	case json_type::number_int:
		s += std::get<std::string>(node.value);
		break;
	case json_type::boolean:
		s += std::get<bool>(node.value) ? "true" : "false";
		break;
	case json_type::null:
		s += "null";
		break;
	}
	return s;
}

std::string to_text(const json& node) {
	g_indent = 0;
	const auto text = to_text_recursive(node);
	g_indent = 0;
	return text;
}

} // namespace kzr
