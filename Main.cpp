#include <iostream>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <array>

#include "Function.h"
static void print_binary(uint32_t v, uint32_t num_bytes)
{
	for (uint32_t i = (num_bytes * 8); i > 0; i--)
	{
		uint32_t mask = 1 << (i - 1);
		std::cout << ((v & mask) != 0);
	}
}

#include "ECS.h"

struct TransformComponent
{
	float x, y;
	float w, h;
};

struct PhysicsComponent
{
	int isStatic;
	float mass;
};

struct AudioComponent
{
	float volume;
	float attenuation;
};

#include "Bitset.h"

template<size_t N>
static void print_bitset(const Bitset<N>& set)
{
	printf("Bitset<%d>\n", N);
	for (uint32_t b = 0; b < set.count() / 8; b++) {
		for (uint32_t i = 0; i < 8; i++)
			printf("%d", set[(b * 8) + i]);
		printf(" ");
	}
	printf("\n");
}

static constexpr size_t hash_combine(size_t lhs, size_t rhs)
{
	if constexpr (sizeof(size_t) >= 8)
		return lhs ^ rhs + 0x517cc1b727220a95 + (lhs << 6) + (lhs >> 2);

	return lhs = rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
}

template<typename T0, typename... Ts>
struct PopFrontTs
{
	using type = std::tuple<Ts...>;
};

template <typename T0, typename... Ts>
decltype(auto) pop_front_v(T0&& t0, Ts&&... ts) {
	if constexpr (sizeof...(Ts) == 0)
		return t0;

	return std::forward<Ts...>(ts...);
}

static int get_nth_index_of(const std::string& fmt, size_t n, char c)
{
	if (n >= fmt.length())
		return -1;

	size_t nth = 0;
	for (size_t i = 0; i < fmt.length(); i++)
	{
		if (fmt[i] == c)
		{
			if (nth++ == n)
				return i;
		}
	}

	return -1;
}

// Erases leading and trailing whitespace from a string
static void string_trim_whitespace(std::string& string)
{
	string.erase(0, string.find_first_not_of(' '));
	string.erase(string.find_last_not_of(' ') + 1);
}

// Within a string, for any pair of spaces '  ', erases one of them
// "this is  a    string" -> "this is a string"
static void string_collapse_whitespace(std::string& string)
{
	for (size_t i = 0; i < string.length(); i++)
	{
		char c0 = string[i];
		char c1 = string[i + 1];
		if (c0 == ' ' && c1 == ' ')
			string.erase(i--, 1);
	}
}

class CommandHandler
{
private:
	struct ICommand
	{
		virtual ~ICommand() {}
		virtual void invoke(const std::vector<std::string>&) = 0;
	};

	template<typename... Args>
	class Command : public ICommand
	{
	public:
		using Callback = void(*)(Args...);
		Callback callback = nullptr;

		Command(Callback c)
			: callback(c) {}

		void invoke(const std::vector<std::string>& arguments) override
		{
			constexpr size_t expectedArgs = sizeof...(Args);
			if (arguments.size() != expectedArgs) {
				printf("command expects %zu arguments, got %zu\n", expectedArgs, arguments.size());
				return;
			}

			invoke_internal(arguments, std::index_sequence_for<Args...>{});
		}
	private:
		template<size_t... Is>
		void invoke_internal(const std::vector<std::string>& args, std::index_sequence<Is...>)
		{
			callback(command_parse_argument<Args>(args[Is])...);
		}

		template<typename T>
		T command_parse_argument(const std::string& stringified);

		template<>
		int command_parse_argument(const std::string& s) {
			return atoi(s.c_str());
		}
		template<>
		unsigned int command_parse_argument(const std::string& s) {
			return atoi(s.c_str());
		}
		template<>
		float command_parse_argument(const std::string& s) {
			return std::strtof(s.c_str());
		}
		template<>
		std::string command_parse_argument(const std::string& s) {
			return s;
		}
		template<>
		const std::string& command_parse_argument(const std::string& s) {
			return s;
		}
	};

	std::unordered_map<std::string, std::unique_ptr<ICommand>> command_hash_to_callback_map;
public:
	template<typename... Args>
	void listen_for(const std::string& name, void(*callback)(Args...))
	{
		command_hash_to_callback_map.emplace(name, std::make_unique<Command<Args...>>(callback));
	}

	bool command_exists(const std::string& name) const
	{
		return command_hash_to_callback_map.count(name);
	}

	bool parse_invoke_command(std::string raw) const
	{
		// Preprocess string
		string_trim_whitespace(raw);
		string_collapse_whitespace(raw);

		size_t first_space = raw.find_first_of(' ');
		bool no_arguments = first_space == std::string::npos;

		std::string name = raw.substr(0, first_space);
		if (!command_exists(name)) {
			printf("command '%s' doesn't exist\n", name.c_str());
			return false;
		}

		// extract args
		std::vector<std::string> args;
		if (!no_arguments) {
			args.reserve(std::count(raw.begin() + first_space, raw.end(), ' '));

			for (uint32_t i = 0; i < args.capacity(); i++)
			{
				uint32_t space = get_nth_index_of(raw, i, ' ') + 1;
				uint32_t next = get_nth_index_of(raw, i + 1, ' ');

				args.emplace_back(raw.substr(space, i == args.capacity() - 1 ? std::string::npos : next - space));
			}
		}		

		const auto& prototype = command_hash_to_callback_map.at(name);
		prototype->invoke(args);

		return true;
	}
};

static ECS ecs;

static void command_create()
{
	printf("created entity %d\n", ecs.create_entity());
}

static void command_destroy(uint32_t e)
{
	printf("destroyed entity %d\n", e);
	ecs.destroy_entity(e);
}

static void command_echo(const std::string& message)
{
	printf("%s\n", message.c_str());
}

// create 5 hello

int main()
{
	CommandHandler cmd;
	cmd.listen_for("create", command_create);
	cmd.listen_for("destroy", command_destroy);
	cmd.listen_for("echo", command_echo);

	char buffer[1024];
	while (std::cin.getline(buffer, 1024))
	{
		cmd.parse_invoke_command(buffer);
	}
}