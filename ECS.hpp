/*
 *  ECS
 *
 *  Blob ECS is a lightweight Entity Component System library
 *  Copyright (C) 2025 LECOCQ Guillaume
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef ECS_HPP_
#define ECS_HPP_

#include "Component.hpp"
#include "Includes.hpp"
#include "Registry.hpp"
#include "System.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <bitset>

namespace ECS
{
class ECS
{
  public:
    /**
     * @brief Construct a new ECS object
     *
     * @param max_entities OPTIONAL specify a maximum entity count, 0 = infinite
     * @param use_as_power OPTIONAL specify if the max_entities is to be interpreted as a power of 2
     * or as a litteral limit (false by default)
     */
    ECS([[maybe_unused]] std::size_t max_entities = 0, [[maybe_unused]] bool use_as_power = false)
    {
    }

    ~ECS()
    {
    }

    /**
     * @brief Returns the amount of currently active entities
     *
     * @return std::size_t Currently active entities
     */
    std::size_t currentEntityCount()
    {
        return m_active_entities;
    }

    /**
     * @brief Checks if the specified entity's ID match an active entity
     *
     * @param e EntityID
     * @return true if it exists
     * @return false if it doesn't
     */
    bool entityIsActive(EntityID e)
    {
        return e < m_id_counter && m_entity_meta.alive[e];
    }

    /**
     * @brief Create a new entity and returns its ID
     *
     * @return EntityID
     */
    EntityID entityCreate()
    {
        EntityID newId;

        if (m_available_ids.size() == 0)
        {
            newId = m_id_counter++;
            m_entity_meta.generation.push_back(0);
            m_entity_meta.component_mask.push_back(EntityComponentMask());
            m_entity_meta.alive.push_back(true);
        }
        else
        {
            newId = m_available_ids.back();
            m_available_ids.pop_back();

            m_entity_meta.generation[newId]++;
            m_entity_meta.component_mask[newId].reset();
            m_entity_meta.alive[newId] = true;
        }
        m_active_entities++;
        return newId;
    }

    /**
     * @brief Delete an entity corresponding to an ID
     *
     * @param e EntityID
     */
    void entityDelete(EntityID e)
    {
        if (!entityIsActive(e))
            return;
        m_available_ids.push_back(e);
        m_active_entities--;
        registry.disableEntity(e, entityGetMetaComponentMask(e));
        m_entity_meta.alive[e] = false;
    }

    std::uint16_t entityGetMetaGeneration(EntityID e)
    {
        return m_entity_meta.generation[e];
    }

    EntityComponentMask &entityGetMetaComponentMask(EntityID e)
    {
        return m_entity_meta.component_mask[e];
    }

    /**
     * @brief Returns a vector containing the IDs of entities that have ALL specified components
     *
     * @tparam Components The component types to check for (variadic template)
     * @return std::vector<EntityID> The list of entities that have all specified components
     */
    template <typename... Components> void getEntitiesByComponentsAllOf(std::vector<EntityID> &buff)
    {
        buff.clear();
        if constexpr (sizeof...(Components) == 0)
            return;
        
#ifdef BLOB_DEBUG
        if (!(componentExists<Components>() && ...))
            return;
#endif

        EntityComponentMask query;
        (query.set(registry.bitOf<Components>()), ...);

        IComponentPool *smallest = nullptr;
        auto consider = [&](IComponentPool *pool) {
            if (!smallest || pool->size() < smallest->size())
                smallest = pool;
        };

        (consider(&registry.getPool<Components>()), ...);

        if constexpr (sizeof...(Components) == 1) {
            buff = smallest->getActiveEntities();
            return;
        }

        for (EntityID e : smallest->getActiveEntities()) {
            if ((m_entity_meta.component_mask[e] & query) == query)
                buff.push_back(e);
        }
    }

    /**
     * @brief Returns a vector containing the IDs of entities that have AT LEAST ONE of the
     * specified components
     *
     * @tparam Components The component types to check for (variadic template)
     * @return std::vector<EntityID> The list of entities that have at least one of the specified
     * components
     */
    template <typename... Components> void getEntitiesByComponentsAnyOf(std::vector<EntityID> &buff)
    {
        buff.clear();

        if constexpr (sizeof...(Components) == 0)
            return;
        
        if (m_any_seen.size() < m_id_counter)
            m_any_seen.resize(m_id_counter, false);

        auto addEntities = [&](IComponentPool *pool) {
            for (EntityID e : pool->getActiveEntities()) {
                if (!m_any_seen[e]) {
                    m_any_seen[e] = true;
                    buff.push_back(e);
                }
            }
        };
        ((componentExists<Components>() ? addEntities(&registry.getPool<Components>()) : void()), ...);

        for (EntityID e : buff)
            m_any_seen[e] = false;
    }

    void entityGetAllComponents(std::unordered_map<uint16_t, void *> &buff, EntityID e)
    {
        IComponentPool **list = registry.getRawPools();
        const std::vector<uint16_t> ids = registry.getRegisteredIds();

        for (auto id : ids) {
            IComponentPool *pool = list[id];
            if (pool->poolIsTag())
                continue;
            void *component = pool->getVoid(e);
            if (component)
                buff[id] = component;
        }
    }

    /**
     * @brief Registers a new component in the environment, the component can then be used withing
     * the environment
     *
     * @tparam T The type of the component to register
     */
    template <typename T> void registerComponent()
    {
        registry.registerComponent<T>();
    }

    /**
     * @brief Checks if the component type is registered
     *
     * @tparam T The Component type to check for
     * @return true If it is registered.
     * @return false if it is not.
     */
    template <typename T> bool componentExists()
    {
        return registry.componentExists<T>();
    }

    /**
     * @brief Checks if an enity has a component attached to it
     *
     * @tparam T The component type to check for
     * @param e EntityID - The ID of the entity
     * @return true If the entity exists AND has the component attached to IT
     * @return false Either if the entity doesn't exists or if the component isn't attached to it
     */
    template <typename T> bool entityHasComponent(EntityID e)
    {
        return registry.getPool<T>().hasComponent(e);
    }

    /**
     * @brief Add a new component to the specified entity
     *
     * @tparam T The component type to add to the entity
     * @param e EntityID - The entity to add the component to
     * @return T& Reference to the newly created Component
     * @throw ERROR::UnregisteredComponent => if the component isn't registered
     * @throw ERROR::ComponentAlreadyAttached => if the component is ALREADY attached to the entity
     */
    template <typename T> T &entityAddComponent(EntityID e)
    {
        m_entity_meta.component_mask[e].set(registry.bitOf<T>(), true);
        return registry.getPool<T>().addComponent(e);
    }

    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...>
    T &entityAddComponent(EntityID e, Args&&... args)
    {
        m_entity_meta.component_mask[e].set(registry.bitOf<T>(), true);
        return registry.getPool<T>().addComponent(e, std::forward<Args>(args)...);
    }

    template<typename T>
        requires std::is_empty_v<T>
    void entityAddComponent(EntityID e)
    {
        m_entity_meta.component_mask[e].set(registry.bitOf<T>(), true);
        registry.getPool<T>().addComponent(e);
    }

    /**
     * @brief Gets the attached specified component to the specified entity
     *
     * @tparam T The component type to get
     * @param e EntityID - The entity's ID
     * @return T& Reference to the associated component
     * @throw ERROR::UnregisteredComponent => if the component isn't registered
     * @throw ERROR::ComponentNotAttached => if the component is NOT attached to the entity
     */
    template <typename T> requires (!std::is_empty_v<T>) T &entityGetComponent(EntityID e)
    {
        return registry.getPool<T>().getComponent(e);
    }

    template <typename T> requires (!std::is_empty_v<T>) const T &entityGetComponent(EntityID e) const
    {
        return static_cast<const T&>(registry.getPool<T>().getComponent(e));
    }

    /**
     * @brief Removes the attached component from the entity
     *
     * @tparam T The component type
     * @param e The entity ID
     */
    template <typename T> void entityRemoveComponent(EntityID e)
    {
        m_entity_meta.component_mask[e].reset(registry.bitOf<T>());
        registry.getPool<T>().removeComponent(e);
    }

    template <typename T> requires std::is_empty_v<T> bool entityHasTag(EntityID e)
    {
        return entityHasComponent<T>(e);
    }

    template <typename T> requires std::is_empty_v<T> void entityAddTag(EntityID e)
    {
        entityAddComponent<T>(e);
    }

    template <typename T> requires std::is_empty_v<T> void entityRemoveTag(EntityID e)
    {
        entityRemoveComponent<T>(e);
    }

    template<typename... Types> requires (std::is_empty_v<Types> && ...) void getEntitiesByTagsAllOf(std::vector<EntityID> &buff)
    {
        getEntitiesByComponentsAllOf<Types...>(buff);
    }

    template<typename... Types> requires (std::is_empty_v<Types> && ...) void getEntitiesByTagsAnyOf(std::vector<EntityID> &buff)
    {
        getEntitiesByComponentsAnyOf<Types...>(buff);
    }

    /**
     * @brief Get the Pool object
     *
     * @tparam T The component type of the Pool
     * @return ComponentPool<T>* The pointer to the pool
     * @throw ERROR::UnregisteredComponent => if the component isn't registered
     */
    template <typename T> ComponentPool<T> &getPool()
    {
        return registry.getPool<T>();
    }

    /**
        * @brief Adds a new system to the ECS
        * 
        * @tparam T The system class
        * @param tickrate The ticks (calls to Update()) the system should skip after ticked
        * @return SystemID The new ID for the system
        */
    template<SystemClass T, typename... Args>
        requires std::constructible_from<T, Args...>
    T &addSystem(Args&&... args)
    {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T *ptr = system.get();

        m_systems.insert_or_assign(std::type_index(typeid(T)), std::move(system));
        m_active_systems.push_back({ptr, SYSTEM_PRIORITY_DEFAULT});
        reorderSystems();
        return *ptr;
    }

    /**
     * @brief Toggles the system on or off, defining if it should tick when Update() is called
     *
     * @param id The id of the system to toggle
     */
    template<SystemClass T>
    void toggleSystem()
    {
        ISystem *ptr = m_systems[std::type_index(typeid(T))].get();

        const auto &it = std::find_if(m_active_systems.begin(), m_active_systems.end(), [ptr](SystemEntry &elem) {return elem.sys == ptr; });

        if (it == m_active_systems.end())
            m_active_systems.push_back({ptr, SYSTEM_PRIORITY_DEFAULT});
        else
            m_active_systems.erase(it);
        reorderSystems();
    }

    /**
     * @brief Checks if the specified system is enabled
     *
     * @param sys The system's ID
     * @return true Is the system is enabled,
     * @return false if it is not
     */
    template<SystemClass T>
    bool systemIsEnabled()
    {
        ISystem *ptr = m_systems[std::type_index(typeid(T))].get();

        return std::find_if(m_active_systems.begin(), m_active_systems.end(), [ptr](SystemEntry &elem) {return elem.sys == ptr; }) != m_active_systems.end();
    }

    template<SystemClass T>
    void systemSetPriority(SysPriority priority)
    {
        ISystem *ptr = m_systems[std::type_index(typeid(T))].get();

        const auto &it = std::find_if(m_active_systems.begin(), m_active_systems.end(), [ptr](SystemEntry &elem) {return elem.sys == ptr; });
        if (it != m_active_systems.end())
            it->priority = priority;
        reorderSystems();
    }

    template<SystemClass T>
    T &getSystem()
    {
        ISystem *ptr = m_systems[std::type_index(typeid(T))].get();

        return *static_cast<T*>(ptr);
    }

    /**
     * @brief Updates every active systems
     *
     */
    void Update(uint32_t msecs = 0)
    {
        for (auto system : m_active_systems) {
            system.sys->Update(*this, msecs);
        }
    }

    Registry registry;

  private:

    void reorderSystems()
    {
        std::sort(
            m_active_systems.begin(),
            m_active_systems.end(),
            [](SystemEntry &a, SystemEntry &b) { return a.priority < b.priority; }
        );
    }

    EntityID m_id_counter = 0;
    EntityMetadata m_entity_meta;

    std::vector<EntityID> m_available_ids;
    std::size_t m_active_entities = 0; // in theory: m_active_entities = m_id_counter - m_available_ids.size()
    std::unordered_map<std::type_index, std::unique_ptr<ISystem>> m_systems;
    std::vector<SystemEntry> m_active_systems;

    std::vector<bool> m_any_seen;
};
} // namespace ECS

#endif /* !ECS_HPP_ */
