#pragma once

#include <ocr/ErrorCode.hpp>

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace ocr {

/**
 * @brief Modern, value-or-error container providing explicit error handling.
 */
template <typename T, typename E = OcrError>
class Result {
public:
    using value_type = T;
    using error_type = E;

    // Constructors
    Result(const T& val) : data_(val) {}
    Result(T&& val) : data_(std::move(val)) {}
    Result(const E& err) : data_(err) {}
    Result(E&& err) : data_(std::move(err)) {}

    // Factory methods
    static Result<T, E> success(T val) {
        return Result<T, E>(std::move(val));
    }

    static Result<T, E> failure(E err) {
        return Result<T, E>(std::move(err));
    }

    static Result<T, E> failure(ErrorCode code, std::string msg = "", std::string path = "", std::optional<int> page = std::nullopt) {
        return Result<T, E>(OcrError{code, std::move(msg), std::move(path), page});
    }

    [[nodiscard]] bool has_value() const noexcept {
        return std::holds_alternative<T>(data_);
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return has_value();
    }

    [[nodiscard]] const T& value() const & {
        if (!has_value()) {
            throw std::runtime_error("Attempted to access value of an erroneous Result: " + error().toString());
        }
        return std::get<T>(data_);
    }

    [[nodiscard]] T& value() & {
        if (!has_value()) {
            throw std::runtime_error("Attempted to access value of an erroneous Result: " + error().toString());
        }
        return std::get<T>(data_);
    }

    [[nodiscard]] T&& value() && {
        if (!has_value()) {
            throw std::runtime_error("Attempted to access value of an erroneous Result: " + error().toString());
        }
        return std::get<T>(std::move(data_));
    }

    [[nodiscard]] const E& error() const & noexcept {
        return std::get<E>(data_);
    }

    [[nodiscard]] E& error() & noexcept {
        return std::get<E>(data_);
    }

    [[nodiscard]] E&& error() && noexcept {
        return std::get<E>(std::move(data_));
    }

    [[nodiscard]] const T* operator->() const noexcept {
        return &value();
    }

    [[nodiscard]] T* operator->() noexcept {
        return &value();
    }

    [[nodiscard]] const T& operator*() const & noexcept {
        return value();
    }

    [[nodiscard]] T& operator*() & noexcept {
        return value();
    }

    [[nodiscard]] T value_or(T default_val) const & {
        return has_value() ? std::get<T>(data_) : default_val;
    }

private:
    std::variant<T, E> data_;
};

/**
 * @brief Specialization for void return types.
 */
template <typename E>
class Result<void, E> {
public:
    using value_type = void;
    using error_type = E;

    Result() : has_value_(true) {}
    Result(const E& err) : has_value_(false), error_(err) {}
    Result(E&& err) : has_value_(false), error_(std::move(err)) {}

    static Result<void, E> success() {
        return Result<void, E>();
    }

    static Result<void, E> failure(E err) {
        return Result<void, E>(std::move(err));
    }

    static Result<void, E> failure(ErrorCode code, std::string msg = "", std::string path = "", std::optional<int> page = std::nullopt) {
        return Result<void, E>(OcrError{code, std::move(msg), std::move(path), page});
    }

    [[nodiscard]] bool has_value() const noexcept { return has_value_; }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value_; }

    [[nodiscard]] const E& error() const & noexcept { return error_; }
    [[nodiscard]] E& error() & noexcept { return error_; }
    [[nodiscard]] E&& error() && noexcept { return std::move(error_); }

private:
    bool has_value_{true};
    E error_{};
};

} // namespace ocr
