#ifndef USER_ELEMENT_HPP
#define USER_ELEMENT_HPP

#include <string>
#include <vector>
#include <variant>

#include "element.hpp"
#include "inventoryHarvester.hpp"
#include "networkProtocolElement.hpp"
#include "utils/reflectiveJson.hpp"

namespace wazuh {
namespace inventory {
namespace harvester {

struct UserData {
    REFLECTIVE_JSON_OBJECT;
    std::string uid;
    std::string gid;
    std::string name;
    std::string home;
    std::string shell;
    std::vector<std::string> groups;

    static void declare_reflective_fields() {
        REFLECTIVE_JSON_ADD_FIELD(uid);
        REFLECTIVE_JSON_ADD_FIELD(gid);
        REFLECTIVE_JSON_ADD_FIELD(name);
        REFLECTIVE_JSON_ADD_FIELD(home);
        REFLECTIVE_JSON_ADD_FIELD(shell);
        REFLECTIVE_JSON_ADD_FIELD(groups);
    }
};

class UserElement : public Element {
   public:
    UserElement() : Element(){};
    ~UserElement() = default;

    std::variant<std::string, std::vector<std::string>> fillData(std::shared_ptr<WazuhDBQuery> query) override;
};

}  // namespace harvester
}  // namespace inventory
}  // namespace wazuh

#endif  // USER_ELEMENT_HPP
