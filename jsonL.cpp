#include "jsonL.hpp"
#include <cstdlib>
#include <cassert>
#include <limits>
#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>

namespace jsonL
{

static const int max_depth = 200;

using std::ifstream;
using std::initializer_list;
using std::make_shared;
using std::map;
using std::move;
using std::ofstream;
using std::string;
using std::vector;

/**
 * null-type -- do nothing
 */
struct NullStruct
{
    bool operator==(NullStruct) const { return true; }
    bool operator<(NullStruct) const { return false; }
};

/**
 * Serialize
 */
static void dump(NullStruct, string &out)
{
    out += "null";
}

static void dump(bool value, string &out)
{
    out += value ? "true" : "false";
}

static void dump(int value, string &out)
{
    char buf[32];
    snprintf(buf, sizeof buf, "%d", value);
    out += buf;
}

static void dump(double value, string &out)
{
    if (std::isfinite(value))
    {
        char buf[32];
        snprintf(buf, sizeof buf, "%.17g", value);
        out += buf;
    }
    else
    {
        out += "null";
    }
}

static void dump(const string &value, string &out)
{
    out += '"';
    for (size_t i = 0; i < value.length(); i++)
    {
        const char ch = value[i];
        if (ch == '\\')
        {
            out += "\\\\";
        }
        else if (ch == '"')
        {
            out += "\\\"";
        }
        else if (ch == '\b')
        {
            out += "\\b";
        }
        else if (ch == '\f')
        {
            out += "\\f";
        }
        else if (ch == '\n')
        {
            out += "\\n";
        }
        else if (ch == '\r')
        {
            out += "\\r";
        }
        else if (ch == '\t')
        {
            out += "\\t";
        }
        else if (static_cast<uint8_t>(ch) <= 0x1f)
        {
            char buf[8];
            snprintf(buf, sizeof buf, "\\u%04x", ch);
            out += buf;
        }
        else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i + 1]) == 0x80 && static_cast<uint8_t>(value[i + 2]) == 0xa8)
        {
            out += "\\u2028";
            i += 2;
        }
        else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i + 1]) == 0x80 && static_cast<uint8_t>(value[i + 2]) == 0xa9)
        {
            out += "\\u2029";
            i += 2;
        }
        else
        {
            out += ch;
        }
    }
    out += '"';
}

static void dump(const Json::array &values, string &out)
{
    bool first = true;
    out += "[";
    for (const auto &value : values)
    {
        if (!first)
            out += ", ";
        value.dump(out);
        first = false;
    }
    out += "]";
}

static void dump(const Json::object &values, string &out)
{
    bool first = true;
    out += "{";
    for (const auto &value : values)
    {
        if (!first)
            out += ", ";
        dump(value.first, out);
        out += ": ";
        value.second.dump(out);
        first = false;
    }
    out += "}";
}

void Json::dump(string &out) const
{
    m_ptr->dump(out);
}

/**
 * wrappers
 */

template <Json::Type tag, typename T>
class Value : public JsonValue
{
protected:
    const T m_value;

    // Constructors
    explicit Value(const T &value) : m_value(value) {}
    explicit Value(T &&value) : m_value(move(value)) {}

    // type
    Json::Type type() const override
    {
        return tag;
    }
    bool equals(const JsonValue *other) const override
    {
        return m_value == static_cast<const Value<tag, T> *>(other)->m_value;
    }
    bool less(const JsonValue *other) const override
    {
        return m_value < static_cast<const Value<tag, T> *>(other)->m_value;
    }
    void dump(string &out) const override
    {
        jsonL::dump(m_value, out);
    }
};

class JsonDouble final : public Value<Json::Type::NUMBER, double>
{
    double number_value() const override { return m_value; }
    int int_value() const override { return static_cast<int>(m_value); }
    // compare with JsonDouble or JsonInt
    bool equals(const JsonValue *other) const override { return m_value == other->number_value(); }
    bool less(const JsonValue *other) const override { return m_value < other->number_value(); }

public:
    explicit JsonDouble(double value) : Value(value) {}
};

