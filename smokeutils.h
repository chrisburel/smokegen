/*
    Copyright (C) 2003-2011  Richard Dale <Richard_Dale@tipitina.demon.co.uk>
    Copyright (C) 2012  Arno Rehn <arno@arnorehn.de>

    Based on the PerlQt marshalling code by Ashley Winters

    This library is free software; you can redistribute and/or modify them under
    the terms of the GNU Lesser General Public License as published by the
    Free Software Foundation; either version 3 of the License, or (at your option)
    any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef SMOKEUTILS_H
#define SMOKEUTILS_H

#include <smoke.h>

#undef DEBUG
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include "marshall.h"

class SmokeQyoto;

class SmokeType {
    Smoke::Type *_t;    // derived from _smoke and _id, but cached

    Smoke *_smoke;
    Smoke::Index _id;
public:
    SmokeType() : _t(0), _smoke(0), _id(0) {}
    SmokeType(Smoke *s, Smoke::Index i) : _smoke(s), _id(i) {
        if(_id < 0 || _id > _smoke->numTypes) _id = 0;
        _t = _smoke->types + _id;
    }
    // default copy constructors are fine, this is a constant structure

    // mutators
    void set(Smoke *s, Smoke::Index i) {
        _smoke = s;
        _id = i;
        _t = _smoke->types + _id;
    }

    // accessors
    Smoke *smoke() const { return _smoke; }
    Smoke::Index typeId() const { return _id; }
    const Smoke::Type &type() const { return *_t; }
    unsigned short flags() const { return _t->flags; }
    unsigned short elem() const { return _t->flags & Smoke::tf_elem; }
    const char *name() const { return _t->name; }
    Smoke::Index classId() const { return _t->classId; }

    // tests
    bool isStack() const { return ((flags() & Smoke::tf_ref) == Smoke::tf_stack); }
    bool isPtr() const { return ((flags() & Smoke::tf_ref) == Smoke::tf_ptr); }
    bool isRef() const { return ((flags() & Smoke::tf_ref) == Smoke::tf_ref); }
    bool isConst() const { return (flags() & Smoke::tf_const); }
    bool isClass() const {
        if(elem() == Smoke::t_class)
            return classId() ? true : false;
        return false;
    }

    bool operator ==(const SmokeType &b) const {
        const SmokeType &a = *this;
        if(a.name() == b.name()) return true;
        if(a.name() && b.name() && !strcmp(a.name(), b.name()))
            return true;
        return false;
    }
    bool operator !=(const SmokeType &b) const {
        const SmokeType &a = *this;
        return !(a == b);
    }

};

class SmokeClass {
    Smoke::Class *_c;
    Smoke::ModuleIndex _mi;
public:
    SmokeClass(const SmokeType &t) {
        _mi = Smoke::ModuleIndex(t.smoke(), t.classId());
        _c = _mi.smoke->classes + _mi.index;
    }
    SmokeClass(Smoke *smoke, Smoke::Index id) : _mi(Smoke::ModuleIndex(smoke, id)) {
        _c = _mi.smoke->classes + _mi.index;
    }
    SmokeClass(const Smoke::ModuleIndex& mi) : _mi(mi) {
        _c = _mi.smoke->classes + _mi.index;
    }

    Smoke::ModuleIndex moduleIndex() const { return _mi; }
    Smoke *smoke() const { return _mi.smoke; }
    const Smoke::Class &c() const { return *_c; }
    Smoke::Index classId() const { return _mi.index; }
    const char *className() const { return _c->className; }
    Smoke::ClassFn classFn() const { return _c->classFn; }
    Smoke::EnumFn enumFn() const { return _c->enumFn; }
    bool operator ==(const SmokeClass &b) const {
        const SmokeClass &a = *this;
        if(a.className() == b.className()) return true;
        if(a.className() && b.className() && !strcmp(a.className(), b.className()))
            return true;
        return false;
    }
    bool operator !=(const SmokeClass &b) const {
        const SmokeClass &a = *this;
        return !(a == b);
    }
    bool isa(const SmokeClass &sc) const {
        return Smoke::isDerivedFrom(_mi, sc._mi);
    }

    unsigned short flags() const { return _c->flags; }
    bool hasConstructor() const { return flags() & Smoke::cf_constructor; }
    bool hasCopy() const { return flags() & Smoke::cf_deepcopy; }
    bool hasVirtual() const { return flags() & Smoke::cf_virtual; }
    bool isExternal() const { return _c->external; }
};

class SmokeMethod {
    Smoke::Method *_m;
    Smoke::ModuleIndex _id;
public:
    SmokeMethod(Smoke *smoke, Smoke::Index id) : _id(Smoke::ModuleIndex(smoke, id)) {
        _m = smoke->methods + id;
    }

    Smoke *smoke() const { return _id.smoke; }
    const Smoke::Method &m() const { return *_m; }
    SmokeClass c() const { return SmokeClass(_id.smoke, _m->classId); }
    const char *name() const { return _id.smoke->methodNames[_m->name]; }
    int numArgs() const { return _m->numArgs; }
    unsigned short flags() const { return _m->flags; }
    SmokeType arg(int i) const {
        if(i >= numArgs()) return SmokeType();
        return SmokeType(_id.smoke, _id.smoke->argumentList[_m->args + i]);
    }
    SmokeType ret() const { return SmokeType(_id.smoke, _m->ret); }
    Smoke::Index methodId() const { return _id.index; }
    Smoke::ModuleIndex moduleMethodId() const { return _id; }
    Smoke::Index method() const { return _m->method; }

    bool isStatic() const { return flags() & Smoke::mf_static; }
    bool isConst() const { return flags() & Smoke::mf_const; }
    bool isConstructor() const { return flags() & Smoke::mf_ctor; }
    bool isDestructor() const { return flags() & Smoke::mf_dtor; }

    void call(Smoke::Stack args, void *ptr = 0) const {
        Smoke::ClassFn fn = c().classFn();
        (*fn)(method(), ptr, args);
    }
};

/*
 * Type handling by moc is simple.
 *
 * If the type name matches /^(?:const\s+)?\Q$types\E&?$/, use the
 * static_QUType, where $types is join('|', qw(bool int double char* QString);
 *
 * Everything else is passed as a pointer! There are types which aren't
 * Smoke::tf_ptr but will have to be passed as a pointer. Make sure to keep
 * track of what's what.
 */

/*
 * Simply using typeids isn't enough for signals/slots. It will be possible
 * to declare signals and slots which use arguments which can't all be
 * found in a single smoke object. Instead, we need to store smoke => typeid
 * pairs. We also need additional informatation, such as whether we're passing
 * a pointer to the union element.
 */

enum MocArgumentType {
    xmoc_ptr,
    xmoc_bool,
    xmoc_int,
    xmoc_uint,
    xmoc_long,
    xmoc_ulong,
    xmoc_double,
    xmoc_charstar,
    xmoc_QString,
    xmoc_void
};

struct MocArgument {
    // smoke object and associated typeid
    SmokeType st;
    MocArgumentType argType;
};

#endif // SMOKEUTILS_H
