/*
** Includes
*/

#ifndef INCLUDES_HPP_
    #define INCLUDES_HPP_

    #include <cstddef>
    #include <concepts>
    #include <type_traits>
    #include <typeindex>
    #include <cstdint>
    #include <atomic>

namespace ECS {
    class ISystem;
    class ECS;
        
    // Concept for accepting system classes
    template<typename T>
    concept SystemClass = std::is_base_of_v<ISystem, T> && std::is_constructible_v<T>;

    // Concept for accepting components
    template<typename T>
    concept ComponentType = std::is_default_constructible_v<T>;

    // Alias for uint32_t, used to represent, locate and perform actions on entities
    using EntityID = uint32_t;

    // Alias for uint16_t, used to represent a System within the ECS
    using SystemID = uint16_t;

    // Entity groups, modify this enum to add new groups to the system
    enum EntityGroup {
        NONE,
        EXAMPLES
    };

    /*
        This is how an entity is stored within the ECS
        isActive represents if an entity exists or not
        group represents a group which the entity belongs to
    */
    struct Entity {
        bool isActive = false;
        EntityGroup group = NONE;
    };

    struct SystemData {
        bool enabled;
        ISystem *sys;
        int tickrate;
        int skipped_ticks;
    };
}

#endif /* !INCLUDES_HPP_ */
