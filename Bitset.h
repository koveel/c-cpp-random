#pragma once

// Offers access to a contiguous series of single-bit values
// sizeof(Bitset<N>) = sizeof(uint8_t) * N / 8
// Doesn't bounds check !
template<size_t N>
class Bitset
{
private:
	static constexpr size_t NBits = N % 8 == 0 ? N : ((N / 8) + 1) * 8; // N rounded up to multiple of 8
	uint8_t m_Bits[NBits / 8]{};
public:
	// to allow 'bitset[n] = ...'
	struct ValueRef
	{
		bool operator=(bool value) {
			m_Bitset->set(m_Index, value);
			return value;
		}
		operator bool() const { return m_Bitset->get(m_Index); }

		size_t m_Index = 0;
		Bitset<NBits>* m_Bitset = nullptr;
	};
public:
	Bitset() = default;

	Bitset(std::initializer_list<bool> bits)
	{
		uint32_t i = 0;
		for (bool bit : bits)
			set(i++, bit);
	}

	template<size_t N2>
	Bitset(const Bitset<N2>& other)
	{
		memcpy(m_Bits, other.get_data(), std::min(NBits, N2) / 8);
	}

	void set(size_t index, bool value)
	{
		uint8_t& packed = m_Bits[index / 8];
		uint8_t mask = 0b10000000 >> (index % 8);
		packed = value ? packed | mask : packed & ~mask;
	}

	bool get(size_t index) const
	{
		uint8_t mask = 0b10000000 >> (index % 8);
		return m_Bits[index / 8] & mask;
	}

	ValueRef operator[](size_t index)
	{
		return { index, this };
	}
	bool operator[](size_t index) const
	{
		return get(index);
	}

	uint8_t* get_data() { return &m_Bits[0]; }
	const uint8_t* get_data() const { return &m_Bits[0]; }

	void reset(bool flag = false)
	{
		uint8_t value = flag ? ~0u : 0u;
		memset(m_Bits, value, sizeof(m_Bits));
	}

	constexpr size_t count() const { return NBits; }
};

// Same as Bitset<N>, but stores a dynamically-resizing buffer of uint8_t
// Only grows, with 2x growth every reallocation
struct DynamicBitset
{
private:
	uint8_t* m_Bits = nullptr;
	size_t m_CapacityBytes = 0;
public:
	// to allow 'bitset[n] = ...'
	struct ValueRef
	{
		bool operator=(bool value) {
			m_Bitset->set(m_Index, value);
			return value;
		}
		operator bool() const { return m_Bitset->get(m_Index); }

		size_t m_Index = 0;
		DynamicBitset* m_Bitset = nullptr;
	};
public:
	DynamicBitset(size_t initialCapacity = 8)
	{
		initialCapacity = initialCapacity % 8 == 0 ? initialCapacity : ((initialCapacity / 8) + 1) * 8; // round up to 8
		m_Bits = new uint8_t[m_CapacityBytes = initialCapacity / 8];
		memset(m_Bits, 0, m_CapacityBytes);
	}
	~DynamicBitset()
	{
		delete[] m_Bits;
	}

	DynamicBitset(std::initializer_list<bool> bits)
		: DynamicBitset()
	{
		uint32_t i = 0;
		for (bool bit : bits)
			set(i++, bit);
	}

	DynamicBitset(const DynamicBitset& other)
		: DynamicBitset(other.m_CapacityBytes)
	{
		memcpy(m_Bits, other.get_data(), other.m_CapacityBytes);
	}

	void set(size_t index, bool value)
	{
		size_t byte = index / 8;
		if (m_CapacityBytes <= byte)
			reallocate(std::max(m_CapacityBytes * 2, byte));

		uint8_t& packed = m_Bits[byte];
		uint8_t mask = 0b10000000 >> (index % 8);
		packed = value ? packed | mask : packed & ~mask;
	}

	bool get(size_t index) const
	{
		uint8_t mask = 0b10000000 >> (index % 8);
		return m_Bits[index / 8] & mask;
	}

	ValueRef operator[](size_t index)
	{
		return { index, this };
	}
	bool operator[](size_t index) const
	{
		return get(index);
	}

	uint8_t* get_data() { return &m_Bits[0]; }
	const uint8_t* get_data() const { return &m_Bits[0]; }

	void reset(bool value = false)
	{
		memset(m_Bits, value, m_CapacityBytes);
	}

	void resize(size_t capacityBits) // grow
	{
		size_t bytes = capacityBits / 8;
		if (m_CapacityBytes >= bytes)
			return;

		reallocate(bytes);
	}

	constexpr size_t count() const { return m_CapacityBytes * 8; }
private:
	void reallocate(size_t newCapacityBytes)
	{
		uint8_t* data = new uint8_t[newCapacityBytes];
		memset(data, 0, newCapacityBytes);
		if (m_Bits)
			memcpy(data, m_Bits, m_CapacityBytes);

		delete[] m_Bits;
		m_Bits = data;
		m_CapacityBytes = newCapacityBytes;
	}
};