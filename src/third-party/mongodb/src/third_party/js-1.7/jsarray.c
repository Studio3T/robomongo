/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set sw=4 ts=8 et tw=80:
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
 * JS array class.
 */
#include "jsstddef.h"
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsutil.h" /* Added by JSIFY */
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsconfig.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsstr.h"

/* 2^32 - 1 as a number and a string */
#define MAXINDEX 4294967295u
#define MAXSTR   "4294967295"

/*
 * Determine if the id represents an array index or an XML property index.
 *
 * An id is an array index according to ECMA by (15.4):
 *
 * "Array objects give special treatment to a certain class of property names.
 * A property name P (in the form of a string value) is an array index if and
 * only if ToString(ToUint32(P)) is equal to P and ToUint32(P) is not equal
 * to 2^32-1."
 *
 * In our implementation, it would be sufficient to check for JSVAL_IS_INT(id)
 * except that by using signed 32-bit integers we miss the top half of the
 * valid range. This function checks the string representation itself; note
 * that calling a standard conversion routine might allow strings such as
 * "08" or "4.0" as array indices, which they are not.
 */
JSBool
js_IdIsIndex(jsval id, jsuint *indexp)
{
    JSString *str;
    jschar *cp;

    if (JSVAL_IS_INT(id)) {
        jsint i;
        i = JSVAL_TO_INT(id);
        if (i < 0)
            return JS_FALSE;
        *indexp = (jsuint)i;
        return JS_TRUE;
    }

    /* NB: id should be a string, but jsxml.c may call us with an object id. */
    if (!JSVAL_IS_STRING(id))
        return JS_FALSE;

    str = JSVAL_TO_STRING(id);
    cp = JSSTRING_CHARS(str);
    if (JS7_ISDEC(*cp) && JSSTRING_LENGTH(str) < sizeof(MAXSTR)) {
        jsuint index = JS7_UNDEC(*cp++);
        jsuint oldIndex = 0;
        jsuint c = 0;
        if (index != 0) {
            while (JS7_ISDEC(*cp)) {
                oldIndex = index;
                c = JS7_UNDEC(*cp);
                index = 10*index + c;
                cp++;
            }
        }

        /* Ensure that all characters were consumed and we didn't overflow. */
        if (*cp == 0 &&
             (oldIndex < (MAXINDEX / 10) ||
              (oldIndex == (MAXINDEX / 10) && c < (MAXINDEX % 10))))
        {
            *indexp = index;
            return JS_TRUE;
        }
    }
    return JS_FALSE;
}

static JSBool
ValueIsLength(JSContext *cx, jsval v, jsuint *lengthp)
{
    jsint i;
    jsdouble d;

    if (JSVAL_IS_INT(v)) {
        i = JSVAL_TO_INT(v);
        if (i < 0) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_ARRAY_LENGTH);
            return JS_FALSE;
        }
        *lengthp = (jsuint) i;
        return JS_TRUE;
    }

    if (!js_ValueToNumber(cx, v, &d)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_ARRAY_LENGTH);
        return JS_FALSE;
    }
    if (!js_DoubleToECMAUint32(cx, d, (uint32 *)lengthp)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_ARRAY_LENGTH);
        return JS_FALSE;
    }
    if (JSDOUBLE_IS_NaN(d) || d != *lengthp) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_ARRAY_LENGTH);
        return JS_FALSE;
    }
    return JS_TRUE;
}

JSBool
js_GetLengthProperty(JSContext *cx, JSObject *obj, jsuint *lengthp)
{
    JSTempValueRooter tvr;
    jsid id;
    JSBool ok;
    jsint i;

    JS_PUSH_SINGLE_TEMP_ROOT(cx, JSVAL_NULL, &tvr);
    id = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
    ok = OBJ_GET_PROPERTY(cx, obj, id, &tvr.u.value);
    if (ok) {
        /*
         * Short-circuit, because js_ValueToECMAUint32 fails when called
         * during init time.
         */
        if (JSVAL_IS_INT(tvr.u.value)) {
            i = JSVAL_TO_INT(tvr.u.value);
            *lengthp = (jsuint)i;       /* jsuint cast does ToUint32 */
        } else {
            ok = js_ValueToECMAUint32(cx, tvr.u.value, (uint32 *)lengthp);
        }
    }
    JS_POP_TEMP_ROOT(cx, &tvr);
    return ok;
}

static JSBool
IndexToValue(JSContext *cx, jsuint index, jsval *vp)
{
    if (index <= JSVAL_INT_MAX) {
        *vp = INT_TO_JSVAL(index);
        return JS_TRUE;
    }
    return js_NewDoubleValue(cx, (jsdouble)index, vp);
}

static JSBool
BigIndexToId(JSContext *cx, JSObject *obj, jsuint index, JSBool createAtom,
             jsid *idp)
{
    jschar buf[10], *start;
    JSClass *clasp;
    JSAtom *atom;
    JS_STATIC_ASSERT((jsuint)-1 == 4294967295U);

    JS_ASSERT(index > JSVAL_INT_MAX);

    start = JS_ARRAY_END(buf);
    do {
        --start;
        *start = (jschar)('0' + index % 10);
        index /= 10;
    } while (index != 0);

    /*
     * Skip the atomization if the class is known to store atoms corresponding
     * to big indexes together with elements. In such case we know that the
     * array does not have an element at the given index if its atom does not
     * exist.
     */
    if (!createAtom &&
        ((clasp = OBJ_GET_CLASS(cx, obj)) == &js_ArrayClass ||
         clasp == &js_ArgumentsClass ||
         clasp == &js_ObjectClass)) {
        atom = js_GetExistingStringAtom(cx, start, JS_ARRAY_END(buf) - start);
        if (!atom) {
            *idp = JSVAL_VOID;
            return JS_TRUE;
        }
    } else {
        atom = js_AtomizeChars(cx, start, JS_ARRAY_END(buf) - start, 0);
        if (!atom)
            return JS_FALSE;
    }

    *idp = ATOM_TO_JSID(atom);
    return JS_TRUE;
}

/*
 * If the property at the given index exists, get its value into location
 * pointed by vp and set *hole to false. Otherwise set *hole to true and *vp
 * to JSVAL_VOID. This function assumes that the location pointed by vp is
 * properly rooted and can be used as GC-protected storage for temporaries.
 */
