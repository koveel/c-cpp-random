#include <iostream>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <array>

static void print_binary(uint32_t v, uint32_t num_bytes)
{
	for (uint32_t i = (num_bytes * 8); i > 0; i--)
	{
		uint32_t mask = 1 << (i - 1);
		std::cout << ((v & mask) != 0);
	}
}

using Entity = uint32_t;

//static uint32_t has_component(const Entity& entity, uint32_t comp) { 
//	return (entity.components & (1 << comp)) != 0;
//}
//entity.components |= (1 << comp);

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

struct ConstantHash {
	size_t operator()(uint64_t x) const { return x; }
};

#define ASSERT(x) if (!(x)) { __debugbreak(); }

class ECS
{
private:
	template<typename C>
	using Pool = std::vector<C>;
	using PoolBuffer = std::array<uint8_t, sizeof(Pool<uint32_t>)>; // Pool<C>s are placement new'd into these buffers

	static inline std::unordered_map<size_t, PoolBuffer, ConstantHash> s_PoolMap; // [hash, pool]

	template<typename C>
	Pool<C>* get_or_create_pool(size_t hash)
	{
		if (!s_PoolMap.count(hash))
		{
			const size_t InitialPoolCapacity = 2;

			auto it = s_PoolMap.emplace(std::make_pair(hash, PoolBuffer{})).first;
			PoolBuffer& buffer = (*it).second;

			Pool<C>* pPool = new(buffer.data()) Pool<C>();
			pPool->reserve(InitialPoolCapacity);
			return pPool;
		}

		PoolBuffer& erased = s_PoolMap[hash];
		return reinterpret_cast<Pool<C>*>(&erased);
	}

	template<typename C>
	Pool<C>* get_pool(size_t hash)
	{
		if (!s_PoolMap.count(hash))
			return nullptr;

		PoolBuffer& erased = s_PoolMap[hash];
		return reinterpret_cast<Pool<C>*>(&erased);
	}
public:
	ECS() = default;

	Entity create_entity() const
	{
		static uint32_t id = 0;
		return ++id;
	}

	template<typename C, typename... Args>
	C& add_component(Entity entity, Args&&... args)
	{
		size_t hash = typeid(C).hash_code();
		Pool<C>* pPool = get_or_create_pool<C>(hash);
		
		if (pPool->capacity() <= entity)
			pPool->reserve(entity);

		auto it = pPool->begin() + (entity - 1);
		ASSERT(it == pPool->end());

		return *pPool->emplace(it, std::forward<Args>(args)...);
	}

	template<typename C>
	C* get_component(Entity entity)
	{
		size_t hash = typeid(C).hash_code();

		Pool<C>* pPool = get_pool<C>(hash);
		if (!pPool)
			return nullptr;

		auto it = pPool->begin() + (entity - 1);
		return it == pPool->end() ? nullptr : &*it;
	}
};

int main()
{
	ECS ecs;
	Entity entity = ecs.create_entity();
	Entity entity2 = ecs.create_entity();

	{
		auto& transform = ecs.add_component<TransformComponent>(entity);
		transform.x = 5.0f;
		transform.h = 10252.0f;
	}

	auto& physics = ecs.add_component<PhysicsComponent>(entity);

	if (auto transform = ecs.get_component<TransformComponent>(entity2))
		printf("%f, %f, %f, %f\n", transform->x, transform->y, transform->w, transform->h);
}