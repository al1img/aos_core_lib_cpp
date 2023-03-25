/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_ARRAY_HPP_
#define AOS_ARRAY_HPP_

#include <assert.h>
#include <new>

#include "aos/common/tools/buffer.hpp"
#include "aos/common/tools/error.hpp"

namespace aos {

/**
 * Array instance.
 * @tparam T array type.
 * @tparam cMaxSize max array size.
 */
template <typename T>
class Array {
public:
    /**
     * Crates empty array instance.
     */
    Array()
        : mItems(nullptr)
        , mSize(0)
        , mMaxSize(0)
    {
    }

    /**
     * Crates array instance over the buffer.
     *
     * @param buffer underlying buffer.
     * @param size current array size.
     */
    explicit Array(const Buffer& buffer)
        : mSize(0)
    {
        SetBuffer(buffer);
    }

    /**
     * Creates array instance from C array.
     *
     * @param items const C array.
     * @param size C array size.
     */
    Array(T* items, size_t size)
        : mItems(items)
        , mSize(size)
        , mMaxSize(size)
    {
    }

    /**
     * Creates array instance from another array.
     *
     * @param array another array instance.
     */
    Array(const Array& array)
        : mItems(array.mItems)
        , mSize(array.mSize)
        , mMaxSize(array.mMaxSize)
    {
    }

    /**
     * Assigns existing array to the current one.
     *
     * @param array existing array.
     * @return Array&.
     */
    Array& operator=(const Array& array)
    {
        assert(mItems && array.mSize <= mMaxSize);

        mSize = array.mSize;

        if (mItems == array.mItems) {
            return *this;
        }

        memcpy(static_cast<void*>(mItems), static_cast<void*>(array.mItems), array.mSize * sizeof(T));

        return *this;
    }

    /**
     * Clears array.
     */
    void Clear() { mSize = 0; }

    /**
     * Checks if array is empty.
     *
     * @return bool.
     */
    bool IsEmpty() const { return mSize == 0; }

    /**
     * Checks if arrays is full.
     *
     * @return bool.
     */
    bool IsFull() const { return mSize == mMaxSize; }

    /**
     * Returns current array size.
     *
     * @return size_t.
     */
    size_t Size() const { return mSize; }

    /**
     * Returns maximum available array size.
     *
     * @return size_t.
     */
    size_t MaxSize() const { return mMaxSize; }

    /**
     * Sets new array size.
     *
     * @param size new size.
     * @return Error.
     */
    Error Resize(size_t size, const T& value = T())
    {
        if (size > mMaxSize) {
            return ErrorEnum::eNoMemory;
        }

        if (size > mSize) {
            for (auto it = end(); it != end() + size - mSize; it++) {
                memcpy(end(), &value, sizeof(T));
            }
        }

        mSize = size;

        return ErrorEnum::eNone;
    }

    /**
     * Returns pointer to array items.
     *
     * @return T*.
     */
    T* Get(void) { return mItems; }

    /**
     * Returns pointer to const array items.
     *
     * @return const T*.
     */
    const T* Get(void) const { return mItems; }

    /**
     * Provides access to array item by index.
     *
     * @param index item index.
     * @return T&.
     */
    T& operator[](size_t index)
    {
        assert(index < mSize);

        return mItems[index];
    }

    /**
     * Provides access to array const item by index.
     *
     * @param index item index.
     * @return const T&.
     */
    const T& operator[](size_t index) const
    {
        assert(index < mSize);

        return mItems[index];
    }

    /**
     * Provides access to array item by index with boundaries check.
     *
     * @param index item index.
     * @return RetWithError<T*>.
     */
    RetWithError<T*> At(size_t index)
    {
        if (index >= mSize) {
            return {nullptr, ErrorEnum::eOutOfRange};
        }

        return &mItems[index];
    }

    /**
     * Provides access to array const item by index with boundaries check.
     *
     * @param index item index.
     * @return RetWithError<const T*>.
     */
    RetWithError<const T*> At(size_t index) const
    {
        if (index >= mSize) {
            return {nullptr, ErrorEnum::eOutOfRange};
        }

        return &mItems[index];
    }

    /**
     * Returns array first item.
     *
     * @return RetWithError<T*>.
     */
    RetWithError<T*> Front() { return At(0); }

    /**
     * Returns array const first item.
     *
     * @return RetWithError<const T*>.
     */
    RetWithError<const T*> Front() const { return At(0); }

    /**
     * Returns array last item.
     *
     * @return RetWithError<T*>.
     */
    RetWithError<T*> Back() { return At(mSize - 1); }

    /**
     * Returns array const last item.
     *
     * @return RetWithError<const T*>.
     */
    RetWithError<const T*> Back() const { return At(mSize - 1); }

    /**
     * Pushes item at the end of array.
     *
     * @param item item to push.
     * @return Error.
     */
    Error PushBack(const T& item)
    {
        if (IsFull()) {
            return ErrorEnum::eNoMemory;
        }

        memcpy(static_cast<void*>(end()), static_cast<const void*>(&item), sizeof(T));

        mSize++;

        return ErrorEnum::eNone;
    }

    /**
     * Pops item from the end of array.
     *
     * @return RetWithError<T*>.
     */
    RetWithError<T> PopBack()
    {
        if (IsEmpty()) {
            RetWithError<T>(T(), ErrorEnum::eNotFound);
        }

        RetWithError<T> result(mItems[mSize - 1], ErrorEnum::eNone);

        mSize--;

        memset(end(), 0, sizeof(T));

        return result;
    }