static JSBool
GetArrayElement(JSContext *cx, JSObject *obj, jsuint index, JSBool *hole,
                jsval *vp)
{
    jsid id;
    JSObject *obj2;
    JSProperty *prop;

    if (index <= JSVAL_INT_MAX) {
        id = INT_TO_JSID(index);
    } else {
        if (!BigIndexToId(cx, obj, index, JS_FALSE, &id))
            return JS_FALSE;
        if (id == JSVAL_VOID) {
            *hole = JS_TRUE;
            *vp = JSVAL_VOID;
            return JS_TRUE;
        }
    }

    if (!OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop))
        return JS_FALSE;
    if (!prop) {
        *hole = JS_TRUE;
        *vp = JSVAL_VOID;
    } else {
        OBJ_DROP_PROPERTY(cx, obj2, prop);
        if (!OBJ_GET_PROPERTY(cx, obj, id, vp))
            return JS_FALSE;
        *hole = JS_FALSE;
    }
    return JS_TRUE;
}

/*
 * Set the value of the property at the given index to v assuming v is rooted.
 */
static JSBool
SetArrayElement(JSContext *cx, JSObject *obj, jsuint index, jsval v)
{
    jsid id;

    if (index <= JSVAL_INT_MAX) {
        id = INT_TO_JSID(index);
    } else {
        if (!BigIndexToId(cx, obj, index, JS_TRUE, &id))
            return JS_FALSE;
        JS_ASSERT(id != JSVAL_VOID);
    }
    return OBJ_SET_PROPERTY(cx, obj, id, &v);
}

static JSBool
DeleteArrayElement(JSContext *cx, JSObject *obj, jsuint index)
{
    jsid id;
    jsval junk;

    if (index <= JSVAL_INT_MAX) {
        id = INT_TO_JSID(index);
    } else {
        if (!BigIndexToId(cx, obj, index, JS_FALSE, &id))
            return JS_FALSE;
        if (id == JSVAL_VOID)
            return JS_TRUE;
    }
    return OBJ_DELETE_PROPERTY(cx, obj, id, &junk);
}

/*
 * When hole is true, delete the property at the given index. Otherwise set
 * its value to v assuming v is rooted.
 */
static JSBool
SetOrDeleteArrayElement(JSContext *cx, JSObject *obj, jsuint index,
                        JSBool hole, jsval v)
{
    if (hole) {
        JS_ASSERT(v == JSVAL_VOID);
        return DeleteArrayElement(cx, obj, index);
    } else {
        return SetArrayElement(cx, obj, index, v);
    }
}


JSBool
js_SetLengthProperty(JSContext *cx, JSObject *obj, jsuint length)
{
    jsval v;
    jsid id;

    if (!IndexToValue(cx, length, &v))
        return JS_FALSE;
    id = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
    return OBJ_SET_PROPERTY(cx, obj, id, &v);
}

JSBool
js_HasLengthProperty(JSContext *cx, JSObject *obj, jsuint *lengthp)
{
    JSErrorReporter older;
    JSTempValueRooter tvr;
    jsid id;
    JSBool ok;

    older = JS_SetErrorReporter(cx, NULL);
    JS_PUSH_SINGLE_TEMP_ROOT(cx, JSVAL_NULL, &tvr);
    id = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
    ok = OBJ_GET_PROPERTY(cx, obj, id, &tvr.u.value);
    JS_SetErrorReporter(cx, older);
    if (ok)
        ok = ValueIsLength(cx, tvr.u.value, lengthp);
    JS_POP_TEMP_ROOT(cx, &tvr);
    return ok;
}

JSBool
js_IsArrayLike(JSContext *cx, JSObject *obj, JSBool *answerp, jsuint *lengthp)
{
    JSClass *clasp;

    clasp = OBJ_GET_CLASS(cx, obj);
    *answerp = (clasp == &js_ArgumentsClass || clasp == &js_ArrayClass);
    if (!*answerp) {
        *lengthp = 0;
        return JS_TRUE;
    }
    return js_GetLengthProperty(cx, obj, lengthp);
}

/*
 * This get function is specific to Array.prototype.length and other array
 * instance length properties.  It calls back through the class get function
 * in case some magic happens there (see call_getProperty in jsfun.c).
 */
static JSBool
array_length_getter(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    return OBJ_GET_CLASS(cx, obj)->getProperty(cx, obj, id, vp);
}

static JSBool
array_length_setter(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsuint newlen, oldlen, gap, index;
    jsid id2;
    jsval junk;
    JSObject *iter;
    JSTempValueRooter tvr;
    JSBool ok;

    if (!ValueIsLength(cx, *vp, &newlen))
        return JS_FALSE;
    if (!js_GetLengthProperty(cx, obj, &oldlen))
        return JS_FALSE;
    if (oldlen > newlen) {
        if (oldlen - newlen < (1 << 24)) {
            do {
                --oldlen;
                if (!DeleteArrayElement(cx, obj, oldlen))
                    return JS_FALSE;
            } while (oldlen != newlen);
        } else {
            /*
             * We are going to remove a lot of indexes in a presumably sparse
             * array. So instead of looping through indexes between newlen and
             * oldlen, we iterate through all properties and remove those that
             * correspond to indexes from the [newlen, oldlen) range.
             * See bug 322135.
             */
            iter = JS_NewPropertyIterator(cx, obj);
            if (!iter)
                return JS_FALSE;

            /* Protect iter against GC in OBJ_DELETE_PROPERTY. */
            JS_PUSH_TEMP_ROOT_OBJECT(cx, iter, &tvr);
            gap = oldlen - newlen;
            for (;;) {
                ok = JS_NextProperty(cx, iter, &id2);
                if (!ok)
                    break;
                if (id2 == JSVAL_VOID)
                    break;
                if (js_IdIsIndex(id2, &index) && index - newlen < gap) {
                    ok = OBJ_DELETE_PROPERTY(cx, obj, id2, &junk);
                    if (!ok)
                        break;
                }
            }
            JS_POP_TEMP_ROOT(cx, &tvr);
            if (!ok)
                return JS_FALSE;
        }
    }
    return IndexToValue(cx, newlen, vp);
}

static JSBool
array_addProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsuint index, length;

    if (!js_IdIsIndex(id, &index))
        return JS_TRUE;
    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    if (index >= length) {
        length = index + 1;
        return js_SetLengthProperty(cx, obj, length);
    }
    return JS_TRUE;
}

static JSBool
array_convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    return js_TryValueOf(cx, obj, type, vp);
}

