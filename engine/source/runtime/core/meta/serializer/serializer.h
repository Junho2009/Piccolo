#pragma once
#include "runtime/core/meta/json.h"
#include "runtime/core/meta/reflection/reflection.h"

#include <cassert>

namespace Piccolo
{
    template<typename...>
    inline constexpr bool always_false = false;

    class PSerializer
    {
    public:
        template<typename T>
        static PJson writePointer(T* instance)
        {
            return PJson::object {{"$typeName", PJson {"*"}}, {"$context", PSerializer::write(*instance)}};
        }

        template<typename T>
        static T*& readPointer(const PJson& json_context, T*& instance)
        {
            assert(instance == nullptr);
            std::string type_name = json_context["$typeName"].string_value();
            assert(!type_name.empty());
            if ('*' == type_name[0])
            {
                /** [CR] 
                 * 疑问：应该也只有反射类型才会走到这个分支吧？因为接下来的 read 需要处理具体类型的反序列化。
                 *   如果不是反射类型，按目前版本的实现是找不到对应的处理逻辑的（见 static T& read(const PJson& json_context, T& instance) 这个版本）。
                 */
                instance = new T;
                read(json_context["$context"], *instance);
            }
            else
            { //[CR] 这里是专门处理反射类型的反序列化
                instance = static_cast<T*>(
                    Reflection::TypeMeta::newFromNameAndPJson(type_name, json_context["$context"]).m_instance);
            }
            return instance;
        }

        template<typename T>
        static PJson write(const Reflection::ReflectionPtr<T>& instance)
        {
            T*          instance_ptr = static_cast<T*>(instance.operator->());
            std::string type_name    = instance.getTypeName();
            return PJson::object {{"$typeName", PJson(type_name)},
                                  {"$context", Reflection::TypeMeta::writeByName(type_name, instance_ptr)}};
        }

        /** [CR]
         * ReflectionPtr 类型的 read：
         *   参照 asset/level/1-1.level.json 中的数据，以它作为理解其工作原理的例子。
         *     首先，这个 json 对应的是 LevelRes 类型的资产。它里面有 ObjectInstanceRes 类型的成员，而后者则有 ReflectionPtr<Component> 类型的容器对象。
         *     从序列化到 json 中的数据得知，这些 "反射类的指针类型"，它们所指向的对象将在本函数中构造出来——详情见 readPointer 的实现。
         */
        template<typename T>
        static T*& read(const PJson& json_context, Reflection::ReflectionPtr<T>& instance)
        {
            std::string type_name = json_context["$typeName"].string_value();
            instance.setTypeName(type_name);
            return readPointer(json_context, instance.getPtrReference());
        }

        template<typename T>
        static PJson write(const T& instance)
        {

            if constexpr (std::is_pointer<T>::value)
            {
                return writePointer((T)instance);
            }
            else
            {
                static_assert(always_false<T>, "PSerializer::write<T> has not been implemented yet!");
                return PJson();
            }
        }

        template<typename T>
        static T& read(const PJson& json_context, T& instance)
        {
            if constexpr (std::is_pointer<T>::value)
            {
                return readPointer(json_context, instance);
            }
            else
            {
                static_assert(always_false<T>, "PSerializer::read<T> has not been implemented yet!");
                return instance;
            }
        }
    };


    /** [CR]
     * 以下特化了一些基础类型的 write/read。
     */

    // implementation of base types
    template<>
    PJson PSerializer::write(const char& instance);
    template<>
    char& PSerializer::read(const PJson& json_context, char& instance);

    template<>
    PJson PSerializer::write(const int& instance);
    template<>
    int& PSerializer::read(const PJson& json_context, int& instance);

    template<>
    PJson PSerializer::write(const unsigned int& instance);
    template<>
    unsigned int& PSerializer::read(const PJson& json_context, unsigned int& instance);

    template<>
    PJson PSerializer::write(const float& instance);
    template<>
    float& PSerializer::read(const PJson& json_context, float& instance);

    template<>
    PJson PSerializer::write(const double& instance);
    template<>
    double& PSerializer::read(const PJson& json_context, double& instance);

    template<>
    PJson PSerializer::write(const bool& instance);
    template<>
    bool& PSerializer::read(const PJson& json_context, bool& instance);

    template<>
    PJson PSerializer::write(const std::string& instance);
    template<>
    std::string& PSerializer::read(const PJson& json_context, std::string& instance);

    // template<>
    // PJson PSerializer::write(const Reflection::object& instance);
    // template<>
    // Reflection::object& PSerializer::read(const PJson& json_context, Reflection::object& instance);

    ////////////////////////////////////
    ////sample of generation coder
    ////////////////////////////////////
    // class test_class
    //{
    // public:
    //     int a;
    //     unsigned int b;
    //     std::vector<int> c;
    // };
    // class ss;
    // class jkj;
    // template<>
    // PJson PSerializer::write(const ss& instance);
    // template<>
    // PJson PSerializer::write(const jkj& instance);

    /*REFLECTION_TYPE(jkj)
    CLASS(jkj,Fields)
    {
        REFLECTION_BODY(jkj);
        int jl;
    };

    REFLECTION_TYPE(ss)
    CLASS(ss:public jkj,WhiteListFields)
    {
        REFLECTION_BODY(ss);
        int jl;
    };*/

    ////////////////////////////////////
    ////template of generation coder
    ////////////////////////////////////
    // template<>
    // PJson PSerializer::write(const test_class& instance);
    // template<>
    // test_class& PSerializer::read(const PJson& json_context, test_class& instance);

    //
    ////////////////////////////////////
} // namespace Piccolo
