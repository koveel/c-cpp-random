#pragma once

// Offers access to a contiguous series of single-bit values (really a uint8_t[])
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

	void reset(bool value = false)
	{
		memset(m_Bits, value, sizeof(m_Bits));
	}

	constexpr size_t count() const { return NBits; }
};