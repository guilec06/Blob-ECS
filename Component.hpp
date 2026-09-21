/*
 *  Component
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

#ifndef COMPONENT_HPP_
#define COMPONENT_HPP_

#include "Errors.hpp"
#include "Includes.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>

namespace ECS
{

/**
 * @brief Class for generating and getting custom type IDs
 */
class ComponentTypeId
{
  private:
    inline static std::atomic<uint16_t> runtime_counter = 0;

  public:
    /**
     * @brief Returns the ID attached to this specific type, generates an ID if this is the first
     * call
     *
     * @tparam T The type to get an ID from
     * @return uint16_t The corresponding ID
     */
    template <typename T> static uint16_t get()
    {
        static const uint16_t id = runtime_counter.fetch_add(1, std::memory_order_relaxed);
        return id;
    }
};

/**
 * @brief A cell that stores an entity ID with its attached component
 *
 * @tparam T The component type
 */
template <typename T> struct DenseComponent
{
    std::vector<T> components;
    std::vector<EntityID> entities;
};

/**
 * @brief Structure for a custom sparse set
 *
 * @tparam T The type of the set
 */
template <typename T> struct SparseSetData
{
    DenseComponent<T> dense_components; // Packed components - SoA
    std::vector<uint32_t> sparse;                    // EntityID -> dense index mapping
};
constexpr uint32_t NULL_INDEX = std::numeric_limits<uint32_t>::max();

/**
 * @brief ComponentPool interface, this should be casted to a <compType>ComponentPool to be used
 */
class IComponentPool
{
  public:
    virtual ~IComponentPool() = default;

    virtual bool hasComponent(EntityID id) const = 0;
    virtual void removeComponent(EntityID) = 0;
    virtual const std::vector<EntityID> &getActiveEntities() = 0;
    virtual std::size_t size() const = 0;
    virtual void *getVoid(EntityID e) = 0;
    virtual bool poolIsTag() const = 0;
};

/**
 * @brief ComponentPool that stores components of the defined type
 *
 * @tparam T The type of component the pool is storing
 */
