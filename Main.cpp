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
#include "Command.h"

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

static ECS ecs;

static void command_create()
{
	printf("created entity %d\n", ecs.create_entity());
}

static void command_destroy(uint32_t e)
{
	printf("attempting to destroy entity %d\n", e);
	ecs.destroy_entity(e);
}

static void command_echo(const std::string& message)
{
	printf("%s\n", message.c_str());
}

struct vec2
{
	float x = 0.0f, y = 0.0f;
};

template<>
static vec2 command_parse_argument(const std::string& s)
{
	// (1.0f, 2.0f)
	std::string copy = s;

	vec2 result;
	result.x = std::strtof(copy.c_str(), nullptr);
	copy.erase(0, s.find_first_of(' ') + 1);
	result.y = std::strtof(copy.c_str(), nullptr);

	return result;
}

static void command_print(vec2 v)
{
	printf("[%f, %f]\n", v.x, v.y);
}

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

static void print_bitset(const DynamicBitset& set)
{
	for (uint32_t b = 0; b < set.count() / 8; b++) {
		for (uint32_t i = 0; i < 8; i++)
			printf("%d", set[(b * 8) + i]);
		printf(" ");
	}
	printf("\n");
}

int main()
{
	CommandHandler cmd;
	cmd.listen_for("create", command_create);
	cmd.listen_for("destroy", command_destroy);
	cmd.listen_for("echo", command_echo);
	cmd.listen_for("print", command_print);

	char buffer[1024];
	while (std::cin.getline(buffer, 1024))
	{
		cmd.parse_and_invoke_command(buffer);
	}
}