#include "jsonL.hpp"
#include <iostream>
using namespace jsonL;
using namespace std;

void print_type(Json::Type t);

class A{
public:
    A(int i) : m_i(i) {}
    Json to_json() const {
        return Json(m_i);
    }
private:
    int m_i;
};

int main(int argc, char const *argv[])
{
#if 0
    std::string err;

    Json json(nullptr);
    print_type(json.type());
    
    Json json1(true);
    print_type(json1.type());

    Json json2(false);
    print_type(json2.type());

    vector<Json> a{1, 2, 3, 4};
    Json json3(a);
    print_type(json3.type());
    auto str3 = json3.dump();
    cout << str3 << endl;

    Json json4(1.0);
    if (json < json4)
        cout << "L" << endl;
    else if (json == json4)
        cout << "E" << endl;
    else
        cout << "G" << endl;

    auto json5 = Json::parse(std::move("true  // sdjkw"), err, JsonParse::COMMENTS);
    print_type(json5.type());

    auto json6 = Json::parse_from_file("canada.json", err, JsonParse::COMMENTS);
    print_type(json6.type());
    cout << err << endl;
    json6.dump_to_file("out.json");
#endif

#if 0
    string err;
    string in = "2200012,null";
    Json json = Json::parse(in, err);
    cout <<err;
#endif

#if 0
    string err;
    string in = "\"20\\u202830\"";
    Json json = Json::parse(in, err);
    if (!err.empty()) 
        cout << err << endl;
    cout << json.string_value();

#endif

/**
 * parse_multi
*/
#if 0
   string err;
   size_t i;
   string in = "\"20\" 3 32.4";
   auto json_vec =  Json::parse_multi(in, i, err); 
   cout << json_vec[0].string_value() << " " 
        << json_vec[1].int_value() << " "
        << json_vec[2].number_value() << endl;
#endif

/**
 * Initialize_list
*/
#if 0
    Json json = Json {"liu shuai"};
    cout << json.string_value() << endl; 

    Json json_t = Json::object {{"name", "liu shuai"}};
    cout << json_t["name"].string_value() << endl;

    Json json1 = Json::array {"liu shuai", true, 181, 64.5};
    cout << json1[0].string_value() << endl;

    Json json2 = Json::object {
        {"name", "liu shuai"},
        {"sex", true},
        {"height", 181},
        {"weight", 64.5}
    };
    cout << json2["name"].string_value() << endl;

    auto map = json2.object_items();
    for (auto& pair : map) {
        cout << pair.first << " ";
        print_type(pair.second.type());
    }
#endif

/**
 * Implicit Ctors
*/
#if 1
    A a {1};
    Json json(a);
    print_type(json.type());

#endif

    return 0;
}

void print_type(Json::Type type_t)
{
    switch (type_t)
    {
#define COUT(t)             \
    case Json::Type::t:     \
        cout << #t << endl; \
        break;
        COUT(NUL);
        COUT(NUMBER);
        COUT(BOOL);
        COUT(STRING);
        COUT(ARRAY);
        COUT(OBJECT);
#undef COUT
    default:
        break;
    }
}