JSClass js_ArrayClass = {
    "Array",
    JSCLASS_HAS_CACHED_PROTO(JSProto_Array),
    array_addProperty, JS_PropertyStub,   JS_PropertyStub,   JS_PropertyStub,
    JS_EnumerateStub,  JS_ResolveStub,    array_convert,     JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

enum ArrayToStringOp {
    TO_STRING,
    TO_LOCALE_STRING,
    TO_SOURCE
};

/*
 * When op is TO_STRING or TO_LOCALE_STRING sep indicates a separator to use
 * or "," when sep is NULL.
 * When op is TO_SOURCE sep must be NULL.
 */
static JSBool
array_join_sub(JSContext *cx, JSObject *obj, enum ArrayToStringOp op,
               JSString *sep, jsval *rval)
{
    JSBool ok, hole;
    jsuint length, index;
    jschar *chars, *ochars;
    size_t nchars, growth, seplen, tmplen, extratail;
    const jschar *sepstr;
    JSString *str;
    JSHashEntry *he;
    JSTempValueRooter tvr;
    JSAtom *atom;
    int stackDummy;

    if (!JS_CHECK_STACK_SIZE(cx, stackDummy)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
        return JS_FALSE;
    }

    ok = js_GetLengthProperty(cx, obj, &length);
    if (!ok)
        return JS_FALSE;

    he = js_EnterSharpObject(cx, obj, NULL, &chars);
    if (!he)
        return JS_FALSE;
#ifdef DEBUG
    growth = (size_t) -1;
#endif

    if (op == TO_SOURCE) {
        if (IS_SHARP(he)) {
#if JS_HAS_SHARP_VARS
            nchars = js_strlen(chars);
#else
            chars[0] = '[';
            chars[1] = ']';
            chars[2] = 0;
            nchars = 2;
#endif
            goto make_string;
        }

        /*
         * Always allocate 2 extra chars for closing ']' and terminating 0
         * and then preallocate 1 + extratail to include starting '['.
         */
        extratail = 2;
        growth = (1 + extratail) * sizeof(jschar);
        if (!chars) {
            nchars = 0;
            chars = (jschar *) malloc(growth);
            if (!chars)
                goto done;
        } else {
            MAKE_SHARP(he);
            nchars = js_strlen(chars);
            growth += nchars * sizeof(jschar);
            chars = (jschar *)realloc((ochars = chars), growth);
            if (!chars) {
                free(ochars);
                goto done;
            }
        }
        chars[nchars++] = '[';
        JS_ASSERT(sep == NULL);
        sepstr = NULL;  /* indicates to use ", " as separator */
        seplen = 2;
    } else {
        /*
         * Free any sharp variable definition in chars.  Normally, we would
         * MAKE_SHARP(he) so that only the first sharp variable annotation is
         * a definition, and all the rest are references, but in the current
         * case of (op != TO_SOURCE), we don't need chars at all.
         */
        if (chars)
            JS_free(cx, chars);
        chars = NULL;
        nchars = 0;
        extratail = 1;  /* allocate extra char for terminating 0 */

        /* Return the empty string on a cycle as well as on empty join. */
        if (IS_BUSY(he) || length == 0) {
            js_LeaveSharpObject(cx, NULL);
            *rval = JS_GetEmptyStringValue(cx);
            return ok;
        }

        /* Flag he as BUSY so we can distinguish a cycle from a join-point. */
        MAKE_BUSY(he);

        if (sep) {
            sepstr = JSSTRING_CHARS(sep);
            seplen = JSSTRING_LENGTH(sep);
        } else {
            sepstr = NULL;      /* indicates to use "," as separator */
            seplen = 1;
        }
    }

    /* Use rval to locally root each element value as we loop and convert. */
#define v (*rval)

    for (index = 0; index < length; index++) {
        ok = GetArrayElement(cx, obj, index, &hole, &v);
        if (!ok)
            goto done;
        if (hole ||
            (op != TO_SOURCE && (JSVAL_IS_VOID(v) || JSVAL_IS_NULL(v)))) {
            str = cx->runtime->emptyString;
        } else {
            if (op == TO_LOCALE_STRING) {
                atom = cx->runtime->atomState.toLocaleStringAtom;
                JS_PUSH_TEMP_ROOT_OBJECT(cx, NULL, &tvr);
                ok = js_ValueToObject(cx, v, &tvr.u.object) &&
                     js_TryMethod(cx, tvr.u.object, atom, 0, NULL, &v);
                JS_POP_TEMP_ROOT(cx, &tvr);
                if (!ok)
                    goto done;
                str = js_ValueToString(cx, v);
            } else if (op == TO_STRING) {
                str = js_ValueToString(cx, v);
            } else {
                JS_ASSERT(op == TO_SOURCE);
                str = js_ValueToSource(cx, v);
            }
            if (!str) {
                ok = JS_FALSE;
                goto done;
            }
        }

        /*
         * Do not append separator after the last element unless it is a hole
         * and we are in toSource. In that case we append single ",".
         */
        if (index + 1 == length)
            seplen = (hole && op == TO_SOURCE) ? 1 : 0;

        /* Allocate 1 at end for closing bracket and zero. */
        tmplen = JSSTRING_LENGTH(str);
        growth = nchars + tmplen + seplen + extratail;
        if (nchars > growth || tmplen > growth ||
            growth > (size_t)-1 / sizeof(jschar)) {
            if (chars) {
                free(chars);
                chars = NULL;
            }
            goto done;
        }
        growth *= sizeof(jschar);
        if (!chars) {
            chars = (jschar *) malloc(growth);
            if (!chars)
                goto done;
        } else {
            chars = (jschar *) realloc((ochars = chars), growth);
            if (!chars) {
                free(ochars);
                goto done;
            }
        }

        js_strncpy(&chars[nchars], JSSTRING_CHARS(str), tmplen);
        nchars += tmplen;

        if (seplen) {
            if (sepstr) {
                js_strncpy(&chars[nchars], sepstr, seplen);
            } else {
                JS_ASSERT(seplen == 1 || seplen == 2);
                chars[nchars] = ',';
                if (seplen == 2)
                    chars[nchars + 1] = ' ';
            }
            nchars += seplen;
        }
    }

  done:
    if (op == TO_SOURCE) {
        if (chars)
            chars[nchars++] = ']';
    } else {
        CLEAR_BUSY(he);
    }
    js_LeaveSharpObject(cx, NULL);
    if (!ok) {
        if (chars)
            free(chars);
        return ok;
    }

#undef v

  make_string:
    if (!chars) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }
    chars[nchars] = 0;
    JS_ASSERT(growth == (size_t)-1 || (nchars + 1) * sizeof(jschar) == growth);
    str = js_NewString(cx, chars, nchars, 0);
    if (!str) {
        free(chars);
        return JS_FALSE;
    }
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

#if JS_HAS_TOSOURCE
static JSBool
array_toSource(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
               jsval *rval)
{
    return array_join_sub(cx, obj, TO_SOURCE, NULL, rval);
}
#endif

static JSBool
array_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
               jsval *rval)
{
    return array_join_sub(cx, obj, TO_STRING, NULL, rval);
}

