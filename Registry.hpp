/*
** EPITECH PROJECT, 2025
** ECS
** File description:
** Registry
*/

#include <atomic>
#include <cstdint>

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
            Registry() {};

        protected:
        private:
    };
}

#endif /* !REGISTRY_HPP_ */
