#pragma once
#include <string>
#include "esp_log.h"

namespace pooaway::error
{

    class Error
    {
    public:
        explicit Error(std::string message) : m_message(std::move(message)) {}
        [[nodiscard]] const std::string &what() const { return m_message; }

    private:
        std::string m_message;
    };

    class HandlerError : public Error
    {
    public:
        explicit HandlerError(const std::string &message) : Error(message) {}
    };

    class InitializationError : public Error
    {
    public:
        explicit InitializationError(const std::string &message) : Error(message) {}
    };

} // namespace pooaway::error