static JSBool
array_toLocaleString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
               jsval *rval)
{
    /*
     *  Passing comma here as the separator. Need a way to get a
     *  locale-specific version.
     */
    return array_join_sub(cx, obj, TO_LOCALE_STRING, NULL, rval);
}

static JSBool
InitArrayElements(JSContext *cx, JSObject *obj, jsuint start, jsuint end,
                  jsval *vector)
{
    while (start != end) {
        if (!SetArrayElement(cx, obj, start++, *vector++))
            return JS_FALSE;
    }
    return JS_TRUE;
}

static JSBool
InitArrayObject(JSContext *cx, JSObject *obj, jsuint length, jsval *vector)
{
    jsval v;
    jsid id;

    if (!IndexToValue(cx, length, &v))
        return JS_FALSE;
    id = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
    if (!OBJ_DEFINE_PROPERTY(cx, obj, id, v,
                             array_length_getter, array_length_setter,
                             JSPROP_PERMANENT,
                             NULL)) {
          return JS_FALSE;
    }
    if (!vector)
        return JS_TRUE;
    return InitArrayElements(cx, obj, 0, length, vector);
}

/*
 * Perl-inspired join, reverse, and sort.
 */
static JSBool
array_join(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;

    if (JSVAL_IS_VOID(argv[0])) {
        str = NULL;
    } else {
        str = js_ValueToString(cx, argv[0]);
        if (!str)
            return JS_FALSE;
        argv[0] = STRING_TO_JSVAL(str);
    }
    return array_join_sub(cx, obj, TO_STRING, str, rval);
}

