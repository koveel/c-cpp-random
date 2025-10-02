#pragma once

template<typename... Ts>
struct Tuple
{
};

template<>
struct Tuple<>
{
};

template<typename T0, typename... Ts>
struct Tuple<T0, Ts...> : Tuple<Ts...>
{
	T0 element;

	Tuple(T0&& t0, Ts&&... ts)
		: element(t0), Tuple<Ts...>(std::forward<Ts>(ts)...)
	{
	}
};

template<size_t N, typename T0, typename... Ts>
static decltype(auto) get(const Tuple<T0, Ts...>& tuple)
{
	static_assert(N >= 0);
	static_assert(N < sizeof...(Ts) + 1);

	if constexpr (N <= 0)
		return tuple.element;
	else
		return get<N - 1>(static_cast<const Tuple<Ts...>&>(tuple));
}