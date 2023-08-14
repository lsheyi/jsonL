#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <initializer_list>
namespace jsonL
{
    
/**
 * Parse Strategy
*/
enum class JsonParse
{
    STANDARD,
    COMMENTS
};

class JsonValue;

class Json final
{
public:
    using array = std::vector<Json>;
    using object = std::map<std::string, Json>;

    /**
     * Json types
     */
    enum class Type
    {
        NUL,
        NUMBER,
        BOOL,
        STRING,
        ARRAY,
        OBJECT
    };

    /**
     * Ctors
     */
    Json() noexcept;
    Json(std::nullptr_t) noexcept;
    Json(double value);
    Json(int value);
    Json(bool value);
    Json(const std::string &value);
    Json(std::string &&value);
    Json(const char *value);
    Json(const array &values);
    Json(array &&values);
    Json(const object &values);
    Json(object &&values);

    /**
     * Implicit Ctors
     */
    template <class T, class = decltype(&T::to_json)>
    Json(const T &t) : Json(t.to_json()) {}

    template <class M, typename std::enable_if<
                            std::is_constructible<std::string, decltype(std::declval<M>().begin()->first)>::value && std::is_constructible<Json, decltype(std::declval<M>().begin()->second)>::value,
                            int>::type = 0>
    Json(const M &m) : Json(object(m.begin(), m.end())) {}

    template <class V, typename std::enable_if<
                            std::is_constructible<Json, decltype(*std::declval<V>().begin())>::value,
                            int>::type = 0>
    Json(const V &v) : Json(array(v.begin(), v.end())) {}

    //only accept const char * since std::string(const char *)
    Json(void *) = delete;

    /**
     * Accessors
     */
    Type type() const;

    bool is_null() const { return type() == Type::NUL; }
    bool is_number() const { return type() == Type::NUMBER; }
    bool is_bool() const { return type() == Type::BOOL; }
    bool is_string() const { return type() == Type::STRING; }
    bool is_array() const { return type() == Type::ARRAY; }
    bool is_object() const { return type() == Type::OBJECT; }

    double number_value() const;
    int int_value() const;
    bool bool_value() const;
    const std::string &string_value() const;
    const array &array_items() const;
    const object &object_items() const;

    const Json &operator[](size_t i) const;
    const Json &operator[](const std::string &key) const;

    /**
     * Operator overload
     */
    bool operator==(const Json &rhs) const;
    bool operator!=(const Json &rhs) const { return !(*this == rhs); }
    bool operator<(const Json &rhs) const;
    bool operator<=(const Json &rhs) const { return !(rhs < *this); }
    bool operator>(const Json &rhs) const { return (rhs < *this); }
    bool operator>=(const Json &rhs) const { return !(*this < rhs); }

    /**
     * Parse   static
     */
    static Json parse(const std::string &in,
                        std::string &err,
                        JsonParse strategy = JsonParse::STANDARD);
    static Json parse(const char *in,
                        std::string &err,
                        JsonParse strategy = JsonParse::STANDARD)
    {
        if (in)
            return parse(std::string(in), err, strategy);
        else
        {
            err = "null input";
            return nullptr;
        }
    }

    static std::vector<Json> parse_multi(const std::string &in,
                                            std::string::size_type &parser_stop_pos,
                                            std::string &err,
                                            JsonParse strategy = JsonParse::STANDARD);

    static inline std::vector<Json> parse_multi(const std::string &in,
                                                std::string &err,
                                                JsonParse strategy = JsonParse::STANDARD)
    {
        std::string::size_type parser_stop_pos;
        return parse_multi(in, parser_stop_pos, err, strategy);
    }

    static Json parse_from_file(const std::string &filename,
                                std::string &err,
                                JsonParse strategy = JsonParse::STANDARD);

    /**
     * Serialize
     */
    void dump(std::string &out) const;
    std::string dump() const
    {
        std::string out;
        dump(out);
        return out;
    }

    void dump_to_file(const std::string &filename) const;

    /**
     * Type judgement
     */
    using shape = std::initializer_list<std::pair<std::string, Type>>;
    bool has_shape(const shape &types, std::string &err) const;

private:
    std::shared_ptr<JsonValue> m_ptr;
};

class JsonValue
{
protected:
    friend class Json;
    friend class JsonInt;
    friend class JsonDouble;

    virtual Json::Type type() const = 0;
    virtual bool equals(const JsonValue *other) const = 0;
    virtual bool less(const JsonValue *other) const = 0;
    virtual void dump(std::string &out) const = 0;

    virtual double number_value() const;
    virtual int int_value() const;
    virtual bool bool_value() const;
    virtual const std::string &string_value() const;
    virtual const Json::array &array_items() const;
    virtual const Json &operator[](size_t i) const;
    virtual const Json::object &object_items() const;
    virtual const Json &operator[](const std::string &key) const;

    
    virtual ~JsonValue() {}
};

}
