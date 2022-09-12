#pragma once

#include <cstddef>
#include <utility>

namespace neat {

template <class T, size_t Len, size_t Align>
class PImpl {
    alignas(Align) std::byte data_[Len];

    T* ptr() noexcept {
        return reinterpret_cast<T*>(data_);
    }

    const T* ptr() const noexcept {
        return reinterpret_cast<const T*>(data_);
    }

  public:
    template <class... Args>
    explicit PImpl(Args&&... args) {
        static_assert(Len == sizeof(T), "Len and sizeof(T) mismatch");
        static_assert(Align == alignof(T), "Align and alignof(T) mismatch");
        new (ptr()) T(std::forward<Args>(args)...);
    }

    PImpl(PImpl&& rhs) noexcept {
        new (ptr()) T(std::move(*rhs.ptr()));
    }

    T* operator->() noexcept {
        return ptr();
    }

    const T* operator->() const noexcept {
        return ptr();
    }

    T& operator*() noexcept {
        return *ptr();
    }

    const T& operator*() const noexcept {
        return *ptr();
    }

    ~PImpl() noexcept {
        ptr()->~T();
    }
};

}  // namespace neat
