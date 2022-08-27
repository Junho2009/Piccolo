#include "common/precompiled.h"

#include "type_info.h"

TypeInfo::TypeInfo(const Cursor& cursor, const Namespace& current_namespace) :
    m_meta_data(cursor) //[CR] 这里做了个隐式转换：将 Cursor 转换为 MetaInfo
    , m_enabled(m_meta_data.getFlag(NativeProperty::Enable))
    , m_root_cursor(cursor)
    , m_namespace(current_namespace)
{}

const MetaInfo& TypeInfo::getMetaData(void) const { return m_meta_data; }

std::string TypeInfo::getSourceFile(void) const { return m_root_cursor.getSourceFile(); }

Namespace TypeInfo::getCurrentNamespace() const { return m_namespace; }

Cursor& TypeInfo::getCurosr() { return m_root_cursor; }
