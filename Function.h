#pragma once

#include <type_traits>

// Template stuff to deal with parameter packs and whatnot:
template<size_t N, typename... Ts> using TypeOfNth =
typename std::tuple_element<N, std::tuple<Ts...>>::type;

template<typename... Args>
struct PackT { };

// Exposes a PackT<> of all types except the first
template<typename T0, typename... Ts>
struct PopFrontT
{
	using pack = PackT<Ts...>;
};

template <size_t N, typename... Ts>
decltype(auto) PackGetNth(Ts&&... ts) {
	return std::get<N>(std::forward_as_tuple(std::forward<Ts>(ts)...));
}

template <typename T0, typename... Ts>
decltype(auto) PopFrontV(T0&& t0, Ts&&... ts) {
	if constexpr (sizeof...(Ts) == 0)
		return t0;

	return std::forward<Ts...>(ts...);
}

// Represents a function pointer of any type, invokable with arguments
// Can be bound to free-floating functions or member functions without any heap allocations
// For functions that return a value, must specify return type when you invoke
class Function
{
private:
	template<typename R, typename... Args>
	using FunctionPtr = R(*)(Args...); // Non-member function
	template<typename T, typename R, typename... Args>
	using MembFuncPtr = R(T::*)(Args...); // Member function

	static constexpr size_t RequiredStorage = std::max(sizeof(FunctionPtr<void>), sizeof(MembFuncPtr<Function, void>));
	uint8_t m_Storage[RequiredStorage]; // Buffer for function ptr

	void* p_Function = nullptr; // FunctionPtr* or MembFuncPtr*
public:
	Function() = default;

	// Initialize with free floating function
	template<typename R, typename... Args>
	Function(FunctionPtr<R, Args...> fn)
	{
		p_Function = new (m_Storage) decltype(fn)(fn);
	}
	// Initialize with member function	
	template<typename T, typename R, typename... Args>
	Function(MembFuncPtr<T, R, Args...> fn)
	{
		p_Function = new (m_Storage) decltype(fn)(fn);
	}

	operator bool() const { return p_Function; }

	template<typename R = void, typename... Args>
	R operator()(Args&&... args) 
	{
		return invoke<R, Args...>(std::forward<Args>(args)...);
	}

	template<typename R = void>
	R invoke()
	{
		FunctionPtr<R>* function = static_cast<FunctionPtr<R>*>(p_Function);
		return (*function)();
	}

	template<typename R = void, typename... Args>
	R invoke(Args&&... args)
	{
		// Member function?
		if constexpr (std::is_class_v<std::remove_reference_t<TypeOfNth<0, Args...>>>)
		{
			if constexpr (is_invokable_as_method<R, std::remove_reference_t<TypeOfNth<0, Args...>>>(PopFrontT<Args...>::pack()))
			{
				if constexpr (sizeof...(Args) == 1)
					return invoke_method<R>(this, PackGetNth<0>(std::forward<Args>(args)...));
				else
					return invoke_method<R>(this, PackGetNth<0>(std::forward<Args>(args)...), PopFrontV(std::forward<Args>(args)...));
			}
		}

		FunctionPtr<R, Args...>* function = static_cast<FunctionPtr<R, Args...>*>(p_Function);
		return (*function)(std::forward<Args>(args)...);
	}
private:
	template<typename R, typename T, typename... Args>
	static constexpr bool is_invokable_as_method(PackT<Args...>)
	{
		return std::is_invocable_v<decltype(invoke_method<R, T, Args...>), Function*, T&, Args...>;
	}

	template<typename R, typename T, typename... Args>
	static decltype(auto) invoke_method(Function* function, T& instance, Args&&... args)
	{
		MembFuncPtr<T, R, Args...>* method = static_cast<MembFuncPtr<T, R, Args...>*>(function->p_Function);
		return (instance.*(*method))(std::forward<Args>(args)...);
	}
};