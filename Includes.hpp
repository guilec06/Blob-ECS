/*
 *  Includes
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

#ifndef INCLUDES_HPP_
#define INCLUDES_HPP_

#include <bitset>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>

namespace ECS
{
class ISystem;
class ECS;

// Concept for accepting system classes
template <typename T>
concept SystemClass = std::is_base_of_v<ISystem, T>;

// Concept for accepting components
template <typename T>
concept ComponentType = std::is_default_constructible_v<T>;

template<typename T>
concept TagType = std::is_empty_v<T>;

// Alias for uint32_t, used to represent, locate and perform actions on entities
using EntityID = uint32_t;

constexpr EntityID NULL_ENTITY = std::numeric_limits<EntityID>::max();

// Alias for uint16_t, used to represent a System within the ECS
using SystemID = uint16_t;

using SysPriority = uint8_t;

struct SystemEntry {
    ISystem *sys;
    SysPriority priority;
};

constexpr std::size_t COMPONENT_POOL_SPARSE_DEFAULT = 20;
constexpr std::size_t COMPONENT_POOL_DENSE_DEFAULT = 10;
constexpr SysPriority SYSTEM_PRIORITY_DEFAULT = 10;

constexpr std::size_t COMPONENT_MASK_WIDTH = 128;
using EntityComponentMask = std::bitset<COMPONENT_MASK_WIDTH>;

struct EntityMetadata {
    std::vector<uint16_t> generation;
    std::vector<EntityComponentMask> component_mask;
    std::vector<bool> alive;
};

} // namespace ECS


#endif /* !INCLUDES_HPP_ */
