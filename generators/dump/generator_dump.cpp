/*
    Dump all parsed classes to stdout.
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

#include <QFileInfo>
#include <QList>

#include <type.h>

#include <iostream>

#include <QDebug>

extern "C" Q_DECL_EXPORT
void generate()
{
//extern GENERATOR_EXPORT QHash<QString, Class> classes;
//extern GENERATOR_EXPORT QHash<QString, Typedef> typedefs;
//extern GENERATOR_EXPORT QHash<QString, Enum> enums;
//extern GENERATOR_EXPORT QHash<QString, Function> functions;
//extern GENERATOR_EXPORT QHash<QString, GlobalVar> globals;
//extern GENERATOR_EXPORT QHash<QString, Type> types;

    std::map<QString, Class> sortedClasses;
    foreach (const QString& key, classes.keys()) {
        sortedClasses[key] = classes[key];
    }

    std::map<QString, Typedef> sortedTypedefs;
    foreach (const QString& key, typedefs.keys()) {
        sortedTypedefs[key] = typedefs[key];
    }

    std::map<QString, Enum> sortedEnums;
    foreach (const QString& key, enums.keys()) {
        sortedEnums[key] = enums[key];
    }

    std::map<QString, Function> sortedFunctions;
    foreach (const QString& key, functions.keys()) {
        sortedFunctions[key] = functions[key];
    }

    std::map<QString, Type> sortedTypes;
    foreach (const QString& key, types.keys()) {
        sortedTypes[key] = types[key];
    }

    qDebug() << "Classes:";
    for (const std::pair<const QString, Class>& item : sortedClasses) {
        const Class& klass = item.second;
        qDebug() << "    " << klass;
    }

    qDebug() << "Typedefs:";
    for (const std::pair<const QString, Typedef>& item : sortedTypedefs) {
        const Typedef& tdef = item.second;
        qDebug() << "    " << tdef;
    }

    qDebug() << "Enums:";
    for (const std::pair<const QString, Enum>& item : sortedEnums) {
        const Enum& e = item.second;
        qDebug() << "    " << e;
    }

    qDebug() << "Functions:";
    for (const std::pair<const QString, Function>& item : sortedFunctions) {
        const Function& function = item.second;
        qDebug() << "    " << function;
    }

    qDebug() << "Types:";
    for (const std::pair<const QString, Type>& item : sortedTypes) {
        const Type& type = item.second;
        qDebug() << "    " << type;
    }
}
