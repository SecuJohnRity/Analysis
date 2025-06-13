#ifndef _WCS_GROUP_HPP
#define _WCS_GROUP_HPP

#include "reflectiveJson.hpp"
#include <string>
#include <vector>

struct Group {
    std::string description;
    unsigned long id = 0;
    long id_signed = 0;
    bool is_hidden = false;
    std::string name;
    std::vector<std::string> users;
    std::string uuid;

    REFLECTABLE(MAKE_FIELD("description", &Group::description),
                MAKE_FIELD("id", &Group::id),
                MAKE_FIELD("id_signed", &Group::id_signed),
                MAKE_FIELD("is_hidden", &Group::is_hidden),
                MAKE_FIELD("name", &Group::name),
                MAKE_FIELD("users", &Group::users),
                MAKE_FIELD("uuid", &Group::uuid));
};

#endif // _WCS_GROUP_HPP
