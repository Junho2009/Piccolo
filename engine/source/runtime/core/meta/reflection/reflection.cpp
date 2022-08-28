#include "reflection.h"
#include <cstring>
#include <map>

#include "core/base/macro.h"

namespace Piccolo
{
    std::vector<std::string> split(std::string input, std::string pat)
    {
        std::vector<std::string> ret_list;
        while (true)
        {
            size_t      index    = input.find(pat);
            std::string sub_list = input.substr(0, index);
            if (!sub_list.empty())
            {

                ret_list.push_back(sub_list);
            }
            input.erase(0, index + pat.size());
            if (index == -1)
            {
                break;
            }
        }
        return ret_list;
    }


    
    namespace Reflection
    {
        const char* k_unknown_type = "UnknownType";
        const char* k_unknown      = "Unknown";

        static std::map<std::string, ClassFunctionTuple*>      m_class_map;
        static std::multimap<std::string, FieldFunctionTuple*> m_field_map; //[CR] 这里用了 multimap，因为一个类型中可能有多个 field，而这里将类型名作为key，因此可能会有多个相同的 key。
        static std::map<std::string, ArrayFunctionTuple*>      m_array_map;

        /**
         * [CR] 该函数是在 “注册”反射代码 的时候被调用的，反射代码调用的是【REGISTER_FIELD_TO_MAP】宏。
         *   作用：注册后，可获取某反射类型（参数 “name” 就是该类型的名称）中、某个反射字段 的函数集（getter / setter / getClassName / ... 详见反射代码模板 “commonReflectionFile.mustache”）
         */
        void TypeMetaRegisterinterface::registerToFieldMap(const char* name, FieldFunctionTuple* value)
        {
            m_field_map.insert(std::make_pair(name, value));
        }

        /**
         * [CR] 该函数是在 “注册”反射代码 的时候被调用的，反射代码调用的是【REGISTER_ARRAY_TO_MAP】宏。
         *   作用：注册后，可获取 某个数组类型（目前基本上都是 std::vector<xxx>）的函数集（getter / setter / getArrayTypeName / ... 详见反射代码模板 “commonReflectionFile.mustache”）
         */
        void TypeMetaRegisterinterface::registerToArrayMap(const char* name, ArrayFunctionTuple* value)
        {
            if (m_array_map.find(name) == m_array_map.end())
            {
                m_array_map.insert(std::make_pair(name, value));
            }
            else
            {
                delete value;
            }
        }

        /**
         * [CR] 该函数是在 “注册”反射代码 的时候被调用的，反射代码调用的是【REGISTER_BASE_CLASS_TO_MAP】宏。
         *   作用：注册后，可获取 某个反射类型（参数 “name” 就是该类型的名称）的 基类
         */
        void TypeMetaRegisterinterface::registerToClassMap(const char* name, ClassFunctionTuple* value)
        {
            if (m_class_map.find(name) == m_class_map.end())
            {
                m_class_map.insert(std::make_pair(name, value));
            }
            else
            {
                delete value;
            }
        }

        void TypeMetaRegisterinterface::unregisterAll()
        {
            for (const auto& itr : m_field_map)
            {
                delete itr.second;
            }
            m_field_map.clear();
            for (const auto& itr : m_class_map)
            {
                delete itr.second;
            }
            m_class_map.clear();
            for (const auto& itr : m_array_map)
            {
                delete itr.second;
            }
            m_array_map.clear();
        }

        TypeMeta::TypeMeta(std::string type_name) : m_type_name(type_name)
        {
            m_is_valid = false;
            m_fields.clear();

            auto fileds_iter = m_field_map.equal_range(type_name);
            while (fileds_iter.first != fileds_iter.second)
            {
                FieldAccessor f_field(fileds_iter.first->second);
                m_fields.emplace_back(f_field);
                m_is_valid = true;

                ++fileds_iter.first;
            }
        }

