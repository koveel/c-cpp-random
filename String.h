#pragma once

// Capacity = Length
class String
{
public:
	String() = default;
	String(const char* string)
		: m_Length(strlen(string))
	{
		copy(string, m_Length);
	}
	String(const String& other)
		: m_Length(other.m_Length)
	{
		copy(other.data(), m_Length);
	}
	String(String&& other) noexcept
	{
		m_Length = other.m_Length;
		if (using_sso())
			strcpy_s(m_SSO, m_Length + 1, other.m_SSO);
		else
		{
			m_Heap = other.m_Heap;
			other.m_Heap = nullptr;
		}
		other.m_Length = 0;
	}
	~String()
	{
		if (!using_sso())
			delete[] m_Heap;
	}

	String& operator=(const String& other)
	{
		clear();
		copy(other.data(), other.m_Length);

		return *this;
	}

	void clear()
	{
		if (!using_sso())
			delete[] m_Heap;

		memset(m_SSO, 0, sizeof(m_SSO));
		m_Length = 0;
	}

	const char* data() const { return using_sso() ? m_SSO : m_Heap; }
private:
	void copy(const char* data, size_t length)
	{
		if (using_sso())
		{
			strcpy_s(m_SSO, length + 1, data);
			return;
		}
		if (m_Heap)
			delete[] m_Heap;

		m_Heap = new char[length + 1];
		strcpy_s(m_Heap, length + 1, data);
	}

	bool using_sso() const { return m_Length < SSO_Size; }
private:
	size_t m_Length = 0;
	static constexpr size_t SSO_Size = 16;
	union
	{
		char* m_Heap;
		char m_SSO[SSO_Size]{};
	};
};

static std::ostream& operator<<(std::ostream& os, const String& str)
{
	return os << str.data();
}