    /**
     * Checks if array equals to another array.
     *
     * @param array to compare with.
     * @return bool.
     */
    bool operator==(const Array& array) const
    {
        if (array.Size() != mSize) {
            return false;
        }

        return memcmp(array.Get(), mItems, mSize * sizeof(T)) == 0;
    };

    /**
     * Checks if array doesn't equal to another array.
     *
     * @param array to compare with.
     * @return bool.
     */
    bool operator!=(const Array& array) const { return !operator==(array); };

    /**
     * Inserts items from range.
     *
     * @param pos insert position.
     * @param from insert from this position.
     * @param till insert till this position.
     * @return Error.
     */
    Error Insert(T* pos, const T* from, const T* till)
    {
        auto size = till - from;

        if (mSize + size > mMaxSize) {
            return ErrorEnum::eNoMemory;
        }

        if (pos < begin() || pos > end()) {
            return ErrorEnum::eInvalidArgument;
        }

        for (auto i = 0; i < end() - pos; i++) {
            new (pos + size + i) T(*(pos + i));
        }

        for (auto i = 0; i < size; i++) {
            new (pos + i) T(*(from + i));
        }

        mSize += size;

        return ErrorEnum::eNone;
    }

    /**
     * Appends array.
     *
     * @param array array to append with.
     * @return Array&.
     */
    Array& Append(const Array& array)
    {
        auto err = Insert(end(), array.begin(), array.end());
        assert(err.IsNone());
    }

    /**
     * Appends array operator.
     *
     * @param array array to append with.
     * @return Array&.
     */
    Array& operator+=(const Array& array) { return Append(array); }

    /**
     * Finds element in array.
     *
     * @param item element to find.
     * @return RetWithError<T*>.
     */
    RetWithError<T*> Find(const T& item) const
    {
        for (auto it = mItems; it != end(); it++) {
            if (*it == item) {
                return it;
            }
        }

        return {nullptr, ErrorEnum::eNotFound};
    }

    /**
     * Finds element in array that match argument.
     *
     * @param match match function.
     * @return RetWithError<T*>.
     */
    template <typename P>
    RetWithError<T*> Find(P match) const
    {
        for (auto it = mItems; it != end(); it++) {
            if (match(*it)) {
                return it;
            }
        }

        return {nullptr, ErrorEnum::eNotFound};
    }

    /**
     * Removes item from array.
     *
     * @param item item to remove.
     * @return RetWithError<T*> pointer to next after deleted item.
     */
    RetWithError<T*> Remove(T* item)
    {
        if (item < begin() || item > end()) {
            return {nullptr, ErrorEnum::eInvalidArgument};
        }

        for (auto i = 0; i < end() - item - 1; i++) {
            new (item + i) T(*(item + 1 + i));
        }

        mSize--;

        return item;
    }

    /**
     * Removes element from array that match argument.
     *
     * @param match match function.
     * @return RetWithError<T*> pointer to end of new array.
     */
    template <typename P>
    RetWithError<T*> Remove(P match)
    {
        for (auto it = begin(); it != end();) {
            if (match(*it)) {
                auto result = Remove(it);
                if (!result.mError.IsNone()) {
                    return result;
                }

                it = result.mValue;
            } else {
                it++;
            }
        }

        return end();
    }

    // Used for range based loop.
    T*       begin(void) { return &mItems[0]; }
    T*       end(void) { return &mItems[mSize]; }
    const T* begin(void) const { return &mItems[0]; }
    const T* end(void) const { return &mItems[mSize]; }

protected:
    void SetBuffer(const Buffer& buffer, size_t maxSize = 0)
    {
        if (maxSize == 0) {
            mMaxSize = buffer.Size() / sizeof(T);
        } else {
            mMaxSize = maxSize;
        }

        assert(mMaxSize != 0);

        mItems = static_cast<T*>(buffer.Get());
    }

    void SetSize(size_t size) { mSize = size; }

private:
    T*     mItems;
    size_t mSize;
    size_t mMaxSize;
};

/**
 * Dynamic array instance.
 *
 * @tparam T type of items.
 * @tparam cMaxSize max size.
 */
template <typename T, size_t cMaxSize>
class DynamicArray : public Array<T> {
public:
    /**
     * Create dynamic array.
     */
    explicit DynamicArray()
        : mBuffer(cMaxSize * sizeof(T))
    {
        Array<T>::SetBuffer(mBuffer);
    }

    // cppcheck-suppress noExplicitConstructor
    /**
     * Creates dynamic array from another array.
     *
     * @param array array to create from.
     */
    DynamicArray(const Array<T>& array)
    {
        Array<T>::SetBuffer(mBuffer);
        Array<T>::operator=(array);
    }

private:
    DynamicBuffer mBuffer;
};

/**
 * Static array instance.
 *
 * @tparam T type of items.
 * @tparam cMaxSize max size.
 */
template <typename T, size_t cMaxSize>
class StaticArray : public Array<T> {
public:
    /**
     * Creates static array.
     */
    explicit StaticArray() { Array<T>::SetBuffer(mBuffer); }

    // cppcheck-suppress noExplicitConstructor
    /**
     * Creates static array from another array.
     *
     * @param array array to create from.
     */
    StaticArray(const Array<T>& array)
    {
        Array<T>::SetBuffer(mBuffer);
        Array<T>::operator=(array);
    }

private:
    StaticBuffer<cMaxSize * sizeof(T)> mBuffer;
};

} // namespace aos

#endif