static JSBool
array_reverse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval)
{
    jsuint len, half, i;
    JSBool hole, hole2;
    jsval *tmproot, *tmproot2;

    if (!js_GetLengthProperty(cx, obj, &len))
        return JS_FALSE;

    /*
     * Use argv[argc] and argv[argc + 1] as local roots to hold temporarily
     * array elements for GC-safe swap.
     */
    tmproot = argv + argc;
    tmproot2 = argv + argc + 1;
    half = len / 2;
    for (i = 0; i < half; i++) {
        if (!GetArrayElement(cx, obj, i, &hole, tmproot) ||
            !GetArrayElement(cx, obj, len - i - 1, &hole2, tmproot2) ||
            !SetOrDeleteArrayElement(cx, obj, len - i - 1, hole, *tmproot) ||
            !SetOrDeleteArrayElement(cx, obj, i, hole2, *tmproot2)) {
            return JS_FALSE;
        }
    }
    *rval = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

typedef struct HSortArgs {
    void         *vec;
    size_t       elsize;
    void         *pivot;
    JSComparator cmp;
    void         *arg;
    JSBool       fastcopy;
} HSortArgs;

static JSBool
sort_compare(void *arg, const void *a, const void *b, int *result);

static int
sort_compare_strings(void *arg, const void *a, const void *b, int *result);

static JSBool
HeapSortHelper(JSBool building, HSortArgs *hsa, size_t lo, size_t hi)
{
    void *pivot, *vec, *vec2, *arg, *a, *b;
    size_t elsize;
    JSComparator cmp;
    JSBool fastcopy;
    size_t j, hiDiv2;
    int cmp_result;

    pivot = hsa->pivot;
    vec = hsa->vec;
    elsize = hsa->elsize;
    vec2 =  (char *)vec - 2 * elsize;
    cmp = hsa->cmp;
    arg = hsa->arg;

    fastcopy = hsa->fastcopy;
#define MEMCPY(p,q,n) \
    (fastcopy ? (void)(*(jsval*)(p) = *(jsval*)(q)) : (void)memcpy(p, q, n))
#define CALL_CMP(a, b) \
    if (!cmp(arg, (a), (b), &cmp_result)) return JS_FALSE;

    if (lo == 1) {
        j = 2;
        b = (char *)vec + elsize;
        if (j < hi) {
            CALL_CMP(vec, b);
            if (cmp_result < 0)
                j++;
        }
        a = (char *)vec + (hi - 1) * elsize;
        b = (char *)vec2 + j * elsize;

        /*
         * During sorting phase b points to a member of heap that cannot be
         * bigger then biggest of vec[0] and vec[1], and cmp(a, b, arg) <= 0
         * always holds.
         */
        if (building || hi == 2) {
            CALL_CMP(a, b);
            if (cmp_result >= 0)
                return JS_TRUE;
        }

        MEMCPY(pivot, a, elsize);
        MEMCPY(a, b, elsize);
        lo = j;
    } else {
        a = (char *)vec2 + lo * elsize;
        MEMCPY(pivot, a, elsize);
    }

    hiDiv2 = hi/2;
    while (lo <= hiDiv2) {
        j = lo + lo;
        a = (char *)vec2 + j * elsize;
        b = (char *)vec + (j - 1) * elsize;
        if (j < hi) {
            CALL_CMP(a, b);
            if (cmp_result < 0)
                j++;
        }
        b = (char *)vec2 + j * elsize;
        CALL_CMP(pivot, b);
        if (cmp_result >= 0)
            break;

        a = (char *)vec2 + lo * elsize;
        MEMCPY(a, b, elsize);
        lo = j;
    }

    a = (char *)vec2 + lo * elsize;
    MEMCPY(a, pivot, elsize);

    return JS_TRUE;

#undef CALL_CMP
#undef MEMCPY

}

JSBool
js_HeapSort(void *vec, size_t nel, void *pivot, size_t elsize,
            JSComparator cmp, void *arg)
{
    HSortArgs hsa;
    size_t i;

    hsa.vec = vec;
    hsa.elsize = elsize;
    hsa.pivot = pivot;
    hsa.cmp = cmp;
    hsa.arg = arg;
    hsa.fastcopy = (cmp == sort_compare || cmp == sort_compare_strings);

    for (i = nel/2; i != 0; i--) {
        if (!HeapSortHelper(JS_TRUE, &hsa, i, nel))
            return JS_FALSE;
    }
    while (nel > 2) {
        if (!HeapSortHelper(JS_FALSE, &hsa, 1, --nel))
            return JS_FALSE;
    }

    return JS_TRUE;
}

typedef struct CompareArgs {
    JSContext   *context;
    jsval       fval;
    jsval       *localroot;     /* need one local root, for sort_compare */
} CompareArgs;

static JSBool
sort_compare(void *arg, const void *a, const void *b, int *result)
{
    jsval av = *(const jsval *)a, bv = *(const jsval *)b;
    CompareArgs *ca = (CompareArgs *) arg;
    JSContext *cx = ca->context;
    jsval fval;
    JSBool ok;

    /**
     * array_sort deals with holes and undefs on its own and they should not
     * come here.
     */
    JS_ASSERT(av != JSVAL_VOID);
    JS_ASSERT(bv != JSVAL_VOID);

    *result = 0;
    ok = JS_TRUE;
    fval = ca->fval;
    if (fval == JSVAL_NULL) {
        JSString *astr, *bstr;

        if (av != bv) {
            /*
             * Set our local root to astr in case the second js_ValueToString
             * displaces the newborn root in cx, and the GC nests under that
             * call.  Don't bother guarding the local root store with an astr
             * non-null test.  If we tag null as a string, the GC will untag,
             * null-test, and avoid dereferencing null.
             */
            astr = js_ValueToString(cx, av);
            *ca->localroot = STRING_TO_JSVAL(astr);
            if (astr && (bstr = js_ValueToString(cx, bv)))
                *result = js_CompareStrings(astr, bstr);
            else
                ok = JS_FALSE;
        }
    } else {
        jsdouble cmp;
        jsval argv[2];

        argv[0] = av;
        argv[1] = bv;
        ok = js_InternalCall(cx,
                             OBJ_GET_PARENT(cx, JSVAL_TO_OBJECT(fval)),
                             fval, 2, argv, ca->localroot);
        if (ok) {
            ok = js_ValueToNumber(cx, *ca->localroot, &cmp);

            /* Clamp cmp to -1, 0, 1. */
            if (ok) {
                if (JSDOUBLE_IS_NaN(cmp)) {
                    /*
                     * XXX report some kind of error here?  ECMA talks about
                     * 'consistent compare functions' that don't return NaN,
                     * but is silent about what the result should be.  So we
                     * currently ignore it.
                     */
                } else if (cmp != 0) {
                    *result = cmp > 0 ? 1 : -1;
                }
            }
        }
    }
    return ok;
}

static int
sort_compare_strings(void *arg, const void *a, const void *b, int *result)
{
    jsval av = *(const jsval *)a, bv = *(const jsval *)b;

    *result = (int) js_CompareStrings(JSVAL_TO_STRING(av), JSVAL_TO_STRING(bv));
    return JS_TRUE;
}

static JSBool
array_sort(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval fval, *vec, *pivotroot;
    CompareArgs ca;
    jsuint len, newlen, i, undefs;
    JSTempValueRooter tvr;
    JSBool hole, ok;

    /*
     * Optimize the default compare function case if all of obj's elements
     * have values of type string.
     */
    JSBool all_strings;

    if (argc > 0) {
        if (JSVAL_IS_PRIMITIVE(argv[0])) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_SORT_ARG);
            return JS_FALSE;
        }
        fval = argv[0];
        all_strings = JS_FALSE; /* non-default compare function */
    } else {
        fval = JSVAL_NULL;
        all_strings = JS_TRUE;  /* check for all string values */
    }

    if (!js_GetLengthProperty(cx, obj, &len))
        return JS_FALSE;
    if (len == 0) {
        *rval = OBJECT_TO_JSVAL(obj);
        return JS_TRUE;
    }

    /*
     * We need a temporary array of len jsvals to hold elements of the array.
     * Check that its size does not overflow size_t, which would allow for
     * indexing beyond the end of the malloc'd vector.
     */
    if (len > ((size_t) -1) / sizeof(jsval)) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    vec = (jsval *) JS_malloc(cx, ((size_t) len) * sizeof(jsval));
    if (!vec)
        return JS_FALSE;

    /*
     * Initialize vec as a root. We will clear elements of vec one by
     * one while increasing tvr.count when we know that the property at
     * the corresponding index exists and its value must be rooted.
     *
     * In this way when sorting a huge mostly sparse array we will not
     * access the tail of vec corresponding to properties that do not
     * exist, allowing OS to avoiding committing RAM. See bug 330812.
     *
     * After this point control must flow through label out: to exit.
     */
    JS_PUSH_TEMP_ROOT(cx, 0, vec, &tvr);

    /*
     * By ECMA 262, 15.4.4.11, a property that does not exist (which we
     * call a "hole") is always greater than an existing property with
     * value undefined and that is always greater than any other property.
     * Thus to sort holes and undefs we simply count them, sort the rest
     * of elements, append undefs after them and then make holes after
     * undefs.
     */
    undefs = 0;
    newlen = 0;
    for (i = 0; i < len; i++) {
        /* Clear vec[newlen] before including it in the rooted set. */
        vec[newlen] = JSVAL_NULL;
        tvr.count = newlen + 1;
        ok = GetArrayElement(cx, obj, i, &hole, &vec[newlen]);
        if (!ok)
            goto out;

        if (hole)
            continue;

        if (vec[newlen] == JSVAL_VOID) {
            ++undefs;
            continue;
        }

        /* We know JSVAL_IS_STRING yields 0 or 1, so avoid a branch via &=. */
        all_strings &= JSVAL_IS_STRING(vec[newlen]);

        ++newlen;
    }

    /* Here len == newlen + undefs + number_of_holes. */
    ca.context = cx;
    ca.fval = fval;
    ca.localroot = argv + argc;       /* local GC root for temporary string */
    pivotroot    = argv + argc + 1;   /* local GC root for pivot val */
    ok = js_HeapSort(vec, (size_t) newlen, pivotroot, sizeof(jsval),
                     all_strings ? sort_compare_strings : sort_compare,
                     &ca);
    if (!ok)
        goto out;

    ok = InitArrayElements(cx, obj, 0, newlen, vec);
    if (!ok)
        goto out;

  out:
    JS_POP_TEMP_ROOT(cx, &tvr);
    JS_free(cx, vec);
    if (!ok)
        return JS_FALSE;

    /* Set undefs that sorted after the rest of elements. */
    while (undefs != 0) {
        --undefs;
        if (!SetArrayElement(cx, obj, newlen++, JSVAL_VOID))
            return JS_FALSE;
    }

    /* Re-create any holes that sorted to the end of the array. */
    while (len > newlen) {
        if (!DeleteArrayElement(cx, obj, --len))
            return JS_FALSE;
    }
    *rval = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

/*
 * Perl-inspired push, pop, shift, unshift, and splice methods.
 */
