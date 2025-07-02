#pragma once

#include <algorithm>
#include <type_traits>
#include <memory>

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

template<typename T>
struct callable_traits_impl { using fn_ptr = void; };

template<typename R, typename... Args>
struct callable_traits_impl<R(*)(Args...)> {
	using fn_ptr = R(*)(Args...);
	using return_ty = R;
	using args_ty = std::tuple<Args...>;
};
template<typename F, typename R, typename... Args>
struct callable_traits_impl<R(F::*)(Args...)> {
	using fn_ptr    = R(F::*)(Args...);
	using return_ty = R;
	using args_ty   = std::tuple<Args...>;
};
template<typename F, typename R, typename... Args>
struct callable_traits_impl<R(F::*)(Args...) const> {
	using fn_ptr = R(F::*)(Args...) const;
	using return_ty = R;
	using args_ty = std::tuple<Args...>;
};

template<typename F>
struct callable_traits_t { using type = callable_traits_impl<decltype(&F::operator())>; };

template<typename R, typename... Args>
struct callable_traits_t<R(*)(Args...)> { using type = callable_traits_impl<R(*)(Args...)>; };

template<typename F, typename R, typename... Args>
struct callable_traits_t<R(F::*)(Args...)> { using type = callable_traits_impl<R(F::*)(Args...)>; };

template<typename F>
using callable_traits = typename callable_traits_t<F>::type;

// Represents a function pointer / functor of any type, invokable with arguments
// Can be bound to free-floating functions or member functions without any heap allocations. Can be used with lambdas but added overhead of a heap allocation
// For functions that return a value, must specify return type when you invoke
// 
// Error checking is basically non-existent (don't call a function with incorrect arguments!!)
class Function
{
private:
	struct ILambda
	{
		virtual ~ILambda() {}
		virtual void invoke_erased(void* return_value, void* erased_tuple_args) = 0;
	};

	template<typename F>
	struct LambdaImpl : ILambda
	{
		F lambda;

		LambdaImpl(F lambda)
			: lambda(lambda)
		{}

		// Voodoo
		virtual void invoke_erased(void* return_value, void* erased_tuple_args) override
		{
			// TODO: some assertions at least?
			using Args = callable_traits<F>::args_ty;
			using Return = callable_traits<F>::return_ty;

			Args* args = static_cast<Args*>(erased_tuple_args);
			if constexpr (std::is_void_v<Return>) {
				std::apply(lambda, *args);
			}
			else {
				Return* value = static_cast<Return*>(return_value);
				*value = std::apply(lambda, *args);
			}
		}
	};
private:
	template<typename R, typename... Args>
	using FunctionPtr = R(*)(Args...); // Non-member function
	template<typename T, typename R, typename... Args>
	using MembFuncPtr = R(T::*)(Args...); // Member function

	static constexpr size_t FunctionPtrRequiredStorage = std::max(sizeof(FunctionPtr<void>), sizeof(MembFuncPtr<Function, void>));
	uint8_t m_Storage[FunctionPtrRequiredStorage]{};

	void* m_FnPtr = nullptr;
	bool m_IsLambda = false;
public:
	// Cant assign Function = Function yet
	Function() = default;
	~Function()
	{
		if (!m_IsLambda)
			return;

		delete m_FnPtr;
	}

	template<typename Lambda>
	Function(Lambda l)
	{
		m_IsLambda = true;
		m_FnPtr = new LambdaImpl(l);
	}

	// Initialize with free floating function
	template<typename R, typename... Args>
	Function(FunctionPtr<R, Args...> fn)
	{
		m_FnPtr = new (m_Storage) decltype(fn)(fn);
	}
	// Initialize with member function	
	template<typename T, typename R, typename... Args>
	Function(MembFuncPtr<T, R, Args...> fn)
	{
		m_FnPtr = new (m_Storage) decltype(fn)(fn);
	}

	operator bool() const { return m_Storage; }

	template<typename R = void, typename... Args>
	R operator()(Args&&... args) const
	{
		return invoke<R, Args...>(std::forward<Args>(args)...);
	}	

	template<typename R = void>
	R invoke() const
	{
		if (m_IsLambda)
			return invoke_lambda<R>();

		FunctionPtr<R>* function = static_cast<FunctionPtr<R>*>(m_FnPtr);
		return (*function)();
	}

	template<typename R = void, typename... Args>
	R invoke(Args&&... args) const
	{
		if (m_IsLambda)
			return invoke_lambda<R>(std::forward<Args>(args)...);

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

		FunctionPtr<R, Args...>* function = static_cast<FunctionPtr<R, Args...>*>(m_FnPtr);
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
		MembFuncPtr<T, R, Args...>* method = static_cast<MembFuncPtr<T, R, Args...>*>(function->m_FnPtr);
		return (instance.*(*method))(std::forward<Args>(args)...);
	}

	template<typename R, typename... Args>
	R invoke_lambda(Args&&... args) const
	{
		ILambda* lambda = static_cast<ILambda*>(m_FnPtr);

		std::tuple<Args...> wrapped_args = std::forward_as_tuple(args...);
		if constexpr (std::is_void_v<R>) {
			lambda->invoke_erased(nullptr, &wrapped_args);
			return;
		}
		else {
			R result;
			lambda->invoke_erased(&result, &wrapped_args);
			return result;
		}
	}
};