#ifndef _WCS_USER_HPP
#define _WCS_USER_HPP

#include "reflectiveJson.hpp"
#include <string>
#include <vector>

// Forward declaration if Host is in a different header or define basic Host here
struct Host {
    std::string ip;
    REFLECTABLE(MAKE_FIELD("ip", &Host::ip));
};

struct Login {
    bool status = false;
    std::string tty;
    std::string type;
    REFLECTABLE(MAKE_FIELD("status", &Login::status),
                MAKE_FIELD("tty", &Login::tty),
                MAKE_FIELD("type", &Login::type));
};

struct Process {
    long pid = 0;
    REFLECTABLE(MAKE_FIELD("pid", &Process::pid));
};

struct AuthFailures {
    int count = 0;
    std::string timestamp; // Assuming ISO8601 date string
    REFLECTABLE(MAKE_FIELD("count", &AuthFailures::count),
                MAKE_FIELD("timestamp", &AuthFailures::timestamp));
};

struct UserGroupInfo { // Renamed from Group to avoid conflict if a top-level Group struct is needed
    unsigned long id = 0; // Assuming this is 'id' from user.group.id
    long id_signed = 0;
    REFLECTABLE(MAKE_FIELD("id", &UserGroupInfo::id),
                MAKE_FIELD("id_signed", &UserGroupInfo::id_signed));
};

struct Password {
    std::string expiration_date; // Assuming ISO8601 date string
    std::string hash_algorithm;
    int inactive_days = 0;
    long last_change = 0; // Assuming this is an epoch timestamp or similar numeric value
    std::string last_set_time; // Assuming ISO8601 date string
    int max_days_between_changes = 0;
    int min_days_between_changes = 0;
    std::string status;
    int warning_days_before_expiration = 0;
    REFLECTABLE(MAKE_FIELD("expiration_date", &Password::expiration_date),
                MAKE_FIELD("hash_algorithm", &Password::hash_algorithm),
                MAKE_FIELD("inactive_days", &Password::inactive_days),
                MAKE_FIELD("last_change", &Password::last_change),
                MAKE_FIELD("last_set_time", &Password::last_set_time),
                MAKE_FIELD("max_days_between_changes", &Password::max_days_between_changes),
                MAKE_FIELD("min_days_between_changes", &Password::min_days_between_changes),
                MAKE_FIELD("status", &Password::status),
                MAKE_FIELD("warning_days_before_expiration", &Password::warning_days_before_expiration));
};

struct User {
    AuthFailures auth_failures;
    std::string created; // Assuming ISO8601 date string
    std::string full_name;
    UserGroupInfo group; // This is user.group in the JSON
    std::vector<std::string> groups; // This is user.groups in the JSON
    std::string home;
    std::string id;
    bool is_hidden = false;
    bool is_remote = false;
    std::string last_login; // Assuming ISO8601 date string
    std::string name;
    Password password;
    std::vector<std::string> roles;
    std::string shell;
    std::string type;
    long uid_signed = 0; // Corresponds to user.uid_signed
    std::string uuid;

    REFLECTABLE(MAKE_FIELD("auth_failures", &User::auth_failures),
                MAKE_FIELD("created", &User::created),
                MAKE_FIELD("full_name", &User::full_name),
                MAKE_FIELD("group", &User::group),
                MAKE_FIELD("groups", &User::groups),
                MAKE_FIELD("home", &User::home),
                MAKE_FIELD("id", &User::id),
                MAKE_FIELD("is_hidden", &User::is_hidden),
                MAKE_FIELD("is_remote", &User::is_remote),
                MAKE_FIELD("last_login", &User::last_login),
                MAKE_FIELD("name", &User::name),
                MAKE_FIELD("password", &User::password),
                MAKE_FIELD("roles", &User::roles),
                MAKE_FIELD("shell", &User::shell),
                MAKE_FIELD("type", &User::type),
                MAKE_FIELD("uid_signed", &User::uid_signed),
                MAKE_FIELD("uuid", &User::uuid));
};

#endif // _WCS_USER_HPP