class JsonInt final : public Value<Json::Type::NUMBER, int>
{
    double number_value() const override { return m_value; }
    int int_value() const override { return m_value; }
    // compare with JsonDouble or JsonInt
    bool equals(const JsonValue *other) const override { return m_value == other->number_value(); }
    bool less(const JsonValue *other) const override { return m_value < other->number_value(); }

public:
    explicit JsonInt(int value) : Value(value) {}
};

class JsonBoolean final : public Value<Json::Type::BOOL, bool>
{
    bool bool_value() const override { return m_value; }

public:
    explicit JsonBoolean(bool value) : Value(value) {}
};

class JsonString final : public Value<Json::Type::STRING, string>
{
    const string &string_value() const override { return m_value; }

public:
    explicit JsonString(const string &value) : Value(value) {}
    explicit JsonString(string &&value) : Value(move(value)) {}
};

class JsonArray final : public Value<Json::Type::ARRAY, Json::array>
{
    const Json::array &array_items() const override { return m_value; }
    const Json &operator[](size_t i) const override;

public:
    explicit JsonArray(const Json::array &value) : Value(value) {}
    explicit JsonArray(Json::array &&value) : Value(move(value)) {}
};

class JsonObject final : public Value<Json::Type::OBJECT, Json::object>
{
    const Json::object &object_items() const override { return m_value; }
    const Json &operator[](const string &key) const override;

public:
    explicit JsonObject(const Json::object &value) : Value(value) {}
    explicit JsonObject(Json::object &&value) : Value(value) {}
};

class JsonNull final : public Value<Json::Type::NUL, NullStruct>
{
public:
    JsonNull() : Value({}) {}
};

/**
 * Static globals - static-init-safe
 */
struct Statics
{
    const std::shared_ptr<JsonValue> null = make_shared<JsonNull>();
    const std::shared_ptr<JsonValue> t = make_shared<JsonBoolean>(true);
    const std::shared_ptr<JsonValue> f = make_shared<JsonBoolean>(false);
    const string empty_string;
    const vector<Json> empty_vector;
    const map<string, Json> empty_map;
    Statics() {}
};

static const Statics &statics()
{
    static const Statics s{};
    return s;
}

static const Json &static_null()
{
    static const Json json_null;
    return json_null;
}

/**
 * Ctors
*/
Json::Json() noexcept : m_ptr(statics().null) {}
Json::Json(std::nullptr_t) noexcept : m_ptr(statics().null) {}
Json::Json(double value) : m_ptr(make_shared<JsonDouble>(value)) {}
Json::Json(int value) : m_ptr(make_shared<JsonInt>(value)) {}
Json::Json(bool value) : m_ptr(value ? statics().t : statics().f) {}
Json::Json(const std::string &value) : m_ptr(make_shared<JsonString>(value)) {}
Json::Json(std::string &&value) : m_ptr(make_shared<JsonString>(move(value))) {}
Json::Json(const char *value) : m_ptr(make_shared<JsonString>(value)) {}
Json::Json(const array &values) : m_ptr(make_shared<JsonArray>(values)) {}
Json::Json(array &&values) : m_ptr(make_shared<JsonArray>(move(values))) {}
Json::Json(const object &values) : m_ptr(make_shared<JsonObject>(values)) {}
Json::Json(object &&values) : m_ptr(make_shared<JsonObject>(move(values))) {}

/**
 * Accessors
 */
Json::Type Json::type() const { return m_ptr->type(); }
double Json::number_value() const { return m_ptr->number_value(); }
int Json::int_value() const { return m_ptr->int_value(); }
bool Json::bool_value() const { return m_ptr->bool_value(); }
const string &Json::string_value() const { return m_ptr->string_value(); }
const vector<Json> &Json::array_items() const { return m_ptr->array_items(); }
const map<string, Json> &Json::object_items() const { return m_ptr->object_items(); }

const Json &Json::operator[](size_t i) const { return (*m_ptr)[i]; }
const Json &Json::operator[](const string &key) const { return (*m_ptr)[key]; }

