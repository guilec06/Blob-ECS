/*
** Errors
*/

#ifndef ERRORS_HPP_
    #define ERRORS_HPP_

#include <exception>
#include <string>

namespace ECS {
    namespace ERROR {
        class UnregisteredComponent : public std::exception {
            public:
                UnregisteredComponent(const std::string& compname)
                : message("Component '" + compname + "' isn't registered!")
                {}
                ~UnregisteredComponent() {}

                const char *what() const noexcept override
                {
                    return message.c_str();
                }
            private:
                std::string message;
        };

        class InvalidEntityID : public std::exception {
            public:
                InvalidEntityID(std::size_t id)
                : message("Entity id " + std::to_string(id) + " is invalid or doesn't exist!")
                {}
                ~InvalidEntityID() {}

                const char *what() const noexcept override
                {
                    return message.c_str();
                }
            private:
                std::string message;
        };

        class ComponentNotAttached : public std::exception {
            public:
                ComponentNotAttached(std::size_t entity, const std::string& comp_name)
                : message("Entity id " + std::to_string(entity) + " doesn't have component '" + comp_name + "' attached to it!")
                {}
                ~ComponentNotAttached() {}

                const char *what() const noexcept override
                {
                    return message.c_str();
                }
            private:
                std::string message;
        };

        class ComponentAlreadyAttached : public std::exception {
            public:
                ComponentAlreadyAttached(std::size_t entity, const std::string& comp_name)
                : message("Entity id " + std::to_string(entity) + " already have component '" + comp_name + "' attached to it!")
                {}
                ~ComponentAlreadyAttached() {}

                const char *what() const noexcept override
                {
                    return message.c_str();
                }
            private:
                std::string message;
        };
    }
}

#endif /* !ERRORS_HPP_ */
