#pragma once

#include "common/precompiled.h"

#include "common/namespace.h"
#include "common/schema_module.h"

#include "cursor/cursor.h"

#include "generator/generator.h"
#include "template_manager/template_manager.h"

class Class;

class MetaParser
{
public:
    static void prepare(void);

    MetaParser(const std::string project_input_file,
               const std::string include_file_path,
               const std::string include_path,
               const std::string include_sys,
               const std::string module_name,
               bool              is_show_errors);
    ~MetaParser(void);
    void finish(void);
    int  parse(void);
    void generateFiles(void);

private:
    std::string m_project_input_file;

    std::vector<std::string> m_work_paths;
    std::string              m_module_name;
    std::string              m_sys_include;
    std::string              m_source_include_file_name;

    CXIndex           m_index;
    CXTranslationUnit m_translation_unit;

    std::unordered_map<std::string, std::string>  m_type_table;
    std::unordered_map<std::string, SchemaMoudle> m_schema_modules;

    std::vector<const char*>                    arguments = {{"-x",
                                           "c++",
                                           "-std=c++11",
        /*
         * [CR] 这里定义了 __REFLECTION_PARSER__ 这个宏，
         * 让引擎中的 反射宏（如 CLASS()）使用 “annotation” 这种类型的 __attribute__（这是由 CLang 提供的功能特性）。
         * 这样，填在 反射宏 中的参数，就会在 AST（语法树）中就会存在这些 “标记”。
         */
                                           "-D__REFLECTION_PARSER__",
                                           "-DNDEBUG",
                                           "-D__clang__",
                                           "-w",
                                           "-MG",
                                           "-M",
                                           "-ferror-limit=0",
                                           "-o clangLog.txt"}};
    std::vector<Generator::GeneratorInterface*> m_generators;

    bool m_is_show_errors;

private:
    /**
     * [CR] 将工程中所有的头文件都搜集起来，并以一行一行的【#include "xxx.h"】的方式，全部放在一个头文件中。
     *   比如放在：【Piccolo/build/parser_header.h】。
     *
     *   p.s.: 如何将所有头文件都找到的？是利用 “precompile.json” 这个文件。
     *     这个文件是如何生成的？可能在 precompile 阶段。暂时还没了解到。
     */
    bool        parseProject(void);
    void        buildClassAST(const Cursor& cursor, Namespace& current_namespace);
    std::string getIncludeFile(std::string name);
};
