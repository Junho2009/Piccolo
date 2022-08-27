#pragma once
namespace Piccolo
{
    namespace Reflection
    {
        class TypeMetaRegister
        {
        public:
            static void Register(); //[CR] 该函数的实现（函数体）由反射系统来自动生成。
            static void Unregister();
        };
    } // namespace Reflection
} // namespace Piccolo