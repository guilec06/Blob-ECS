/*
** Component
*/

#ifndef COMPONENT_HPP_
    #define COMPONENT_HPP_

#include "Includes.hpp"
#include "Errors.hpp"
#include <vector>
#include <limits>
#include <cstdint>
#include <iostream>

namespace ECS {

    template<typename T>
    struct DenseComponent {
        T component;
        EntityID entity;
    };

    template<ComponentType T>
    struct SparseSetData {
        std::vector<DenseComponent<T>> dense_components;           // Packed components
        std::vector<uint32_t> sparse;                           // EntityID → dense index mapping
    };
    constexpr uint32_t NULL_INDEX = std::numeric_limits<uint32_t>::max();

    class IComponentPool {
        public:
            virtual ~IComponentPool() = default;
            virtual void disableEntity(EntityID) = 0;
    };

    template <ComponentType T>
    class ComponentPool : public IComponentPool {
        public:
            ComponentPool() : m_cache_dirty(true) {
                m_data.dense_components.reserve(10000);
                m_data.sparse.reserve(100000);
            }

            ~ComponentPool() {
            }

            bool hasComponent(EntityID e) {
                return e < m_data.sparse.size() && m_data.sparse[e] != NULL_INDEX;
            }

            T &addComponent(EntityID e) {
                if (hasComponent(e))
                    throw ERROR::ComponentAlreadyAttached(e, std::string(typeid(T).name()));

                if (e >= m_data.sparse.size())
                    sparseGrow(e);

                uint32_t new_dense_index = static_cast<uint32_t>(m_data.dense_components.size());
                m_data.dense_components.emplace_back();

                m_data.dense_components[new_dense_index].entity = e;

                m_data.sparse[e] = new_dense_index;

                m_cache_dirty = true;
                return m_data.dense_components.back().component;
            }

            void removeComponent(EntityID e) {
                if (!hasComponent(e)) return;

                uint32_t dense_index = m_data.sparse[e];
                uint32_t last_index = static_cast<uint32_t>(m_data.dense_components.size() - 1);

                if (dense_index != last_index) {
                    m_data.dense_components[dense_index] = std::move(m_data.dense_components[last_index]);
                    
                    EntityID moved_entity = m_data.dense_components[dense_index].entity;
                    m_data.sparse[moved_entity] = dense_index;
                }

                m_data.dense_components.pop_back();

                m_data.sparse[e] = NULL_INDEX;

                m_cache_dirty = true;
            }

            T &getComponent(EntityID e) {
                
                return m_data.dense_components[m_data.sparse[e]].component;
            }

            const std::vector<EntityID> &getActiveEntities() {
                if (m_cache_dirty) {
                    m_cached_entities = m_data.dense_entities;
                    std::sort(m_cached_entities.begin(), m_cached_entities.end());
                    m_cache_dirty = false;
                }
                return m_cached_entities;
            }

            void disableEntity(EntityID e) override {
                removeComponent(e);
            }

            const std::vector<EntityID>& getPool() {
                return getActiveEntities();
            }

        private:
            SparseSetData<T> m_data;
            std::vector<EntityID> m_cached_entities;
            bool m_cache_dirty;

            void sparseGrow(EntityID new_standard) {
                std::size_t curr_size = m_data.sparse.size();
                if (curr_size == 0)
                    curr_size = 8192;
                else
                    while (curr_size <= new_standard) curr_size <<= 1;
                m_data.sparse.resize(curr_size, NULL_INDEX);
            }
    };
}

#endif /* !COMPONENT_HPP_ */
