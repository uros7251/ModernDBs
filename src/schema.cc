// ---------------------------------------------------------------------------------------------------
// MODERNDBS
// ---------------------------------------------------------------------------------------------------

#include "moderndbs/schema.h"

using Type = moderndbs::schema::Type;

Type Type::Integer()    {
    Type t{};
    t.tclass = kInteger;
    t.length = 0;
    return t; }
Type Type::Char(unsigned length) {
    Type t{};
    t.tclass = kChar;
    t.length = length;
    return t;
}

const char *Type::name() const {
    switch (tclass) {
        case kInteger:      return "integer";
        case kChar:         return "char";
        default:            return "unknown";
    }
}
