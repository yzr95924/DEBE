#include "../../include/IAS/json.h"

namespace json {

JSON Array()
{
    return JSON::Make(JSON::Class::Array);
}

template <typename... T>
JSON Array(T... args)
{
    JSON arr = JSON::Make(JSON::Class::Array);
    arr.append(args...);
    return arr;
}

JSON Object()
{
    return JSON::Make(JSON::Class::Object);
}

std::ostream& operator<<(std::ostream& os, const JSON& json)
{
    os << json.dump();
    return os;
}

JSON JSON::Load(const string& str)
{
    size_t offset = 0;
    return parse_next(str, offset);
}
}