static JSBool
array_push(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsuint length, newlength;

    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    newlength = length + argc;
    if (!InitArrayElements(cx, obj, length, newlength, argv))
        return JS_FALSE;

    /* Per ECMA-262, return the new array length. */
    if (!IndexToValue(cx, newlength, rval))
        return JS_FALSE;
    return js_SetLengthProperty(cx, obj, newlength);
}

static JSBool
array_pop(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsuint index;
    JSBool hole;

    if (!js_GetLengthProperty(cx, obj, &index))
        return JS_FALSE;
    if (index > 0) {
        index--;

        /* Get the to-be-deleted property's value into rval. */
        if (!GetArrayElement(cx, obj, index, &hole, rval))
            return JS_FALSE;
        if (!hole && !DeleteArrayElement(cx, obj, index))
            return JS_FALSE;
    }
    return js_SetLengthProperty(cx, obj, index);
}

static JSBool
array_shift(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsuint length, i;
    JSBool hole;

    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    if (length == 0) {
        *rval = JSVAL_VOID;
    } else {
        length--;

        /* Get the to-be-deleted property's value into rval ASAP. */
        if (!GetArrayElement(cx, obj, 0, &hole, rval))
            return JS_FALSE;

        /*
         * Slide down the array above the first element.
         */
        for (i = 0; i != length; i++) {
            if (!GetArrayElement(cx, obj, i + 1, &hole, &argv[0]))
                return JS_FALSE;
            if (!SetOrDeleteArrayElement(cx, obj, i, hole, argv[0]))
                return JS_FALSE;
        }

        /* Delete the only or last element when it exist. */
        if (!hole && !DeleteArrayElement(cx, obj, length))
            return JS_FALSE;
    }
    return js_SetLengthProperty(cx, obj, length);
}

static JSBool
array_unshift(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval)
{
    jsuint length, last;
    jsval *vp;
    JSBool hole;

    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    if (argc > 0) {
        /* Slide up the array to make room for argc at the bottom. */
        if (length > 0) {
            last = length;
            vp = argv + argc;   /* local root */
            do {
                --last;
                if (!GetArrayElement(cx, obj, last, &hole, vp) ||
                    !SetOrDeleteArrayElement(cx, obj, last + argc, hole, *vp)) {
                    return JS_FALSE;
                }
            } while (last != 0);
        }

        /* Copy from argv to the bottom of the array. */
        if (!InitArrayElements(cx, obj, 0, argc, argv))
            return JS_FALSE;

        length += argc;
        if (!js_SetLengthProperty(cx, obj, length))
            return JS_FALSE;
    }

    /* Follow Perl by returning the new array length. */
    return IndexToValue(cx, length, rval);
}

static JSBool
array_splice(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval *vp;
    jsuint length, begin, end, count, delta, last;
    jsdouble d;
    JSBool hole;
    JSObject *obj2;

    /*
     * Nothing to do if no args.  Otherwise point vp at our one explicit local
     * root and get length.
     */
    if (argc == 0)
        return JS_TRUE;
    vp = argv + argc;
    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;

    /* Convert the first argument into a starting index. */
    if (!js_ValueToNumber(cx, *argv, &d))
        return JS_FALSE;
    d = js_DoubleToInteger(d);
    if (d < 0) {
        d += length;
        if (d < 0)
            d = 0;
    } else if (d > length) {
        d = length;
    }
    begin = (jsuint)d; /* d has been clamped to uint32 */
    argc--;
    argv++;

    /* Convert the second argument from a count into a fencepost index. */
    delta = length - begin;
    if (argc == 0) {
        count = delta;
        end = length;
    } else {
        if (!js_ValueToNumber(cx, *argv, &d))
            return JS_FALSE;
        d = js_DoubleToInteger(d);
        if (d < 0)
            d = 0;
        else if (d > delta)
            d = delta;
        count = (jsuint)d;
        end = begin + count;
        argc--;
        argv++;
    }


    /*
     * Create a new array value to return.  Our ECMA v2 proposal specs
     * that splice always returns an array value, even when given no
     * arguments.  We think this is best because it eliminates the need
     * for callers to do an extra test to handle the empty splice case.
     */
    obj2 = js_NewArrayObject(cx, 0, NULL);
    if (!obj2)
        return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(obj2);

    /* If there are elements to remove, put them into the return value. */
    if (count > 0) {
        for (last = begin; last < end; last++) {
            if (!GetArrayElement(cx, obj, last, &hole, vp))
                return JS_FALSE;

            /* Copy *vp to new array unless it's a hole. */
            if (!hole && !SetArrayElement(cx, obj2, last - begin, *vp))
                return JS_FALSE;
        }

        if (!js_SetLengthProperty(cx, obj2, end - begin))
            return JS_FALSE;
    }

    /* Find the direction (up or down) to copy and make way for argv. */
    if (argc > count) {
        delta = (jsuint)argc - count;
        last = length;
        /* (uint) end could be 0, so can't use vanilla >= test */
        while (last-- > end) {
            if (!GetArrayElement(cx, obj, last, &hole, vp) ||
                !SetOrDeleteArrayElement(cx, obj, last + delta, hole, *vp)) {
                return JS_FALSE;
            }
        }
        length += delta;
    } else if (argc < count) {
        delta = count - (jsuint)argc;
        for (last = end; last < length; last++) {
            if (!GetArrayElement(cx, obj, last, &hole, vp) ||
                !SetOrDeleteArrayElement(cx, obj, last - delta, hole, *vp)) {
                return JS_FALSE;
            }
        }
        length -= delta;
    }

    /* Copy from argv into the hole to complete the splice. */
    if (!InitArrayElements(cx, obj, begin, begin + argc, argv))
        return JS_FALSE;

    /* Update length in case we deleted elements from the end. */
    return js_SetLengthProperty(cx, obj, length);
}

/*
 * Python-esque sequence operations.
 */
