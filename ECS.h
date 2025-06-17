#pragma once

#include "Bitset.h"

using Entity = uint32_t;

//return (entity.components & (1 << comp)) != 0;
//entity.components |= (1 << comp);

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

	DynamicBitset entities_availability; // 0 = inactive, 1 = active
	Entity entity_count = 0;
public:
	ECS() = default;

	Entity create_entity()
	{
		entity_count++;
		Entity id = get_first_available_entity_id();
		if (!id) id = entity_count;

		entities_availability[id - 1] = 1;
		return id;
	}

	void destroy_entity(Entity& entity)
	{
		if (!entities_availability[entity - 1])
			return; // entity doesn't exist (assert?)

		entity_count--;
		entities_availability[entity - 1] = 0;

		entity = 0;
	}

	static inline uint32_t s_ComponentMaskIt = 0;

	// Constructs a component C belonging to a given entity and returns a reference to it
	// asserts that the component does not already exist - crash if so
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

	// Returns a pointer to the component belonging to entity, or nullptr if it does not exist
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

	// Destroys a component C belonging to an entity
	template<typename C>
	void remove_component(Entity entity)
	{
		size_t hash = typeid(C).hash_code();

		Pool<C>* pPool = get_pool<C>(hash);
		if (!pPool) return;

		auto it = pPool->begin() + (entity - 1);
		if (it == pPool->end()) return;

		(*it).~C();
	}

	// For each existing component C, call F(Entity, C&)
	template<typename C, typename F>
	void for_each(F func)
	{
		size_t hash = typeid(C).hash_code();
		Pool<C>* pPool = get_pool<C>(hash);

		for (Entity entity = 0; entity < pPool->size(); entity++)
			func(entity, (*pPool)[entity]);
	}

	template<typename F>
	void for_each_on(Entity entity, F func)
	{
		for (auto& pair : s_PoolMap)
		{
			//return reinterpret_cast<Pool<C>*>(&erased);
		}

		//size_t hash = typeid(C).hash_code();
		//Pool<C>* pPool = get_pool<C>(hash);
		//
		//for (Entity entity = 0; entity < pPool->size(); entity++)
		//	func(entity, (*pPool)[entity]);
	}
private:
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

	uint32_t get_first_available_entity_id()
	{
		for (uint32_t i = 0; i < entities_availability.count(); i++)
		{
			if (entities_availability[i] == 0)
				return i + 1;
		}

		return 0;
	}
};