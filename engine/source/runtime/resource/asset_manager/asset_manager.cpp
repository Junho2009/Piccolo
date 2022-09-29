#include "runtime/resource/asset_manager/asset_manager.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/global/global_context.h"

#include <filesystem>

namespace Piccolo
{
    std::filesystem::path AssetManager::getFullPath(const std::string& relative_path) const
    {
        /** [CR]
         * 有关 std::filesystem：
         *   C++ 资料：https://en.cppreference.com/w/cpp/filesystem
         *   从 C++17 开始支持；最初是从 boost.filesystem 发展而来的；当前 boost 的版本，适应范围（包括编译器、操作系统）比 C++17 这个版本更广。
         *   里面的函数不是在所有情况下都能用，需要视 OS、文件系统而定。比如，FAT系统 不支持 符号链接、多个硬链接 等特性。
         *
         *   扩展知识：不同操作系统、编译器，其 C++标准库 的实现会有差别。
         *     比如这个”获取绝对路径“的函数，在 Windows 系统下可能是调用 GetFullPathNameW 函数来获得。
         *
         * 这个函数的作用是，将 "..getRootFolder() / relative_path" 中的所有相对路径（比如"../"）去掉，变成当前系统下的绝对路径格式。
         */
        return std::filesystem::absolute(g_runtime_global_context.m_config_manager->getRootFolder() / relative_path);
    }
} // namespace Piccolo