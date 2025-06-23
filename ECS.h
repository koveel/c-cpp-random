#pragma once

#include "Bitset.h"

using Entity = uint32_t;

struct ConstantHash {
	size_t operator()(uint64_t x) const { return x; }
};
 
#define ASSERT(x) if (!(x)) { __debugbreak(); }

class ECS
{
private:
	template<typename C>
	struct Storage
	{
		Storage(size_t initialSize)
			: active(initialSize)
		{
			storage.resize(initialSize);
		}

		size_t capacity() const { return storage.capacity(); }

		void resize(size_t capacity)
		{
			storage.resize(capacity);
			active.resize(capacity);
		}

		std::vector<C> storage;
		// TODO: better way of doing this?
		DynamicBitset active; // 0 = get_component invalid, 1 = get_component valid
	};
	using StorageBuffer = std::array<uint8_t, sizeof(Storage<uint32_t>)>; // Storage<C>s are placement new'd into these buffers
	static inline std::unordered_map<size_t, StorageBuffer, ConstantHash> s_StorageMap; // [hash, storage]

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

	Entity create_entity(Entity desired_id)
	{
		ASSERT(desired_id > 0);

		auto bit = entities_availability[desired_id - 1];
		if (bit != 0)
			return create_entity();

		entity_count++;
		bit = 1;

		return desired_id;
	}

	void destroy_entity(Entity& entity)
	{
		if (!entities_availability[entity - 1])
			return; // entity doesn't exist (assert?)

		entity_count--;
		entities_availability[entity - 1] = 0;

		entity = 0;
	}

	// Constructs a component C belonging to a given entity and returns a reference to it
	// asserts that the component does not already exist - crash if so
	template<typename C, typename... Args>
	C& add_component(Entity entity, Args&&... args)
	{
		size_t hash = typeid(C).hash_code();

		Storage<C>* pStorage = get_or_create_storage<C>(hash);
		if (pStorage->capacity() <= entity)
			pStorage->resize(entity);

		auto active = pStorage->active[entity - 1];
		ASSERT(!active);
		active = true;

		return (pStorage->storage[entity - 1] = C(std::forward<Args>(args)...));
	}

	// Returns a pointer to the component belonging to entity, or nullptr if it does not exist
	template<typename C>
	C* get_component(Entity entity)
	{
		size_t hash = typeid(C).hash_code();

		Storage<C>* pStorage = get_storage<C>(hash);
		if (!pStorage || !pStorage->active[entity - 1])
			return nullptr;

		return &pStorage->storage[entity - 1];
	}

	// Destroys a component C belonging to an entity
	template<typename C>
	void remove_component(Entity entity)
	{
		size_t hash = typeid(C).hash_code();

		Storage<C>* pStorage = get_storage<C>(hash);
		if (!pStorage)
			return;

		auto active = pStorage->active[entity - 1];
		ASSERT(active);
		active = 0;

		//(*it).~C();
	}

	// For each existing component C, call F(Entity, C&)
	template<typename C, typename F>
	void for_each(F func)
	{
		size_t hash = typeid(C).hash_code();
		Storage<C>* pStorage = get_storage<C>(hash);

		for (Entity entity = 0; entity < pStorage->size(); entity++)
			func(entity, (*pStorage)[entity]);
	}

	template<typename F>
	void for_each_on(Entity entity, F func)
	{
		for (auto& pair : s_StorageMap)
		{
			//return reinterpret_cast<Storage<C>*>(&erased);
		}

		//size_t hash = typeid(C).hash_code();
		//Storage<C>* pStorage = get_storage<C>(hash);
		//
		//for (Entity entity = 0; entity < pStorage->size(); entity++)
		//	func(entity, (*pStorage)[entity]);
	}

	uint32_t get_entity_count() const { return entity_count; }
private:
	template<typename C>
	Storage<C>* get_or_create_storage(size_t hash)
	{
		if (!s_StorageMap.count(hash))
		{
			const size_t InitialStorageCapacity = 8;

			auto it = s_StorageMap.emplace(std::make_pair(hash, StorageBuffer{})).first;
			StorageBuffer& buffer = (*it).second;

			Storage<C>* pStorage = new(buffer.data()) Storage<C>(InitialStorageCapacity);
			return pStorage;
		}

		StorageBuffer& erased = s_StorageMap[hash];
		return reinterpret_cast<Storage<C>*>(&erased);
	}

	template<typename C>
	Storage<C>* get_storage(size_t hash)
	{
		if (!s_StorageMap.count(hash))
			return nullptr;

		StorageBuffer& erased = s_StorageMap[hash];
		return reinterpret_cast<Storage<C>*>(&erased);
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