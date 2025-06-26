#pragma once

#include <fstream>
#include <filesystem>

// Handles opening a file for reading/writing and offers methods to read and write binary data to the file
class Serializer
{
public:
	// Determines if a type has a serialize(Serializer&) function
	template<typename T, typename = void>
	struct is_serializeable_struct_t : std::false_type {};
	template <typename T>
	struct is_serializeable_struct_t<T, std::void_t<decltype(std::declval<T>().serialize(std::declval<Serializer&>()))>> : std::true_type {};

	// Determines if a type has a deserialize(Serializer&) function
	template<typename T, typename = void>
	struct is_deserializeable_struct_t : std::false_type {};
	template <typename T>
	struct is_deserializeable_struct_t<T, std::void_t<decltype(std::declval<T>().deserialize(std::declval<Serializer&>()))>> : std::true_type {};
public:
	Serializer() = default;
	Serializer(const std::string& file) { open(file); }

	~Serializer()
	{
		m_File.close();
	}

	// Opens a file to prepare for serializing / deserializing data
	void open(const std::string& file)
	{
		std::ios_base::openmode openFlags = std::ios::out | std::ios::in | std::ios::binary;
		if (!std::filesystem::exists(file))
			openFlags |= std::ios::trunc;
		
		m_File.open(file, openFlags);
		ASSERT(m_File.is_open());
	}

	// Serializes an object to the file
	template<typename T>
	void write(const T& data)
	{
		if constexpr (std::is_arithmetic_v<T>) {
			m_File.write(reinterpret_cast<const char*>(&data), sizeof(T));
		}

		if constexpr (is_serializeable_struct_t<T>::value) {
			data.serialize(*this);
		}
	}

	// Serializes a number of objects to the file
	template<typename... Ts>
	void write(const Ts&... data)
	{
		(write<Ts>(data), ...);
	}

	// Deserializes an object without advancing the stream position
	template<typename T>
	T peek()
	{
		auto start = m_File.tellg();
		T result = read<T>();
		m_File.seekg(start);

		return result;
	}

	// Deserializes an object of type T
	template<typename T>
	void read(T& result)
	{
		if constexpr (std::is_arithmetic_v<T>) {
			m_File.read(reinterpret_cast<char*>(&result), sizeof(T));
		}

		if constexpr (is_deserializeable_struct_t<T>::value) {
			result.deserialize(*this);
		}

		ASSERT(m_File.good() && "failed to deserialize data");
	}

	// Deserializes a number of objects
	template<typename... Ts>
	void read(Ts&... results)
	{
		(read<Ts>(results), ...);
	}

	template<typename T>
	T read()
	{
		T result;
		read(result);
		return result;
	}	
private:
	std::fstream m_File;
};