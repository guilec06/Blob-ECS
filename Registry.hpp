/*
 *  Registry
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

#include "Component.hpp"

#include <cstdint>

#ifndef REGISTRY_HPP_
#define REGISTRY_HPP_

namespace ECS
{

class Registry
{
  public:
    Registry()
    {
    }

    ~Registry()
    {
        for (const auto &it : m_pool_list)
        {
            if (it != nullptr)
                delete it;
        }
    }

    /**
     * @brief Checks if a component type exists in the registry.
     *
     * This function verifies whether a component pool for the given type ID is present
     * and initialized in the registry's pool list.
     *
     * @param type_id The unique identifier for the component type to check.
     * @return true if the component type exists and has an associated pool, false otherwise.
     */
    bool componentExists(uint16_t type_id)
    {
        return m_pool_list[type_id] != nullptr;
    }

    /**
     * @brief Checks if a component of the specified type exists in the registry.
     *
     * This template function retrieves the type ID for the given component type T
     * and delegates to the non-template overload of componentExists(uint16_t) to
     * perform the check.
     *
     * @tparam T The component type to check for existence. Must satisfy ComponentType.
     * @return true if a component of type T exists, false otherwise.
     */
    template <typename T> bool componentExists()
    {
        uint16_t type_id = ComponentTypeId::get<T>();
        return componentExists(type_id);
    }

    /**
     * @brief Registers a new component type to the registry
     *
     * @tparam T The type to register
     * @return true If the component has been registered successfully
     * @return false If the component was already registered
     */
    template <typename T> bool registerComponent()
    {
        if (componentExists<T>())
            return false;

        uint16_t type_id = ComponentTypeId::get<T>();
        m_pool_list[type_id] = static_cast<IComponentPool *>(new ComponentPool<T>());
        m_registered_ids.push_back(type_id);

        if (m_next_bit >= COMPONENT_MASK_WIDTH)
            throw ERROR::TooManyComponentTypes(COMPONENT_MASK_WIDTH);
        m_bit_of_type[type_id] = m_next_bit;
        m_type_of_bit[m_next_bit] = type_id;
        m_next_bit++;

        return true;
    }

    /**
     * @brief Get the signature-mask bit index assigned to a registered component type.
     *
     * @tparam T The component type
     * @return uint16_t The bit index within EntityComponentMask for this type
     */
    template <typename T> uint16_t bitOf()
    {
        return m_bit_of_type[ComponentTypeId::get<T>()];
    }

    /**
     * @brief Get the Pool object
     *
     * @tparam T The component type the pool stores
     * @return ComponentPool<T>&
     */
    template <typename T> ComponentPool<T> &getPool()
    {
        #ifdef BLOB_DEBUG
            if (!componentExists<T>())
                throw ERROR::UnregisteredComponent(typeid(T).name());
        #endif
        return *(static_cast<ComponentPool<T> *>(m_pool_list[ComponentTypeId::get<T>()]));
    }

    /**
     * @brief Disables an entity from being computed
     *
     * @param e Entity ID
     */
    void disableEntity(EntityID e, EntityComponentMask &mask)
    {
        for (std::size_t i = 0; i < mask.size(); i++) {
            if (mask.test(i))
                m_pool_list[m_type_of_bit[i]]->removeComponent(e);
        }
    }

    IComponentPool **getRawPools()
    {
        return m_pool_list;
    }

    const std::vector<uint16_t> &getRegisteredIds() const
    {
        return m_registered_ids;
    }

    std::size_t size() const
    {
        return m_registered_ids.size();
    }

  protected:
  private:
    IComponentPool *m_pool_list[UINT16_MAX]{nullptr};
    std::vector<uint16_t> m_registered_ids;

    uint16_t m_next_bit = 0;
    uint16_t m_bit_of_type[UINT16_MAX]{};
    uint16_t m_type_of_bit[COMPONENT_MASK_WIDTH]{};
};
} // namespace ECS

#endif /* !REGISTRY_HPP_ */
