#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

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

// You can add an explicit template specialization if you want arguments of a different type to be parsed
template<typename T>
T command_parse_argument(const std::string& stringified);

template<>
static int command_parse_argument(const std::string& s) {
	return std::atoi(s.c_str());
}
template<>
static unsigned int command_parse_argument(const std::string& s) {
	return std::atoi(s.c_str());
}
template<>
static float command_parse_argument(const std::string& s) {
	return std::strtof(s.c_str(), nullptr);
}
template<>
static std::string command_parse_argument(const std::string& s) { return s; }
template<>
static const std::string& command_parse_argument(const std::string& s) { return s; }

// Offers ability to bind a function void(*)(...), and invoke it using a string containing the name and arguments separated by a space
class CommandHandler
{
private:
	struct ICommand
	{
		virtual bool invoke(const std::vector<std::string>&) = 0;
	};

	template<typename... Args>
	class Command : public ICommand
	{
	public:
		using Callback = void(*)(Args...);
		Callback callback = nullptr;

		Command(Callback c)
			: callback(c) {}

		bool invoke(const std::vector<std::string>& arguments) override
		{
			constexpr size_t expectedArgs = sizeof...(Args);
			if (arguments.size() != expectedArgs) {
				printf("command expects %zu arguments, got %zu\n", expectedArgs, arguments.size());
				return false;
			}

			invoke_internal(arguments, std::index_sequence_for<Args...>{});
			return true;
		}
	private:
		template<size_t... Is>
		void invoke_internal(const std::vector<std::string>& args, std::index_sequence<Is...>)
		{
			callback(command_parse_argument<Args>(args[Is])...);
		}
	};

	std::unordered_map<std::string, std::unique_ptr<ICommand>> command_hash_to_callback_map;
public:
	// Bind a callback (with or without arguments) to a string for the command name
	template<typename... Args>
	void listen_for(const std::string& name, void(*callback)(Args...))
	{
		command_hash_to_callback_map.emplace(name, std::make_unique<Command<Args...>>(callback));
	}

	bool command_exists(const std::string& name) const
	{
		return command_hash_to_callback_map.count(name);
	}

	// Parses a command string and tries to invoke the corresponding command
	// Returns: true if invoked successfully, false if an error occurred (invalid command, incorrect arguments, etc)
	bool parse_and_invoke_command(std::string raw) const
	{
		// Preprocess string
		string_trim_whitespace(raw);
		string_collapse_whitespace(raw);

		size_t first_space = raw.find_first_of(' ');
		bool has_arguments = first_space != std::string::npos;

		std::string name = raw.substr(0, first_space);
		if (!command_exists(name)) {
			printf("command '%s' doesn't exist\n", name.c_str());
			return false;
		}

		// Extract arguments
		std::vector<std::string> args;
		if (has_arguments)
		{
			std::string arguments_string = raw.substr(raw.find_first_not_of(' ', first_space));
			bool success = extract_command_arguments_from_string(arguments_string, args);

			if (!success)
				return false; // failed to parse args
		}

		const auto& prototype = command_hash_to_callback_map.at(name);
		return prototype->invoke(args);
	}
private:
	// Parses a string of arguments and populates a vector with each argument split at a space
	// Surround arguments with quotations to group into one, irrespective of any spaces they contain
	static bool extract_command_arguments_from_string(const std::string& raw, std::vector<std::string>& result)
	{
		// Might over-reserve if arguments contain a space
		result.reserve(std::count(raw.begin(), raw.end(), ' '));

		// A string wrapped in this character will be treated as a single argument, even if there are spaces within it
		const char GroupingCharacter = '"';

		for (size_t i = 0; i < raw.size(); i++)
		{
			char c = raw[i];
			if (c == GroupingCharacter)
			{
				size_t groupEnd = raw.find_first_of(GroupingCharacter, i + 1);
				if (groupEnd == std::string::npos) {
					printf("expected a '%c' to close argument grouping\n", GroupingCharacter);
					return false;
				}

				size_t argStart = i + 1;
				result.emplace_back(raw.substr(argStart, groupEnd - argStart));
				i = groupEnd + 1;
				continue;
			}

			if (c != ' ')
			{
				size_t nextSpace = raw.find_first_of(' ', i + 1);

				if (nextSpace == std::string::npos) {
					// Last argument
					result.emplace_back(raw.substr(i));
					break;
				}

				result.emplace_back(raw.substr(i, nextSpace - i));
				i = nextSpace;
			}
		}

		result.shrink_to_fit();
		return true;
	}
};