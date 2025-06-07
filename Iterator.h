#pragma once

// iterator for contiguous elements
template<typename T>
class Iterator
{
public:
	Iterator(T* pointer)
		: mPointer(pointer)
	{
	}
	Iterator& operator++()
	{
		mPointer++;
		return *this;
	}
	Iterator operator++(int)
	{
		Iterator it = *this;
		++mPointer;
		return it;
	}
	Iterator& operator--()
	{
		mPointer--;
		return *this;
	}
	Iterator operator--(int)
	{
		Iterator it = *this;
		--mPointer;
		return it;
	}
	uint32_t operator+(const Iterator& other) const
	{
		return static_cast<uint32_t>(mPointer + other.mPointer);
	}
	uint32_t operator-(const Iterator& other) const
	{
		return static_cast<uint32_t>(mPointer - other.mPointer);
	}

	T& operator[](uint32_t index) const
	{
		return *(mPointer + index);
	}
	T* operator->() const
	{
		return mPointer;
	}
	T& operator*() const
	{
		return *mPointer;
	}
	bool operator==(const Iterator& other) const
	{
		return mPointer == other.mPointer;
	}
	bool operator!=(const Iterator& other) const
	{
		return mPointer != other.mPointer;
	}
	bool operator<(const Iterator& other) const
	{
		return mPointer < other.mPointer;
	}
	bool operator>(const Iterator& other) const
	{
		return mPointer > other.mPointer;
	}
private:
	T* mPointer = nullptr;
};