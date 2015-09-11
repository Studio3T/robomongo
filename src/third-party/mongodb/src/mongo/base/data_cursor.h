/*    Copyright 2014 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#pragma once

#include <cstddef>
#include <cstring>

#include "mongo/base/data_view.h"
#include "mongo/platform/endian.h"

namespace mongo {

class ConstDataCursor : public ConstDataView {
public:
    typedef ConstDataView view_type;

    ConstDataCursor(ConstDataView::bytes_type bytes) : ConstDataView(bytes) {}

    ConstDataCursor operator+(std::size_t s) const {
        return view() + s;
    }

    ConstDataCursor& operator+=(std::size_t s) {
        *this = view() + s;
        return *this;
    }

    ConstDataCursor operator-(std::size_t s) const {
        return view() - s;
    }

    ConstDataCursor& operator-=(std::size_t s) {
        *this = view() - s;
        return *this;
    }

    ConstDataCursor& operator++() {
        return operator+=(1);
    }

    ConstDataCursor operator++(int) {
        ConstDataCursor tmp = *this;
        operator++();
        return tmp;
    }

    ConstDataCursor& operator--() {
        return operator-=(1);
    }

    ConstDataCursor operator--(int) {
        ConstDataCursor tmp = *this;
        operator--();
        return tmp;
    }

    template <typename T>
    ConstDataCursor& skip() {
        *this = view() + sizeof(T);
        return *this;
    }

    template <typename T>
    ConstDataCursor& readNativeAndAdvance(T* t) {
        readNative(t);
        skip<T>();
        return *this;
    }

    template <typename T>
    T readNativeAndAdvance() {
        T out;
        readNativeAndAdvance(&out);
        return out;
    }

    template <typename T>
    T readLEAndAdvance() {
        return endian::littleToNative(readNativeAndAdvance<T>());
    }

    template <typename T>
    T readBEAndAdvance() {
        return endian::bigToNative(readNativeAndAdvance<T>());
    }
};

class DataCursor : public DataView {
public:
    typedef DataView view_type;

    DataCursor(DataView::bytes_type bytes) : DataView(bytes) {}

    operator ConstDataCursor() const {
        return view();
    }

    DataCursor operator+(std::size_t s) const {
        return view() + s;
    }

    DataCursor& operator+=(std::size_t s) {
        *this = view() + s;
        return *this;
    }

    DataCursor operator-(std::size_t s) const {
        return view() - s;
    }

    DataCursor& operator-=(std::size_t s) {
        *this = view() - s;
        return *this;
    }

    DataCursor& operator++() {
        return operator+=(1);
    }

    DataCursor operator++(int) {
        DataCursor tmp = *this;
        operator++();
        return tmp;
    }

    DataCursor& operator--() {
        return operator-=(1);
    }

    DataCursor operator--(int) {
        DataCursor tmp = *this;
        operator--();
        return tmp;
    }

    template <typename T>
    DataCursor& skip() {
        *this = view() + sizeof(T);
        return *this;
    }

    template <typename T>
    DataCursor& readNativeAndAdvance(T* t) {
        readNative(t);
        skip<T>();
        return *this;
    }

    template <typename T>
    T readNativeAndAdvance() {
        T out;
        readNativeAndAdvance(&out);
        return out;
    }

    template <typename T>
    T readLEAndAdvance() {
        return endian::littleToNative(readNativeAndAdvance<T>());
    }

    template <typename T>
    T readBEAndAdvance() {
        return endian::bigToNative(readNativeAndAdvance<T>());
    }

    template <typename T>
    DataCursor& writeNativeAndAdvance(const T& value) {
        writeNative(value);
        skip<T>();
        return *this;
    }

    template <typename T>
    DataCursor& writeLEAndAdvance(const T& value) {
        return writeNativeAndAdvance(endian::nativeToLittle(value));
    }

    template <typename T>
    DataCursor& writeBEAndAdvance(const T& value) {
        return writeNativeAndAdvance(endian::nativeToBig(value));
    }
};

}  // namespace mongo
