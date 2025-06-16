/*
 * Wazuh Inventory Harvester - Group class definition
 * Copyright (C) 2015, Wazuh Inc.
 * October 2024.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef _WCS_GROUP_HPP
#define _WCS_GROUP_HPP

#include "reflectiveJson.hpp"
#include <string_view> // Added for std::string_view
#include <vector>      // Kept for std::vector

struct Group final {
    std::string_view description; // Changed to string_view
    unsigned long id = 0;
    long id_signed = 0;
    bool is_hidden = false;
    std::string_view name;        // Changed to string_view
    std::vector<std::string_view> users; // Changed to vector of string_view
    std::string_view uuid;        // Changed to string_view

    REFLECTABLE(MAKE_FIELD("description", &Group::description),
                MAKE_FIELD("id", &Group::id),
                MAKE_FIELD("id_signed", &Group::id_signed),
                MAKE_FIELD("is_hidden", &Group::is_hidden),
                MAKE_FIELD("name", &Group::name),
                MAKE_FIELD("users", &Group::users),
                MAKE_FIELD("uuid", &Group::uuid));
};

#endif // _WCS_GROUP_HPP
