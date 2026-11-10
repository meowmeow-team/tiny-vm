#pragma once
#include <cstdint>
#include <exception>
#include <string>

namespace vm {

// Базовый класс для всех ошибок виртуальной машины
class vm_error : public std::exception {
public:
    explicit vm_error(std::string message);
    [[nodiscard]] const char* what() const noexcept override;

protected:
    std::string msg;
};

// Ошибка ввода-вывода (не удалось открыть файл, пустой путь)
class io_error : public vm_error {
public:
    explicit io_error(const std::string& message);
};

// Ошибка токенизации в лексере
class tokenizing_error : public vm_error {
public:
    tokenizing_error(const char* message, const char* word);
};

// Ошибка разбора грамматики в парсере
class syntax_error : public vm_error {
public:
    syntax_error(const char* message, uint32_t row, uint32_t col);
};

}  // namespace vm