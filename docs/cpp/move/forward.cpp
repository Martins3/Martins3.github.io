#include <iostream>
#include <type_traits>
#include <utility>

struct Widget {
};

void sink(Widget &)
{
	std::cout << "sink(Widget&)\n";
}

void sink(const Widget &)
{
	std::cout << "sink(const Widget&)\n";
}

void sink(Widget &&)
{
	std::cout << "sink(Widget&&)\n";
}

void sink(const Widget &&)
{
	std::cout << "sink(const Widget&&)\n";
}

template <typename T>
void show_deduction(T &&)
{
	using Base = std::remove_reference_t<T>;

	std::cout << "T = ";
	if constexpr (std::is_const_v<Base>)
		std::cout << "const ";
	std::cout << "Widget";
	if constexpr (std::is_lvalue_reference_v<T>)
		std::cout << '&';
	else if constexpr (std::is_rvalue_reference_v<T>)
		std::cout << "&&";

	std::cout << ", T&& collapses to ";
	if constexpr (std::is_const_v<Base>)
		std::cout << "const ";
	std::cout << "Widget";
	if constexpr (std::is_lvalue_reference_v<T &&>)
		std::cout << "&\n";
	else
		std::cout << "&&\n";
}

template <typename T>
void relay_badly(T &&value)
{
	sink(value);
}

template <typename T>
void relay_with_move(T &&value)
{
	sink(std::move(value));
}

template <typename T>
void relay_perfectly(T &&value)
{
	sink(std::forward<T>(value));
}

int main()
{
	Widget value;
	const Widget const_value;

	std::cout << "== deduction and reference collapsing ==\n";
	show_deduction(value);
	show_deduction(const_value);
	show_deduction(Widget{});
	show_deduction(std::move(const_value));

	std::cout << "\n== named parameters are lvalue expressions ==\n";
	std::cout << "relay_badly(lvalue):  ";
	relay_badly(value);
	std::cout << "relay_badly(rvalue):  ";
	relay_badly(Widget{});

	std::cout << "\n== std::move is unconditional ==\n";
	std::cout << "relay_with_move(lvalue): ";
	relay_with_move(value);
	std::cout << "relay_with_move(rvalue): ";
	relay_with_move(Widget{});

	std::cout << "\n== std::forward preserves the caller's value category ==\n";
	std::cout << "relay_perfectly(lvalue):       ";
	relay_perfectly(value);
	std::cout << "relay_perfectly(const lvalue): ";
	relay_perfectly(const_value);
	std::cout << "relay_perfectly(rvalue):       ";
	relay_perfectly(Widget{});
	std::cout << "relay_perfectly(const rvalue): ";
	relay_perfectly(std::move(const_value));

	auto &&lvalue_reference = value;
	auto &&rvalue_reference = Widget{};
	static_assert(std::is_same_v<decltype(lvalue_reference), Widget &>);
	static_assert(std::is_same_v<decltype(rvalue_reference), Widget &&>);
}