        TypeMeta::TypeMeta() : m_type_name(k_unknown_type), m_is_valid(false) { m_fields.clear(); }

        TypeMeta TypeMeta::newMetaFromName(std::string type_name)
        {
            TypeMeta f_type(type_name);
            return f_type;
        }

        bool TypeMeta::newArrayAccessorFromName(std::string array_type_name, ArrayAccessor& accessor)
        {
            auto iter = m_array_map.find(array_type_name);

            if (iter != m_array_map.end())
            {
                ArrayAccessor new_accessor(iter->second);
                accessor = new_accessor;
                return true;
            }

            return false;
        }

        ReflectionInstance TypeMeta::newFromNameAndPJson(std::string type_name, const PJson& json_context)
        {
            auto iter = m_class_map.find(type_name);

            if (iter != m_class_map.end())
            {
                return ReflectionInstance(TypeMeta(type_name), (std::get<1>(*iter->second)(json_context)));
            }
            return ReflectionInstance();
        }

        PJson TypeMeta::writeByName(std::string type_name, void* instance)
        {
            auto iter = m_class_map.find(type_name);

            if (iter != m_class_map.end())
            {
                return std::get<2>(*iter->second)(instance);
            }
            return PJson();
        }

        std::string TypeMeta::getTypeName() { return m_type_name; }

        int TypeMeta::getFieldsList(FieldAccessor*& out_list)
        {
            int count = m_fields.size();
            out_list  = new FieldAccessor[count];
            for (int i = 0; i < count; ++i)
            {
                out_list[i] = m_fields[i];
            }
            return count;
        }

        int TypeMeta::getBaseClassReflectionInstanceList(ReflectionInstance*& out_list, void* instance)
        {
            auto iter = m_class_map.find(m_type_name);

            if (iter != m_class_map.end())
            {
                return (std::get<0>(*iter->second))(out_list, instance);
            }

            return 0;
        }

        FieldAccessor TypeMeta::getFieldByName(const char* name)
        {
            const auto it = std::find_if(m_fields.begin(), m_fields.end(), [&](const auto& i) {
                return std::strcmp(i.getFieldName(), name) == 0;
            });
            if (it != m_fields.end())
                return *it;
            return FieldAccessor(nullptr);
        }

        TypeMeta& TypeMeta::operator=(const TypeMeta& dest)
        {
            if (this == &dest)
            {
                return *this;
            }
            m_fields.clear();
            m_fields = dest.m_fields;

            m_type_name = dest.m_type_name;
            m_is_valid  = dest.m_is_valid;

            return *this;
        }
        FieldAccessor::FieldAccessor()
        {
            m_field_type_name = k_unknown_type;
            m_field_name      = k_unknown;
            m_functions       = nullptr;
        }

        FieldAccessor::FieldAccessor(FieldFunctionTuple* functions) : m_functions(functions)
        {
            m_field_type_name = k_unknown_type;
            m_field_name      = k_unknown;
            if (m_functions == nullptr)
            {
                return;
            }

            m_field_type_name = (std::get<4>(*m_functions))();
            m_field_name      = (std::get<3>(*m_functions))();

            //[CR] 解析 meta tags 字符串，提取所有的 <tag_name, tag_value>。
            std::string meta_tags_str = std::get<6>(*m_functions)();
            auto&& tags = split(meta_tags_str, ",");
            for (auto& tag : tags)
            {
                auto&& kv = split(tag, ":");
                auto tag_name = kv[0];
                auto&& tag_value = kv.size() > 1 ? kv[1] : "";
                m_field_tags[tag_name] = tag_value;
            }
            //LOG_INFO("### m_field_name: {}, meta_tags_str: {}", m_field_name, meta_tags_str);
        }

        void* FieldAccessor::get(void* instance)
        {
            // todo: should check validation
            return static_cast<void*>((std::get<1>(*m_functions))(instance));
        }

        void FieldAccessor::set(void* instance, void* value)
        {
            // todo: should check validation
            (std::get<0>(*m_functions))(instance, value);
        }

