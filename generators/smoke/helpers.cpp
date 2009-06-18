/*
    Generator for the SMOKE sources
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

#include <QHash>
#include <QList>

#include <type.h>

#include "globals.h"

QList<const Class*> Util::superClassList(const Class* klass)
{
    static QHash<const Class*, QList<const Class*> > superClassCache;

    QList<const Class*> ret;
    if (superClassCache.contains(klass))
        return superClassCache[klass];
    foreach (const Class::BaseClassSpecifier& base, klass->baseClasses()) {
        ret << base.baseClass;
        ret.append(superClassList(base.baseClass));
    }
    // cache
    superClassCache[klass] = ret;
    return ret;
}

QList<const Class*> Util::descendantsList(const Class* klass)
{
    static QHash<const Class*, QList<const Class*> > descendantsClassCache;

    QList<const Class*> ret;
    if (descendantsClassCache.contains(klass))
        return descendantsClassCache[klass];
    for (QHash<QString, Class>::const_iterator iter = classes.constBegin(); iter != classes.constEnd(); iter++) {
        if (superClassList(&iter.value()).contains(klass))
            ret << &iter.value();
    }
    // cache
    descendantsClassCache[klass] = ret;
    return ret;
}

void Util::preparse(QSet<Type*> *usedTypes, const QList<QString>& keys)
{
    foreach (const QString& key, keys) {
        Class& klass = classes[key];
        addCopyConstructor(&klass);
        addDefaultConstructor(&klass);
        foreach (const Method& m, klass.methods()) {
            if (m.access() == Access_private)
                continue;
            (*usedTypes) << m.type();
            foreach (const Parameter& param, m.parameters())
                (*usedTypes) << param.type();
        }
        foreach (const Field& f, klass.fields()) {
            if (f.access() == Access_private)
                continue;
            (*usedTypes) << f.type();
        }
        foreach (BasicTypeDeclaration* decl, klass.children()) {
            Enum* e = 0;
            if ((e = dynamic_cast<Enum*>(decl))) {
                Type *t = Type::registerType(Type(e));
                (*usedTypes) << t;
            }
        }
    }
}

bool Util::canClassBeInstanciated(const Class* klass)
{
    static QHash<const Class*, bool> cache;
    if (cache.contains(klass))
        return cache[klass];
    
    bool ctorFound = false, publicCtorFound = false;
    foreach (const Method& meth, klass->methods()) {
        if (meth.isConstructor()) {
            ctorFound = true;
            if (meth.access() != Access_private) {
                publicCtorFound = true;
                // this class can be instanciated, break the loop
                break;
            }
        }
    }
    
    // The class can be instanciated if it has a public constructor or no constructor at all
    // because then it has a default one generated by the compiler.
    bool ret = (publicCtorFound || !ctorFound);
    cache[klass] = ret;
    return ret;
}

bool Util::canClassBeCopied(const Class* klass)
{
    static QHash<const Class*, bool> cache;
    if (cache.contains(klass))
        return cache[klass];

    bool privateCopyCtorFound = false;
    foreach (const Method& meth, klass->methods()) {
        if (meth.access() != Access_private)
            continue;
        if (meth.isConstructor() && meth.parameters().count() == 1) {
            const Type* type = meth.parameters()[0].type();
            // c'tor should be Foo(const Foo& copy)
            if (type->isConst() && type->isRef() && type->getClass() == klass) {
                privateCopyCtorFound = true;
                break;
            }
        }
    }
    
    bool parentCanBeCopied = true;
    foreach (const Class::BaseClassSpecifier& base, klass->baseClasses()) {
        if (!canClassBeCopied(base.baseClass)) {
            parentCanBeCopied = false;
            break;
        }
    }
    
    // if the parent can be copied and we didn't find a private copy c'tor, the class is copiable
    bool ret = (parentCanBeCopied && !privateCopyCtorFound);
    cache[klass] = ret;
    return ret;
}

bool Util::hasClassVirtualDestructor(const Class* klass)
{
    static QHash<const Class*, bool> cache;
    if (cache.contains(klass))
        return cache[klass];

    bool virtualDtorFound = false;
    foreach (const Method& meth, klass->methods()) {
        if (meth.isDestructor() && meth.flags() & Method::Virtual) {
            virtualDtorFound = true;
            break;
        }
    }
    
    cache[klass] = virtualDtorFound;
    return virtualDtorFound;
}

void Util::addDefaultConstructor(Class* klass)
{
    foreach (const Method& meth, klass->methods()) {
        // if the class already has a constructor or if it has pure virtuals, there's nothing to do for us
        if (meth.isConstructor() || meth.flags() & Method::PureVirtual)
            return;
    }
    
    Type t = Type(klass);
    t.setPointerDepth(1);
    Method meth = Method(klass, klass->name(), Type::registerType(t));
    meth.setIsConstructor(true);
    klass->appendMethod(meth);
}

void Util::addCopyConstructor(Class* klass)
{
    foreach (const Method& meth, klass->methods()) {
        if (meth.isConstructor() && meth.parameters().count() == 1) {
            const Type* type = meth.parameters()[0].type();
            // found a copy c'tor? then there's nothing to do
            if (type->isConst() && type->isRef() && type->getClass() == klass)
                return;
        }
    }
    
    // if the parent can't be copied, a copy c'tor is of no use
    foreach (const Class::BaseClassSpecifier& base, klass->baseClasses()) {
        if (!canClassBeCopied(base.baseClass))
            return;
    }
    
    Type t = Type(klass);
    t.setPointerDepth(1);
    Method meth = Method(klass, klass->name(), Type::registerType(t));
    meth.setIsConstructor(true);
    // parameter is a constant reference to another object of the same types
    Type paramType = Type(klass, true); paramType.setIsRef(true);
    meth.appendParameter(Parameter("copy", Type::registerType(paramType)));
    klass->appendMethod(meth);
}

QString Util::mungedName(const Method& meth) {
    QString ret = meth.name();
    foreach (const Parameter& param, meth.parameters()) {
        const Type* type = param.type();
        if (type->pointerDepth() > 1) {
            // reference to array or hash
            ret += "?";
        } else if (type->isIntegral()|| type->getEnum()) {
            // plain scalar
            ret += "$";
        } else if (type->getClass()) {
            // object
            ret += "#";
        } else {
            // unknown
            ret += "?";
        }
    }
    return ret;
}

QString Util::stackItemField(const Type* type)
{
    if ((type->pointerDepth() > 0 || !type->isIntegral()) && !type->getEnum())
        return "s_class";
    if (type->getEnum())
        return "s_enum";
    
    QString typeName = type->name();
    typeName.replace("unsigned ", "u");
    typeName.replace("signed ", "");
    typeName.replace("long long", "long");
    typeName.replace("long double", "double");
    return "s_" + typeName;
}

QString Util::assignmentString(const Type* type, const QString& var)
{
    if (type->pointerDepth() > 0) {
        return "(void*)" + var;
    } else if (type->isIntegral()) {
        return var;
    } else if (type->isRef()) {
        return "(void*)&" + var;
    } else if (type->getEnum()) {
        return var;
    } else {
        QString ret = "(void*)new ";
        if (Class* retClass = type->getClass())
            ret += retClass->toString();
        else if (Typedef* retTdef = type->getTypedef())
            ret += retTdef->toString(); 
        else
            ret += type->toString();
        ret += '(' + var + ')';
        return ret;
    }
    return QString();
}

QList<const Method*> Util::collectVirtualMethods(const Class* klass)
{
    QList<const Method*> methods;
    foreach (const Method& meth, klass->methods()) {
        if ((meth.flags() & Method::Virtual || meth.flags() & Method::PureVirtual) && !meth.isDestructor())
            methods << &meth;
    }
    foreach (const Class::BaseClassSpecifier& baseClass, klass->baseClasses()) {
        methods.append(collectVirtualMethods(baseClass.baseClass));
    }
    return methods;
}