double JsonValue::number_value() const { return 0; }
int JsonValue::int_value() const { return 0; }
bool JsonValue::bool_value() const { return false; }
const string &JsonValue::string_value() const { return statics().empty_string; }
const Json::array &JsonValue::array_items() const { return statics().empty_vector; }
const Json &JsonValue::operator[](size_t i) const { return static_null(); }
const Json::object &JsonValue::object_items() const { return statics().empty_map; }
const Json &JsonValue::operator[](const string &key) const { return static_null(); }

const Json &JsonArray::operator[](size_t i) const
{
    if (i >= m_value.size())
        return static_null();
    else
        return m_value[i];
}

const Json &JsonObject::operator[](const string &key) const
{
    auto iter = m_value.find(key);
    return (iter == m_value.end()) ? static_null() : iter->second;
}

/**
 * Comparison
 */

bool Json::operator==(const Json &other) const
{
    if (m_ptr == other.m_ptr)
        return true;
    if (m_ptr->type() != other.m_ptr->type())
        return false;
    return m_ptr->equals(other.m_ptr.get());
}

bool Json::operator<(const Json &other) const
{
    if (m_ptr == other.m_ptr)
        return false;
    if (m_ptr->type() != other.m_ptr->type())
        return m_ptr->type() < other.m_ptr->type();
    return m_ptr->less(other.m_ptr.get());
}

/**
 * Parse
 */

static inline string esc(char c)
{
    char buf[12];
    if (static_cast<uint8_t>(c) >= 0x20 && static_cast<uint8_t>(c) <= 0x7f)
    {
        snprintf(buf, sizeof buf, "'%c' (%d)", c, c);
    }
    else
    {
        snprintf(buf, sizeof buf, "(%d)", c);
    }
    return string(buf);
}

static inline bool in_range(long x, long lower, long upper)
{
    return (x >= lower) && (x <= upper);
}

namespace
{

struct JsonParser final
{
    const string &str;
    size_t i;
    string &err;
    bool failed;
    const JsonParse strategy;

    // 设置错误信息，返回NULL
    Json fail(string &&msg)
    {
        return fail(move(msg), Json());
    }

    template <typename T>
    T fail(string &&msg, const T err_ret)
    {
        if (!failed)
            err = std::move(msg);
        failed = true;
        return err_ret;
    }

    /*
        * consume ws
        * ws = *(%x20 / %x09 / %x0A / %x0D) 空格符space、制表符tab、换行符LF、回车符CR
        */
    void consume_whitespace()
    {
        while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')
            ++i;
    }

    /**
     * Consume comment
     */
    bool consume_comment()
    {
        bool comment_found = false;
        if (str[i] == '/')
        {
            i++;
            if (i == str.size())
                return fail("unexpected end of input after start of comment", false);
            if (str[i] == '/')
            { // inline comment
                i++;
                // advance until next line, or end of input
                while (i < str.size() && str[i] != '\n')
                {
                    i++;
                }
                comment_found = true;
            }
            else if (str[i] == '*')
            { // multiline comment
                i++;
                if (i > str.size() - 2)
                    return fail("unexpected end of input inside multi-line comment", false);
                // advance until closing tokens
                while (!(str[i] == '*' && str[i + 1] == '/'))
                {
                    i++;
                    if (i > str.size() - 2)
                        return fail(
                            "unexpected end of input inside multi-line comment", false);
                }
                i += 2;
                comment_found = true;
            }
            else
                return fail("malformed comment", false);
        }
        return comment_found;
    }

    /**
     * advance
     */
    void consume_garbage()
    {
        consume_whitespace();
        if (strategy == JsonParse::COMMENTS)
        {
            bool comment_found = false;
            do
            {
                comment_found = consume_comment();
                if (failed)
                    return;
                consume_whitespace();
            } while (comment_found);
        }
    }

    /**
     * get next token
     */
    char get_next_token()
    {
        consume_garbage();
        if (failed)
            return static_cast<char>(0);
        if (i == str.size())
            return fail("unexpected end of input", static_cast<char>(0));
        return str[i++];
    }

