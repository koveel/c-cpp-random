#pragma once

#include "Iterator.h"

// simple dynamically-resizing array
// make sure to use proper indices, cause it doesn't really bounds-check
template<typename T>
class DynamicArray
{
public:
	using Iterator = Iterator<T>;
public:
	DynamicArray()
	{
		reallocate(2);
	}
	DynamicArray(uint32_t capacity)
	{
		reallocate(capacity);
	}
	~DynamicArray()
	{
		clear();
		::operator delete(mData, mCapacity * sizeof(T));
	}

	void add(const T& element)
	{
		if (mSize >= mCapacity)
			reallocate(static_cast<uint32_t>(mCapacity + mCapacity / 2));

		new (&mData[mSize++]) T(element);
	}
	void add(T&& element)
	{
		if (mSize >= mCapacity)
			reallocate(static_cast<uint32_t>(mCapacity + mCapacity / 2));

		new (&mData[mSize++]) T(std::move(element));
	}

	template<typename... Args>
	T& emplace(Args&&... args)
	{
		if (mSize >= mCapacity)
			reallocate(static_cast<uint32_t>(mCapacity + mCapacity / 2));

		new(&mData[mSize]) T(std::forward<Args>(args)...);
		return mData[mSize++];
	}

	void remove_last()
	{
		if (mSize <= 0)
			return;

		mSize--;
		mData[mSize].~T();
	}

	void remove(uint32_t index)
	{
		mData[index].~T();
		mSize--;

		for (uint32_t i = index; i < mSize; i++)
			new(&mData[i]) T(std::move(mData[i + 1]));
	}
	void remove(const Iterator& it)
	{
		uint32_t index = it - Iterator(mData);
		remove(index);
	}

	void clear()
	{
		for (uint32_t i = 0; i < mSize; i++)
			mData[i].~T();
		mSize = 0;
	}

	// shrink capacity to size (reallocates)
	void fit()
	{
		if (mSize == mCapacity)
			return;

		reallocate(mSize);
	}

	Iterator find(const T& element) const
	{
		return std::find(begin(), end(), element);
	}

	bool contains(const T& element) const
	{
		return find(element) != end();
	}

	int32_t index_of(const T& element) const
	{
		uint32_t index = 0;
		for (Iterator it = begin(); it != end(); it++)
		{
			if (*it == element)
				return index;
			index++;
		}

		return -1;
	}

	void reserve(uint32_t capacity)
	{
		if (capacity < mCapacity)
			return;

		reallocate(capacity);
	}

	T& operator[](uint32_t index)
	{
		return mData[index];
	}
	const T& operator[](uint32_t index) const
	{
		return mData[index];
	}

	uint32_t get_size() const { return mSize; }
	uint32_t get_capacity() const { return mCapacity; }

	Iterator begin() const { return { mData }; }
	Iterator end()   const { return { mData + mSize }; }
private:
	void reallocate(uint32_t newCapacity)
	{
		if (!mData)
		{
			mCapacity = newCapacity;
			mData = (T*)::operator new(mCapacity * sizeof(T));
			return;
		}

		// shrink that
		mSize = newCapacity < mSize ? newCapacity : mSize;
		//if (newCapacity < mSize)
		//	mSize = newCapacity;

		T* newData = (T*)::operator new(newCapacity * sizeof(T));
		for (uint32_t i = 0; i < mSize; i++)
		{
			new (&newData[i]) T(std::move(mData[i]));
			mData[i].~T();
		}

		::operator delete(mData, mCapacity * sizeof(T));
		mData = newData;
		mCapacity = newCapacity;
	}
private:
	uint32_t mSize = 0;
	uint32_t mCapacity = 0;
	T* mData = nullptr;
};