static JSBool
array_concat(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval *vp, v;
    JSObject *nobj, *aobj;
    jsuint length, alength, slot;
    uintN i;
    JSBool hole;

    /* Hoist the explicit local root address computation. */
    vp = argv + argc;

    /* Treat obj as the first argument; see ECMA 15.4.4.4. */
    --argv;
    JS_ASSERT(obj == JSVAL_TO_OBJECT(argv[0]));

    /* Create a new Array object and store it in the rval local root. */
    nobj = js_NewArrayObject(cx, 0, NULL);
    if (!nobj)
        return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(nobj);

    /* Loop over [0, argc] to concat args into nobj, expanding all Arrays. */
    length = 0;
    for (i = 0; i <= argc; i++) {
        v = argv[i];
        if (JSVAL_IS_OBJECT(v)) {
            aobj = JSVAL_TO_OBJECT(v);
            if (aobj && OBJ_GET_CLASS(cx, aobj) == &js_ArrayClass) {
                if (!OBJ_GET_PROPERTY(cx, aobj,
                                      ATOM_TO_JSID(cx->runtime->atomState
                                                   .lengthAtom),
                                      vp)) {
                    return JS_FALSE;
                }
                if (!ValueIsLength(cx, *vp, &alength))
                    return JS_FALSE;
                for (slot = 0; slot < alength; slot++) {
                    if (!GetArrayElement(cx, aobj, slot, &hole, vp))
                        return JS_FALSE;

                    /*
                     * Per ECMA 262, 15.4.4.4, step 9, ignore non-existent
                     * properties.
                     */
                    if (!hole && !SetArrayElement(cx, nobj, length + slot, *vp))
                        return JS_FALSE;
                }
                length += alength;
                continue;
            }
        }

        if (!SetArrayElement(cx, nobj, length, v))
            return JS_FALSE;
        length++;
    }

    return js_SetLengthProperty(cx, nobj, length);
}

static JSBool
array_slice(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval *vp;
    JSObject *nobj;
    jsuint length, begin, end, slot;
    jsdouble d;
    JSBool hole;

    /* Hoist the explicit local root address computation. */
    vp = argv + argc;

    /* Create a new Array object and store it in the rval local root. */
    nobj = js_NewArrayObject(cx, 0, NULL);
    if (!nobj)
        return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(nobj);

    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    begin = 0;
    end = length;

    if (argc > 0) {
        if (!js_ValueToNumber(cx, argv[0], &d))
            return JS_FALSE;
        d = js_DoubleToInteger(d);
        if (d < 0) {
            d += length;
            if (d < 0)
                d = 0;
        } else if (d > length) {
            d = length;
        }
        begin = (jsuint)d;

        if (argc > 1) {
            if (!js_ValueToNumber(cx, argv[1], &d))
                return JS_FALSE;
            d = js_DoubleToInteger(d);
            if (d < 0) {
                d += length;
                if (d < 0)
                    d = 0;
            } else if (d > length) {
                d = length;
            }
            end = (jsuint)d;
        }
    }

    if (begin > end)
        begin = end;

    for (slot = begin; slot < end; slot++) {
        if (!GetArrayElement(cx, obj, slot, &hole, vp))
            return JS_FALSE;
        if (!hole && !SetArrayElement(cx, nobj, slot - begin, *vp))
            return JS_FALSE;
    }
    return js_SetLengthProperty(cx, nobj, end - begin);
}

#if JS_HAS_ARRAY_EXTRAS

static JSBool
array_indexOfHelper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                    jsval *rval, JSBool isLast)
{
    jsuint length, i, stop;
    jsint direction;
    JSBool hole;

    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    if (length == 0)
        goto not_found;

    if (argc <= 1) {
        i = isLast ? length - 1 : 0;
    } else {
        jsdouble start;

        if (!js_ValueToNumber(cx, argv[1], &start))
            return JS_FALSE;
        start = js_DoubleToInteger(start);
        if (start < 0) {
            start += length;
            if (start < 0) {
                if (isLast)
                    goto not_found;
                i = 0;
            } else {
                i = (jsuint)start;
            }
        } else if (start >= length) {
            if (!isLast)
                goto not_found;
            i = length - 1;
        } else {
            i = (jsuint)start;
        }
    }

    if (isLast) {
        stop = 0;
        direction = -1;
    } else {
        stop = length - 1;
        direction = 1;
    }

    for (;;) {
        if (!GetArrayElement(cx, obj, (jsuint)i, &hole, rval))
            return JS_FALSE;
        if (!hole && js_StrictlyEqual(*rval, argv[0]))
            return js_NewNumberValue(cx, i, rval);
        if (i == stop)
            goto not_found;
        i += direction;
    }

  not_found:
    *rval = INT_TO_JSVAL(-1);
    return JS_TRUE;
}

static JSBool
array_indexOf(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval)
{
    return array_indexOfHelper(cx, obj, argc, argv, rval, JS_FALSE);
}

static JSBool
array_lastIndexOf(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                  jsval *rval)
{
    return array_indexOfHelper(cx, obj, argc, argv, rval, JS_TRUE);
}

/* Order is important; extras that use a caller's predicate must follow MAP. */
typedef enum ArrayExtraMode {
    FOREACH,
    MAP,
    FILTER,
    SOME,
    EVERY
} ArrayExtraMode;

static JSBool
array_extra(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval,
            ArrayExtraMode mode)
{
    jsval *vp, *sp, *origsp, *oldsp;
    jsuint length, newlen, i;
    JSObject *callable, *thisp, *newarr;
    void *mark;
    JSStackFrame *fp;
    JSBool ok, cond, hole;

    /* Hoist the explicit local root address computation. */
    vp = argv + argc;

    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;

    /*
     * First, get or compute our callee, so that we error out consistently
     * when passed a non-callable object.
     */
    callable = js_ValueToCallableObject(cx, &argv[0], JSV2F_SEARCH_STACK);
    if (!callable)
        return JS_FALSE;

    /*
     * Set our initial return condition, used for zero-length array cases
     * (and pre-size our map return to match our known length, for all cases).
     */
#ifdef __GNUC__ /* quell GCC overwarning */
    newlen = 0;
    newarr = NULL;
    ok = JS_TRUE;
#endif
    switch (mode) {
      case MAP:
      case FILTER:
        newlen = (mode == MAP) ? length : 0;
        newarr = js_NewArrayObject(cx, newlen, NULL);
        if (!newarr)
            return JS_FALSE;
        *rval = OBJECT_TO_JSVAL(newarr);
        break;
      case SOME:
        *rval = JSVAL_FALSE;
        break;
      case EVERY:
        *rval = JSVAL_TRUE;
        break;
      case FOREACH:
        break;
    }

    if (length == 0)
        return JS_TRUE;

    if (argc > 1) {
        if (!js_ValueToObject(cx, argv[1], &thisp))
            return JS_FALSE;
        argv[1] = OBJECT_TO_JSVAL(thisp);
    } else {
        thisp = NULL;
    }

    /* We call with 3 args (value, index, array), plus room for rval. */
    origsp = js_AllocStack(cx, 2 + 3 + 1, &mark);
    if (!origsp)
        return JS_FALSE;

    /* Lift current frame to include our args. */
    fp = cx->fp;
    oldsp = fp->sp;

    for (i = 0; i < length; i++) {
        ok = GetArrayElement(cx, obj, i, &hole, vp);
        if (!ok)
            break;
        if (hole)
            continue;

        /*
         * Push callable and 'this', then args. We must do this for every
         * iteration around the loop since js_Invoke uses origsp[0] for rval
         * storage and some native functions use origsp[1] for local rooting.
         */
        sp = origsp;
        *sp++ = OBJECT_TO_JSVAL(callable);
        *sp++ = OBJECT_TO_JSVAL(thisp);
        *sp++ = *vp;
        *sp++ = INT_TO_JSVAL(i);
        *sp++ = OBJECT_TO_JSVAL(obj);

        /* Do the call. */
        fp->sp = sp;
        ok = js_Invoke(cx, 3, JSINVOKE_INTERNAL);
        vp[1] = fp->sp[-1];
        fp->sp = oldsp;
        if (!ok)
            break;

        if (mode > MAP) {
            if (vp[1] == JSVAL_NULL) {
                cond = JS_FALSE;
            } else if (JSVAL_IS_BOOLEAN(vp[1])) {
                cond = JSVAL_TO_BOOLEAN(vp[1]);
            } else {
                ok = js_ValueToBoolean(cx, vp[1], &cond);
                if (!ok)
                    goto out;
            }
        }

        switch (mode) {
          case FOREACH:
            break;
          case MAP:
            ok = SetArrayElement(cx, newarr, i, vp[1]);
            if (!ok)
                goto out;
            break;
          case FILTER:
            if (!cond)
                break;
            /* Filter passed *vp, push as result. */
            ok = SetArrayElement(cx, newarr, newlen++, *vp);
            if (!ok)
                goto out;
            break;
          case SOME:
            if (cond) {
                *rval = JSVAL_TRUE;
                goto out;
            }
            break;
          case EVERY:
            if (!cond) {
                *rval = JSVAL_FALSE;
                goto out;
            }
            break;
        }
    }

 out:
    js_FreeStack(cx, mark);
    if (ok && mode == FILTER)
        ok = js_SetLengthProperty(cx, newarr, newlen);
    return ok;
}