    /**
     * Parse a double.
     */
    Json parse_number()
    {
        size_t start_pos = i;

        if (str[i] == '-')
            i++;

        if (str[i] == '0')
        {
            i++;
            if (in_range(str[i], '0', '9'))
                return fail("leading 0s not permitted in numbers");
        }
        else if (in_range(str[i], '1', '9'))
        {
            i++;
            while (in_range(str[i], '0', '9'))
                i++;
        }
        else
        {
            return fail("invalid " + esc(str[i]) + "in number");
        }

        if (str[i] != '.' && str[i] != 'e' && str[i] != 'E' && (i - start_pos) <= static_cast<size_t>(std::numeric_limits<int>::digits10))
        {
            return std::atoi(str.c_str() + start_pos);
        }

        if (str[i] == '.')
        {
            i++;
            if (!in_range(str[i], '0', '9'))
                return fail("at least one digit required in fractional part");

            while (in_range(str[i], '0', '9'))
                i++;
        }

        if (str[i] == 'e' || str[i] == 'E')
        {
            i++;
            if (str[i] == '+' || str[i] == '-')
                i++;
            if (!in_range(str[i], '0', '9'))
                return fail("at least one digit required in exponent");
            while (in_range(str[i], '0', '9'))
                i++;
        }

        return std::strtod(str.c_str() + start_pos, nullptr);
    }

    /**
     * encode UTF-8
     */
    void encode_utf8(long pt, string &out)
    {
        if (pt < 0)
            return;

        if (pt < 0x80)
        {
            out += static_cast<char>(pt);
        }
        else if (pt < 0x800)
        {
            out += static_cast<char>((pt >> 6) | 0xC0);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        }
        else if (pt < 0x10000)
        {
            out += static_cast<char>((pt >> 12) | 0xE0);
            out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        }
        else
        {
            out += static_cast<char>((pt >> 18) | 0xF0);
            out += static_cast<char>(((pt >> 12) & 0x3F) | 0x80);
            out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        }
    }

    /**
     * Parse a string
     */
    string parse_string()
    {
        string out;
        long last_escaped_codepoint = -1;
        while (true)
        {
            if (i == str.size())
                return fail("unexpected end of input in string", "");

            char ch = str[i++];

            if (ch == '"')
            {
                encode_utf8(last_escaped_codepoint, out);
                return out;
            }

            if (in_range(ch, 0, 0x1f))
                return fail("unescaped " + esc(ch) + " in string", "");

            if (ch != '\\')
            {
                encode_utf8(last_escaped_codepoint, out);
                last_escaped_codepoint = -1;
                out += ch;
                continue;
            }

            if (i == str.size())
                return fail("unexpected end of input in string", "");

            ch = str[i++];

            if (ch == 'u')
            {
                string esc = str.substr(i, 4);

                if (esc.length() < 4)
                {
                    return fail("bad \\u escape: " + esc, "");
                }
                for (size_t j = 0; j < 4; j++)
                {
                    if (!in_range(esc[j], 'a', 'f') && !in_range(esc[j], 'A', 'F') && !in_range(esc[j], '0', '9'))
                        return fail("bad \\u escape: " + esc, "");
                }

                long codepoint = strtol(esc.data(), nullptr, 16);

                if (in_range(last_escaped_codepoint, 0xD800, 0xDBFF) && in_range(codepoint, 0xDC00, 0xDFFF))
                {
                    encode_utf8((((last_escaped_codepoint - 0xD800) << 10) | (codepoint - 0xDC00)) + 0x10000, out);
                    last_escaped_codepoint = -1;
                }
                else
                {
                    encode_utf8(last_escaped_codepoint, out);
                    last_escaped_codepoint = codepoint;
                }

                i += 4;
                continue;
            }

            encode_utf8(last_escaped_codepoint, out);
            last_escaped_codepoint = -1;

            if (ch == 'b')
            {
                out += '\b';
            }
            else if (ch == 'f')
            {
                out += '\f';
            }
            else if (ch == 'n')
            {
                out += '\n';
            }
            else if (ch == 'r')
            {
                out += '\r';
            }
            else if (ch == 't')
            {
                out += '\t';
            }
            else if (ch == '"' || ch == '\\' || ch == '/')
            {
                out += ch;
            }
            else
            {
                return fail("invalid escape character " + esc(ch), "");
            }
        }
    }

