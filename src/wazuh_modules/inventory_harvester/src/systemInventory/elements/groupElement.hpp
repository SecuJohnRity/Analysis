#ifndef GROUP_ELEMENT_HPP
#define GROUP_ELEMENT_HPP

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

struct GroupData {
    REFLECTIVE_JSON_OBJECT;
    std::string gid;
    std::string name;
    std::vector<std::string> members;

    static void declare_reflective_fields() {
        REFLECTIVE_JSON_ADD_FIELD(gid);
        REFLECTIVE_JSON_ADD_FIELD(name);
        REFLECTIVE_JSON_ADD_FIELD(members);
    }
};

class GroupElement : public Element {
   public:
    GroupElement() : Element(){};
    ~GroupElement() = default;

    std::variant<std::string, std::vector<std::string>> fillData(std::shared_ptr<WazuhDBQuery> query) override;
};

}  // namespace harvester
}  // namespace inventory
}  // namespace wazuh

#endif  // GROUP_ELEMENT_HPP
