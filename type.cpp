/*
    Copyright (C) 2009 Arno Rehn <arno@arnorehn.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "type.h"
#include "options.h"

QHash<QString, Class> classes;
QHash<QString, Typedef> typedefs;
QHash<QString, Enum> enums;
QHash<QString, Function> functions;
QHash<QString, GlobalVar> globals;
QHash<QString, Type> types;

QString BasicTypeDeclaration::toString() const
{
    QString ret;
    Class* parent = m_parent;
    while (parent) {
        ret.prepend(parent->name() + "::");
        parent = parent->parent();
    }
    if (!m_nspace.isEmpty())
        ret.prepend(m_nspace + "::");
    ret += m_name;
    return ret;
}

QString Member::toString(bool withAccess, bool withClass) const
{
    QString ret;
    if (withAccess) {
        if (m_access == Access_public)
            ret += "public ";
        else if (m_access == Access_protected)
            ret += "protected ";
        else if (m_access == Access_private)
            ret += "private ";
    }
    if (m_flags & Static)
        ret += "static ";
    if (m_flags & Virtual)
        ret += "virtual ";
    ret += m_type->toString() + " ";
    if (withClass)
        ret += m_typeDecl->toString() + "::";
    ret += m_name;
    return ret;
}

QString EnumMember::toString() const
{
    QString ret;
    if (m_typeDecl->parent())
        ret += m_typeDecl->parent()->toString();
    else
        ret += m_typeDecl->nameSpace();
    return ret + "::" + name();
}

QString Parameter::toString() const
{
    return m_type->toString();
}

QString Method::toString(bool withAccess, bool withClass, bool withInitializer) const
{
    QString ret = Member::toString(withAccess, withClass);
    ret += "(";
    for (int i = 0; i < m_params.count(); i++) {
        ret += m_params[i].toString();
        if (i < m_params.count() - 1) ret += ", ";
    }
    ret += ")";
    if (m_isConst) ret += " const";
    if ((m_flags & Member::PureVirtual) && withInitializer) ret += " = 0";
    return ret;
}

const Type* Type::Void = Type::registerType(Type("void"));

#ifdef Q_OS_WIN
Type* Type::registerType(const Type& type)
{
    QString typeString = type.toString();
    QHash<QString, Type>::iterator iter = types.insert(typeString, type);
    return &iter.value();
}
#endif

Type Typedef::resolve() const {
    bool isRef = false, isConst = false, isVolatile = false;
    QList<bool> pointerDepth;

    // not pretty, but safe. 'this' (without const) will never be returned or modified from here on.
    const Type tmp(const_cast<Typedef*>(this));
    const Type* t = &tmp;

    while (t->getTypedef() && !ParserOptions::notToBeResolved.contains(t->getTypedef()->name())) {
        if (!isRef) isRef = t->isRef();
        if (!isConst) isConst = t->isConst();
        if (!isVolatile) isVolatile = t->isVolatile();
        t = t->getTypedef()->type();
        for (int i = t->pointerDepth() - 1; i >= 0; i--) {
            pointerDepth.prepend(t->isConstPointer(i));
        }
    }
    Type ret = *t;

    // not fully resolved -> erase the typedef pointer and only set a name
    if (ret.getTypedef()) {
        ret.setName(ret.getTypedef()->name());
        ret.setTypedef(0);
    }

    if (isRef) ret.setIsRef(true);
    if (isConst) ret.setIsConst(true);
    if (isVolatile) ret.setIsVolatile(true);

    ret.setPointerDepth(pointerDepth.count());
    for (int i = 0; i < pointerDepth.count(); i++) {
        ret.setIsConstPointer(i, pointerDepth[i]);
    }
    return ret;
}

QString GlobalVar::toString() const
{
    QString ret = m_type->toString() + " ";
    if (!m_nspace.isEmpty())
        ret += m_nspace + "::";
    ret += m_name;
    return ret;
}

QString Function::toString() const
{
    QString ret = GlobalVar::toString();
    ret += "(";
    for (int i = 0; i < m_params.count(); i++) {
        ret += m_params[i].type()->toString();
        if (i < m_params.count() - 1) ret += ", ";
    }
    ret += ")";
    return ret;
}

QString Type::toString(const QString& fnPtrName) const
{
    QString ret;
    if (m_isVolatile) ret += "volatile ";
    if (m_isConst) ret += "const ";
    ret += name();
    if (!m_templateArgs.isEmpty()) {
        ret += "<";
        for (int i = 0; i < m_templateArgs.count(); i++) {
            if (i > 0) ret += ',';
            ret += m_templateArgs[i].toString();
        }
        ret += ">";
    }
    
    // FIXME: This won't work for an array of function pointers!
    if (isArray() && (m_pointerDepth > 0 || m_isRef)) ret += '(';
    
    for (int i = 0; i < m_pointerDepth; i++) {
        ret += "*";
        if (isConstPointer(i)) ret += " const ";
    }
    ret = ret.trimmed();
    if (m_isRef) ret += "&";
    
    if (isArray()) ret += fnPtrName;
    if (isArray() && (m_pointerDepth > 0 || m_isRef)) ret += ')';
    
    foreach(int size, m_arrayLengths) {
        ret += '[' + QString::number(size) + ']';
    }
    
    if (m_isFunctionPointer) {
        ret += "(*" + fnPtrName + ")(";
        for (int i = 0; i < m_params.count(); i++) {
            if (i > 0) ret += ',';
            ret += m_params[i].type()->toString();
        }
        ret += ')';
    }
    // the compiler would misinterpret ">>" as the operator - replace it with "> >"
    return ret.replace(">>", "> >");
}

QDebug operator<<(QDebug debug, Access access) {
    switch (access) {
        case Access_public:
            debug << "public";
            break;
        case Access_protected:
            debug << "protected";
            break;
        case Access_private:
            debug << "private";
            break;
    };
    return debug;
}

QDebug operator<<(QDebug debug, Class::Kind kind) {
    switch (kind) {
        case Class::Kind_Class:
            debug << "class";
            break;
        case Class::Kind_Struct:
            debug << "struct";
            break;
        case Class::Kind_Union:
            debug << "union";
            break;
    }
    return debug;
}

QDebug operator<<(QDebug debug, Class::BaseClassSpecifier baseClass) {
    debug << baseClass.baseClass->toString() << ' ' << baseClass.access << ' ' << baseClass.isVirtual;
    return debug;
}

QDebug operator<<(QDebug debug, BasicTypeDeclaration klass) {
    debug << klass.toString() <<
    klass.nameSpace() <<
    (klass.parent() ? klass.parent()->toString() : "") <<
    klass.access();
    return debug;
}

QDebug operator<<(QDebug debug, Class klass) {
    debug << (BasicTypeDeclaration)klass <<
    klass.kind() <<
    klass.isForwardDecl() <<
    klass.isNameSpace() <<
    klass.baseClasses() <<
    klass.isTemplate() << "\n";

    std::map<QString, Method> sortedMethods;
    foreach (const Method& method, klass.methods()) {
        QString key = method.name();
        sortedMethods[key] = method;
    }
    for (const std::pair<const QString, Method>& item : sortedMethods) {
        debug << "        " << item.second << "\n";
    }
    return debug;
}

QDebug operator<<(QDebug debug, Method method) {
    debug << (Member)method <<
        method.parameters();
    return debug;
}

QDebug operator<<(QDebug debug, Typedef tdef) {
    debug << (BasicTypeDeclaration)tdef <<
        '[' << *tdef.type() << ']';
    return debug;
}

QDebug operator<<(QDebug debug, Member::Flags flags) {
    if (flags & Member::Virtual) {
        debug << "Virtual";
    }
    if (flags & Member::PureVirtual) {
        debug << "PureVirtual";
    }
    if (flags & Member::Static) {
        debug << "Static";
    }
    if (flags & Member::DynamicDispatch) {
        debug << "DynamicDispatch";
    }
    if (flags & Member::Explicit) {
        debug << "Explicit";
    }
    return debug;
}

QDebug operator<<(QDebug debug, Member member) {
    QString retTypeStr;
    QDebug retType(&retTypeStr);
    if (member.type()) {
        retType << *member.type();
    }

    debug << member.name() << ' ' <<
        retTypeStr << ' ' <<
        member.declaringType()->name() << ' ' <<
        member.access() << ' ' <<
        member.flags();
    return debug;
}

QDebug operator<<(QDebug debug, EnumMember e) {
    debug << (Member)e <<
        e.value();
    return debug;
}

QDebug operator<<(QDebug debug, Enum e) {
    debug << (BasicTypeDeclaration)e <<
        e.members();
    return debug;
}

QDebug operator<<(QDebug debug, GlobalVar var) {
    debug << var.qualifiedName() <<
        *var.type();
    return debug;
}

QDebug operator<<(QDebug debug, Parameter parameter) {
    debug << parameter.name() << ' ' <<
        *parameter.type() << ' ' <<
        parameter.defaultValue();
    return debug;
}

QDebug operator<<(QDebug debug, Function function) {
    debug << (GlobalVar)function <<
        function.parameters() <<
        function.toString();
    return debug;
}

QDebug operator<<(QDebug debug, Type type) {
    debug << type.name() <<
        type.isConst() <<
        type.isVolatile() <<
        type.pointerDepth() <<
        type.m_constPointer <<
        type.isRef() <<
        type.isIntegral() <<
        type.m_arrayLengths <<
        type.templateArguments() <<
        type.isFunctionPointer() <<
        type.parameters();

    return debug;
}
