/*
** EPITECH PROJECT, 2025
** ECS
** File description:
** Registry
*/

#include <atomic>
#include <cstdint>
#include <vector>

#include "Component.hpp"

#ifndef REGISTRY_HPP_
    #define REGISTRY_HPP_

namespace ECS {
    class ComponentTypeId {
        private:
            inline static std::atomic<uint16_t> counter = 0;
        public:
            template<typename T>
            static uint16_t get() {
                static uint16_t id = counter++;
                return id;
            }
    };

    class Registry {
        public:
            Registry() {
                m_pool_list.reserve(UINT16_MAX);
            };
            ~Registry() {
                for (const auto &it : m_pool_list)
                    delete it;
            };

            /**
             * @brief Checks if a component type exists in the registry.
             *
             * This function verifies whether a component pool for the given type ID is present
             * and initialized in the registry's pool list.
             *
             * @param type_id The unique identifier for the component type to check.
             * @return true if the component type exists and has an associated pool, false otherwise.
             */
            bool componentExists(uint16_t type_id) {
                return type_id < m_pool_list.size() && m_pool_list[type_id] != nullptr;
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
            template <ComponentType T>
            bool componentExists() {
                uint16_t type_id = ComponentTypeId::get<T>();
                return componentExists(type_id);
            }

            template <ComponentType T>
            bool registerComponent() {
                if (componentExists<T>())
                    return false;

                uint16_t type_id = ComponentTypeId::get<T>();
                m_pool_list.push_back(new ComponentPool<T>());
                return true;
            }

            template <ComponentType T>
            ComponentPool<T> &getPool() {
                if (!componentExists<T>())
                    throw ERROR::UnregisteredComponent(typeid(T).name());
                return *(static_cast<ComponentPool<T>*>(m_pool_list[ComponentTypeId::get<T>()]));
            }

            void disableEntity(EntityID e) {
                for (const auto &it : m_pool_list) {
                    if (it != nullptr)
                        it->disableEntity(e);
                }
            }

        protected:
        private:
            std::vector<IComponentPool*> m_pool_list;
    };
}

#endif /* !REGISTRY_HPP_ */
