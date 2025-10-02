#pragma once

#include "Iterator.h"

// simple dynamically-resizing array
// make sure to use proper indices, cause it doesn't really bounds-check
template<typename T>
class DynamicArray
{
public:
	using Iterator = ContiguousIterator<T>;
public:
	DynamicArray(size_t capacity = 2)
	{
		reallocate(capacity);
	}
	DynamicArray(std::initializer_list<T> elements)
	{
		reallocate(elements.size());
		for (const T& t : elements)
			add(t);
	}
	DynamicArray(const DynamicArray& other)
	{
		reallocate(other.size());
		for (const T& t : other)
			add(t);
	}
	~DynamicArray()
	{
		clear();
		::operator delete(m_Data, m_Capacity * sizeof(T));
	}

	void add(const T& element)
	{
		if (m_Size >= m_Capacity)
			reallocate(m_Capacity + m_Capacity / 2);

		new (m_Data + m_Size++) T(element);
	}
	void add(T&& element)
	{
		if (m_Size >= m_Capacity)
			reallocate(m_Capacity + m_Capacity / 2);

		new (m_Data + m_Size++) T(std::move(element));
	}

	template<typename... Args>
	T& emplace(Args&&... args)
	{
		if (m_Size >= m_Capacity)
			reallocate(m_Capacity + m_Capacity / 2);

		new(m_Data + m_Size) T(std::forward<Args>(args)...);
		return m_Data[m_Size++];
	}	

	void remove(size_t index)
	{
		m_Data[index].~T();
		m_Size--;

		for (size_t i = index; i < m_Size; i++)
			new(m_Data + i) T(std::move(m_Data[i + 1]));
	}
	void remove(Iterator it)
	{
		size_t index = it - Iterator(m_Data);
		remove(index);
	}
	void remove_last()
	{
		if (m_Size <= 0)
			return;

		m_Size--;
		m_Data[m_Size].~T();
	}

	void clear()
	{
		for (size_t i = 0; i < m_Size; i++)
			m_Data[i].~T();
		m_Size = 0;
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
		int32_t index = 0;
		for (Iterator it = begin(); it != end(); it++)
		{
			if (*it == element)
				return index;
			index++;
		}

		return -1;
	}

	void reserve(size_t capacity)
	{
		if (capacity < m_Capacity)
			return;

		reallocate(capacity);
	}

	void resize(size_t capacity)
	{
		m_Capacity = capacity;
		reserve(m_Capacity);

		for (size_t i = m_Size; i < m_Capacity; i++)
		{
			new(m_Data + i) T();
		}
	}

	// shrink capacity to size (reallocates)
	void fit()
	{
		if (m_Size == m_Capacity)
			return;

		reallocate(m_Size);
	}

	T& operator[](size_t index)
	{
		return m_Data[index];
	}
	const T& operator[](size_t index) const
	{
		return m_Data[index];
	}

	T& last() { return operator[](m_Size - 1); }

	size_t size() const { return m_Size; }
	size_t capacity() const { return m_Capacity; }

	Iterator begin() const { return { m_Data }; }
	Iterator end()   const { return { m_Data + m_Size }; }
private:
	void reallocate(size_t newCapacity)
	{
		if (!m_Data)
		{
			m_Capacity = newCapacity;
			m_Data = (T*)::operator new(m_Capacity * sizeof(T));
			return;
		}

		m_Size = newCapacity < m_Size ? newCapacity : m_Size;

		T* newData = (T*)::operator new(newCapacity * sizeof(T));
		for (size_t i = 0; i < m_Size; i++)
		{
			new (newData + i) T(std::move(m_Data[i]));
		}

		::operator delete(m_Data, m_Capacity * sizeof(T));
		m_Data = newData;
		m_Capacity = newCapacity;
	}
private:
	size_t m_Size = 0;
	size_t m_Capacity = 0;
	T* m_Data = nullptr;
};