template <typename T, bool isTag = std::is_empty_v<T>> class ComponentPool : public IComponentPool
{
  public:
    ComponentPool() : m_cache_dirty(true), m_size(0)
    {
        m_data.dense_components.components.reserve(COMPONENT_POOL_DENSE_DEFAULT);
        m_data.dense_components.entities.reserve(COMPONENT_POOL_DENSE_DEFAULT);
        m_data.sparse.reserve(COMPONENT_POOL_SPARSE_DEFAULT);
    }

    ~ComponentPool()
    {
    }

    /**
     * @brief Checks if a specific entity has the component
     *
     * @param e The entity ID
     * @return true The entity has the component
     * @return false The entity does not have the component
     */
    bool hasComponent(EntityID e) const override
    {
        return e < m_data.sparse.size() && m_data.sparse[e] != NULL_INDEX;
    }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    T &addComponent(EntityID e, Args&&... args)
    {
        if (hasComponent(e))
            throw ERROR::ComponentAlreadyAttached(e, std::string(typeid(T).name()));

        if (e >= m_data.sparse.size())
            sparseGrow(e);
        
        uint32_t new_dense_index = static_cast<uint32_t>(m_data.dense_components.components.size());
        m_data.dense_components.components.push_back({
            T(std::forward<Args>(args)...)
        });
        m_data.dense_components.entities.push_back(e);

        m_data.sparse[e] = new_dense_index;
        m_cache_dirty = true;
        m_size++;
        return m_data.dense_components.components.back();
    }

    /**
     * @brief Removes the component attached to the specified entity
     *
     * @param e The entity ID
     */
    void removeComponent(EntityID e) override
    {
        if (!hasComponent(e))
            return;

        uint32_t dense_index = m_data.sparse[e];
        uint32_t last_index = static_cast<uint32_t>(m_data.dense_components.components.size() - 1);

        if (dense_index != last_index)
        {
            m_data.dense_components.components[dense_index] = std::move(m_data.dense_components.components[last_index]);
            m_data.dense_components.entities[dense_index] = m_data.dense_components.entities[last_index];

            EntityID moved_entity = m_data.dense_components.entities[dense_index];
            m_data.sparse[moved_entity] = dense_index;
        }
        
        m_data.dense_components.components.pop_back();
        m_data.dense_components.entities.pop_back();
        m_size--;
        
        m_data.sparse[e] = NULL_INDEX;

        m_cache_dirty = true;
    }

    /**
     * @brief Get the Component object attached to the specified entity
     *
     * @param e The entity ID
     * @return T& Reference to the component
     */
    T &getComponent(EntityID e)
    {

        return m_data.dense_components.components[m_data.sparse[e]];
    }

    /**
     * @brief Get a vector containing every entity's id which have this component attached
     *
     * @return const std::vector<EntityID>& Vector of entities
     */
    const std::vector<EntityID> &getActiveEntities() override
    {
        if (m_cache_dirty) {
            m_cached_entities = m_data.dense_components.entities;
            m_cache_dirty = false;
        }
        return m_cached_entities;
    }


    DenseComponent<T> &getData()
    {
        return m_data.dense_components;
    }

    std::size_t size() const override
    {
        return m_size;
    }
    
    void *getVoid(EntityID e) override
    {
        if (e >= m_data.sparse.size())
            return nullptr;
        if (m_data.sparse[e] == NULL_INDEX)
            return nullptr;
        return &m_data.dense_components.components[m_data.sparse[e]];
    }

    bool poolIsTag() const override
    {
        return false;
    }

  private:
    SparseSetData<T> m_data;
    std::vector<EntityID> m_cached_entities;
    bool m_cache_dirty;
    std::size_t m_size;

    void sparseGrow(EntityID new_standard)
    {
        std::size_t curr_size = m_data.sparse.size();
        if (curr_size == 0)
            curr_size = 8192;
        while (curr_size <= new_standard)
            curr_size <<= 1;
        m_data.sparse.resize(curr_size, NULL_INDEX);
    }
};

template<typename T>
class ComponentPool<T, true> : public IComponentPool {
    public:
        ComponentPool()
        {
        }

        ~ComponentPool()
        {
        }

        bool hasComponent(EntityID e) const override
        {
            return e < m_data.size() && m_data[e];
        }

        void addComponent(EntityID e)
        {
            if (hasComponent(e))
                throw ERROR::ComponentAlreadyAttached(e, std::string(typeid(T).name()));

            if (e >= m_data.size())
                grow(e);
            m_data[e] = true;
            m_cache_dirty = true;
            m_size++;
        }

        void removeComponent(EntityID e) override
        {
            if (e < m_data.size()) {
                m_cache_dirty |= m_data[e];
                m_size -= m_data[e];
                m_data[e] = false;
            }
        }

        const std::vector<EntityID> &getActiveEntities() override
        {
            if (m_cache_dirty)
            {
                m_cached_entities.clear();
                m_cached_entities.reserve(m_data.size());
                for (std::size_t i = 0; i < m_data.size(); i++) {
                    if (m_data[i])
                        m_cached_entities.push_back(i);
                }
                m_cache_dirty = false;
            }
            return m_cached_entities;
        }

        std::size_t size() const override
        {
            return m_size;
        }

        void *getVoid(EntityID e) override
        {
            return nullptr;
        }

        bool poolIsTag() const override
        {
            return true;
        }

    private:
        std::vector<bool> m_data;
        std::vector<EntityID> m_cached_entities;
        bool m_cache_dirty = true;
        std::size_t m_size;

        void grow(std::size_t new_standard)
        {
            std::size_t curr_size = m_data.size();
            if (curr_size == 0)
                curr_size = 8192;
            while (curr_size <= new_standard)
                curr_size <<= 1;
            m_data.resize(curr_size, false);
        }

};

} // namespace ECS

#endif /* !COMPONENT_HPP_ */
