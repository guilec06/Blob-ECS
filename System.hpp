/*
** EPITECH PROJECT, 2025
** ECS
** File description:
** System
*/

#ifndef SYSTEM_HPP_
    #define SYSTEM_HPP_

#include "ECS.hpp"

namespace ECS {

    /**
     * @brief System Interface, every system should inherit from this class
     */
    class ISystem {
        public:
            virtual ~ISystem() = default;

            virtual void Update(ECS &, SystemID thisID, uint32_t msecs) = 0;
        protected:
        private:
    };
}
#endif /* !SYSTEM_HPP_ */