static JSBool
array_forEach(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval)
{
    return array_extra(cx, obj, argc, argv, rval, FOREACH);
}

static JSBool
array_map(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
          jsval *rval)
{
    return array_extra(cx, obj, argc, argv, rval, MAP);
}

static JSBool
array_filter(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
             jsval *rval)
{
    return array_extra(cx, obj, argc, argv, rval, FILTER);
}

static JSBool
array_some(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
           jsval *rval)
{
    return array_extra(cx, obj, argc, argv, rval, SOME);
}

static JSBool
array_every(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
           jsval *rval)
{
    return array_extra(cx, obj, argc, argv, rval, EVERY);
}
#endif

static JSFunctionSpec array_methods[] = {
#if JS_HAS_TOSOURCE
    {js_toSource_str,       array_toSource,         0,0,0},
#endif
    {js_toString_str,       array_toString,         0,0,0},
    {js_toLocaleString_str, array_toLocaleString,   0,0,0},

    /* Perl-ish methods. */
    {"join",                array_join,             1,JSFUN_GENERIC_NATIVE,0},
    {"reverse",             array_reverse,          0,JSFUN_GENERIC_NATIVE,2},
    {"sort",                array_sort,             1,JSFUN_GENERIC_NATIVE,2},
    {"push",                array_push,             1,JSFUN_GENERIC_NATIVE,0},
    {"pop",                 array_pop,              0,JSFUN_GENERIC_NATIVE,0},
    {"shift",               array_shift,            0,JSFUN_GENERIC_NATIVE,1},
    {"unshift",             array_unshift,          1,JSFUN_GENERIC_NATIVE,1},
    {"splice",              array_splice,           2,JSFUN_GENERIC_NATIVE,1},

    /* Python-esque sequence methods. */
    {"concat",              array_concat,           1,JSFUN_GENERIC_NATIVE,1},
    {"slice",               array_slice,            2,JSFUN_GENERIC_NATIVE,1},

#if JS_HAS_ARRAY_EXTRAS
    {"indexOf",             array_indexOf,          1,JSFUN_GENERIC_NATIVE,0},
    {"lastIndexOf",         array_lastIndexOf,      1,JSFUN_GENERIC_NATIVE,0},
    {"forEach",             array_forEach,          1,JSFUN_GENERIC_NATIVE,2},
    {"map",                 array_map,              1,JSFUN_GENERIC_NATIVE,2},
    {"filter",              array_filter,           1,JSFUN_GENERIC_NATIVE,2},
    {"some",                array_some,             1,JSFUN_GENERIC_NATIVE,2},
    {"every",               array_every,            1,JSFUN_GENERIC_NATIVE,2},
#endif

    {0,0,0,0,0}
};

static JSBool
Array(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsuint length;
    jsval *vector;

    /* If called without new, replace obj with a new Array object. */
    if (!(cx->fp->flags & JSFRAME_CONSTRUCTING)) {
        obj = js_NewObject(cx, &js_ArrayClass, NULL, NULL);
        if (!obj)
            return JS_FALSE;
        *rval = OBJECT_TO_JSVAL(obj);
    }

    if (argc == 0) {
        length = 0;
        vector = NULL;
    } else if (argc > 1) {
        length = (jsuint) argc;
        vector = argv;
    } else if (!JSVAL_IS_NUMBER(argv[0])) {
        length = 1;
        vector = argv;
    } else {
        if (!ValueIsLength(cx, argv[0], &length))
            return JS_FALSE;
        vector = NULL;
    }
    return InitArrayObject(cx, obj, length, vector);
}

JSObject *
js_InitArrayClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto;

    proto = JS_InitClass(cx, obj, NULL, &js_ArrayClass, Array, 1,
                         NULL, array_methods, NULL, NULL);

    /* Initialize the Array prototype object so it gets a length property. */
    if (!proto || !InitArrayObject(cx, proto, 0, NULL))
        return NULL;
    return proto;
}

JSObject *
js_NewArrayObject(JSContext *cx, jsuint length, jsval *vector)
{
    JSTempValueRooter tvr;
    JSObject *obj;

    obj = js_NewObject(cx, &js_ArrayClass, NULL, NULL);
    if (!obj)
        return NULL;

    JS_PUSH_TEMP_ROOT_OBJECT(cx, obj, &tvr);
    if (!InitArrayObject(cx, obj, length, vector))
        obj = NULL;
    JS_POP_TEMP_ROOT(cx, &tvr);

    /* Set/clear newborn root, in case we lost it.  */
    cx->weakRoots.newborn[GCX_OBJECT] = (JSGCThing *) obj;
    return obj;
}