    /**
     * expect null true false
     */
    Json expect(const string &expected, Json res)
    {
        assert(i != 0);
        size_t len = expected.length();
        i--;
        if (str.compare(i, len, expected) == 0)
        {
            i += len;
            return res;
        }
        else
        {
            return fail("parse error : expected " + expected + ", got " + str.substr(i, len));
        }
    }

    /**
     * Parse a JSON object
     */
    Json parse_json(int depth)
    {
        if (depth > max_depth)
        {
            return fail("exceeded maximum nesting depth");
        }

        char ch = get_next_token();
        if (failed)
            return Json();

        if (ch == '-' || (ch >= '0' && ch <= '9'))
        {
            i--;
            return parse_number();
        }

        if (ch == 'n')
            return expect("null", Json());

        if (ch == 't')
            return expect("true", true);

        if (ch == 'f')
            return expect("false", false);

        if (ch == '"')
            return parse_string();

        if (ch == '{')
        {
            map<string, Json> data;
            ch = get_next_token();
            if (ch == '}')
                return data;

            while (1)
            {
                if (ch != '"')
                    return fail("expected '\"' in object, got " + esc(ch));

                string key = parse_string();
                if (failed)
                    return Json();

                ch = get_next_token();
                if (ch != ':')
                    return fail("expected ':' in object, got " + esc(ch));

                data[std::move(key)] = parse_json(depth + 1);
                if (failed)
                    return Json();

                ch = get_next_token();
                if (ch == '}')
                    break;
                if (ch != ',')
                    return fail("expected ',' in object, got " + esc(ch));

                ch = get_next_token();
            }
            return data;
        }

        if (ch == '[')
        {
            vector<Json> data;
            ch = get_next_token();
            if (ch == ']')
                return data;

            while (1)
            {
                i--;
                data.push_back(parse_json(depth + 1));
                if (failed)
                    return Json();

                ch = get_next_token();
                if (ch == ']')
                    break;
                if (ch != ',')
                    return fail("expected ',' in list, got " + esc(ch));

                ch = get_next_token();
                (void)ch;
            }
            return data;
        }

        return fail("expected value, got " + esc(ch));
    }
};

} // namespace

Json Json::parse(const std::string &in,
                    std::string &err,
                    JsonParse strategy)
{
    JsonParser parser{in, 0, err, false, strategy};
    Json result = parser.parse_json(0);

    parser.consume_garbage();
    if (parser.failed)
        return Json();
    if (parser.i != in.size())
        return parser.fail("unexpected trailing " + esc(in[parser.i]));

    return result;
}

std::vector<Json> Json::parse_multi(const std::string &in,
                                    std::string::size_type &parser_stop_pos,
                                    std::string &err,
                                    JsonParse strategy)
{
    JsonParser parser{in, 0, err, false, strategy};
    parser_stop_pos = 0;
    vector<Json> json_vec;
    while (parser.i != in.size() && !parser.failed)
    {
        json_vec.push_back(parser.parse_json(0));
        if (parser.failed)
            break;
        parser.consume_garbage();
        if (parser.failed)
            break;
        parser_stop_pos = parser.i;
    }
    return json_vec;
}

bool Json::has_shape(const shape &types, std::string &err) const
{
    if (!is_object())
    {
        err = "expected JSON object, got " + dump();
        return false;
    }

    const auto &obj_items = object_items();
    for (auto &item : types)
    {
        const auto it = obj_items.find(item.first);
        if (it == obj_items.cend() || it->second.type() != item.second)
        {
            err = "bad type for " + item.first + "in" + dump();
            return false;
        }
    }

    return true;
}

Json Json::parse_from_file(const std::string &filename,
                            string &err,
                            JsonParse strategy)
{
    ifstream fin(filename);
    if (!fin.is_open())
    {
        err = "can not open the file";
        return Json();
    }
    std::stringstream buffer;
    buffer << fin.rdbuf();
    string in = buffer.str();
    fin.close();
    return parse(in, err, strategy);
}

void Json::dump_to_file(const std::string &filename) const
{
    ofstream fout(filename);
    if (!fout.is_open())
    {
        std::cout << "can not open the file" << std::endl;
        return;
    }
    fout << this->dump();
    fout.close();
}

} // namespace jsonL