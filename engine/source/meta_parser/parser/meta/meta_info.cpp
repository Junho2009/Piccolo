#include "common/precompiled.h"

#include "parser/parser.h"

#include "meta_info.h"

MetaInfo::MetaInfo(const Cursor& cursor)
{
    for (auto& child : cursor.getChildren())
    {

        //[CR] 只处理 "annotation" 这种 __attribute__。这是 Piccolo 的反射宏中使用的。
        if (child.getKind() != CXCursor_AnnotateAttr)
            continue;
        
        //printf("### Junho - MetaInfo-Ctor - child name: %s\n", child.getDisplayName().c_str()); //[CR] DEBUG

        for (auto& prop : extractProperties(child))
            m_properties[prop.first] = prop.second;
    }
}

std::string MetaInfo::getProperty(const std::string& key) const
{
    auto search = m_properties.find(key);

    // use an empty string by default
    return search == m_properties.end() ? "" : search->second;
}

bool MetaInfo::getFlag(const std::string& key) const { return m_properties.find(key) != m_properties.end(); }

std::vector<MetaInfo::Property> MetaInfo::extractProperties(const Cursor& cursor)
{
    std::vector<Property> ret_list;

    auto propertyList = cursor.getDisplayName();
    //printf("### Junho - propertyList: %s\n", propertyList.c_str()); //[CR] DEBUG

    auto&& properties = Utils::split(propertyList, ",");

    static const std::string white_space_string = " \t\r\n";

    std::vector<std::string> property_str_list;

    for (auto& property_item : properties)
    {
        auto&& item_details = Utils::split(property_item, ":");
        auto&& temp_string  = Utils::trim(item_details[0], white_space_string);
        if (temp_string.empty())
        {
            continue;
        }
        auto&& temp_value_str = item_details.size() > 1 ? Utils::trim(item_details[1], white_space_string) : "";
        ret_list.emplace_back(temp_string, temp_value_str);

        property_str_list.emplace_back(temp_string + ":" + temp_value_str);

        //printf("### Junho - property - name: %s, value: %s\n", temp_string.c_str(), temp_value_str.c_str()); //[CR] DEBUG
    }

    //[CR] TODO: 为了快速实现，没有对 extractProperties 函数的实现进行调整，应该改为同时对 m_properties 和 m_properties_str 进行初始化。
    std::stringstream ss;
    std::copy(property_str_list.begin(), property_str_list.end(),
        std::ostream_iterator<std::string>(ss, ","));
    m_properties_str = ss.str();
    
    return ret_list;
}