        TypeMeta FieldAccessor::getOwnerTypeMeta()
        {
            // todo: should check validation
            TypeMeta f_type((std::get<2>(*m_functions))());
            return f_type;
        }

        bool FieldAccessor::getTypeMeta(TypeMeta& field_type)
        {
            TypeMeta f_type(m_field_type_name);
            field_type = f_type;
            return f_type.m_is_valid;
        }

        const char* FieldAccessor::getFieldName() const { return m_field_name; }
        const char* FieldAccessor::getFieldTypeName() { return m_field_type_name; }

        bool FieldAccessor::isArrayType()
        {
            // todo: should check validation
            return (std::get<5>(*m_functions))();
        }

        bool FieldAccessor::hasMetaTag(const char* name) const
        {
            return m_field_tags.find(name) != m_field_tags.end();
        }

        std::string FieldAccessor::getMetaTagValue(const char* name) const
        {
            auto search = m_field_tags.find(name);
            return search == m_field_tags.end() ? "" : search->second;
        }

        FieldAccessor& FieldAccessor::operator=(const FieldAccessor& dest)
        {
            if (this == &dest)
            {
                return *this;
            }
            m_functions       = dest.m_functions;
            m_field_name      = dest.m_field_name;
            m_field_type_name = dest.m_field_type_name;

            //[CR] NOTE: 容易被遗漏的地方：对 m_field_tags 进行拷贝构造。
            m_field_tags.clear();
            for (auto it = dest.m_field_tags.begin(); it != dest.m_field_tags.end(); ++it)
            {
                m_field_tags[it->first] = it->second;
            }
            
            return *this;
        }

        ArrayAccessor::ArrayAccessor() :
            m_func(nullptr), m_array_type_name("UnKnownType"), m_element_type_name("UnKnownType")
        {}

        ArrayAccessor::ArrayAccessor(ArrayFunctionTuple* array_func) : m_func(array_func)
        {
            m_array_type_name   = k_unknown_type;
            m_element_type_name = k_unknown_type;
            if (m_func == nullptr)
            {
                return;
            }

            m_array_type_name   = std::get<3>(*m_func)();
            m_element_type_name = std::get<4>(*m_func)();
        }
        const char* ArrayAccessor::getArrayTypeName() { return m_array_type_name; }
        const char* ArrayAccessor::getElementTypeName() { return m_element_type_name; }
        void        ArrayAccessor::set(int index, void* instance, void* element_value)
        {
            // todo: should check validation
            size_t count = getSize(instance);
            // todo: should check validation(index < count)
            std::get<0> (*m_func)(index, instance, element_value);
        }

        void* ArrayAccessor::get(int index, void* instance)
        {
            // todo: should check validation
            size_t count = getSize(instance);
            // todo: should check validation(index < count)
            return std::get<1>(*m_func)(index, instance);
        }

        int ArrayAccessor::getSize(void* instance)
        {
            // todo: should check validation
            return std::get<2>(*m_func)(instance);
        }

        ArrayAccessor& ArrayAccessor::operator=(ArrayAccessor& dest)
        {
            if (this == &dest)
            {
                return *this;
            }
            m_func              = dest.m_func;
            m_array_type_name   = dest.m_array_type_name;
            m_element_type_name = dest.m_element_type_name;
            return *this;
        }

        ReflectionInstance& ReflectionInstance::operator=(ReflectionInstance& dest)
        {
            if (this == &dest)
            {
                return *this;
            }
            m_instance = dest.m_instance;
            m_meta     = dest.m_meta;

            return *this;
        }

        ReflectionInstance& ReflectionInstance::operator=(ReflectionInstance&& dest)
        {
            if (this == &dest)
            {
                return *this;
            }
            m_instance = dest.m_instance;
            m_meta     = dest.m_meta;

            return *this;
        }
    } // namespace Reflection
} // namespace Piccolo