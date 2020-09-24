#pragma once

#include "mail.h"
#include "type_traits.h"
#include "new.h"

namespace lib
{

template <class T>
struct in_place_type_t
{
    explicit in_place_type_t() = default;
};

template <class T>
struct in_place_type_v
{
    static constexpr in_place_type_t<T> value{};
};

namespace internal
{
struct _vtable_format
{
    void (*copy_assign)(const void* src, void* dst);
    void (*move_assign)(void* src, void* dst);
    void (*destroy)(void* target);
};

struct _vtable_creator
{
    template <class T>
    static void create_copier(const void* src, void* dst)
    {
        ::new (dst) T(*static_cast<const T*>(src));
    }
    template <class T>
    static void create_mover(void* src, void* dst)
    {
        ::new (dst) T(move(*static_cast<T*>(src)));
    }
    template <class T>
    static void create_destroyer(void* target)
    {
        static_assert(sizeof(T) > 0, "Incomplete type cannot use.");
        static_cast<T*>(target)->~T();
    }
};

template <class T>
constexpr _vtable_format create_vtable(void) noexcept
{
    return {
        _vtable_creator::create_copier<T>,
        _vtable_creator::create_mover<T>,
        _vtable_creator::create_destroyer<T>,
    };
};

template <class T>
constexpr _vtable_format* get_cached_vtable(void) noexcept
{
    constexpr _vtable_format cache = create_vtable<T>();
    return &cache;
}

class _message
{
private:
    void* const               _buffer;
    internal::_vtable_format* _invoker = nullptr;

public:
    bool has_value(void) const noexcept { return (_invoker); }

    void reset(void) noexcept
    {
        if (_invoker)
        {
            _invoker->destroy(_buffer);
            _invoker = nullptr;
        }
    }

    // template <class T>
    // T* _cast(void) const
    // {
    //     return (_invoker == get_cached_vtable<T>() ? static_cast<T*>(_buffer) : nullptr);
    // }

protected:
    explicit _message(char* buffer) noexcept : _buffer(buffer) {}

    ~_message(void) noexcept { reset(); }

    void copy_data(const _message& rhs)
    {
        reset();
        if (rhs._invoker)
        {
            rhs._invoker->copy_assign(rhs._buffer, _buffer);
            _invoker = rhs._invoker;
        }
    }

    void move_data(_message&& rhs)
    {
        reset();
        if (rhs._invoker)
        {
            rhs._invoker->move_assign(rhs._buffer, _buffer);
            _invoker     = rhs._invoker;
            rhs._invoker = nullptr;
        }
    }

    template <class Decayed, class... Args>
    Decayed& _emplace(Args&&... args)
    {
        reset();
        _invoker = internal::get_cached_vtable<Decayed>();
        new (_buffer) Decayed{forward<Args>(args)...};
        return (*static_cast<Decayed*>(_buffer));
    }
};
}

// template <class T>
// T* any_cast(internal::_message* target) noexcept
// {
//     return (target ? target->_cast<T>() : nullptr);
// }

// template <class T>
// const T* any_cast(const internal::_message* target) noexcept
// {
//     return (target ? target->_cast<T>() : nullptr);
// }

// template <class T>
// T any_cast(internal::_message& target)
// {
//     using U = remove_cvref_t<T>;
//     static_assert(is_constructible<T, const U&>::value, "T is not constructible");
//     return (*any_cast<U>(&target));
// }

// template <class T>
// T any_cast(const internal::_message& target)
// {
//     using U = remove_cvref_t<T>;
//     static_assert(is_constructible<T, U&>::value, "T is not constructible");
//     return (*any_cast<U>(&target));
// }

// template <class T>
// T any_cast(internal::_message&& target)
// {
//     using U = remove_cvref_t<T>;
//     static_assert(is_constructible<T, U>::value, "T is not constructible");
//     return (move(*any_cast<U>(&target)));
// }

template <size_t SIZE, size_t ALIGN = alignof(max_align_t)>
class message : public internal::_message
{
private:
    alignas(ALIGN) char _buffer[SIZE]{};

public:
    constexpr message(void) noexcept : _message(_buffer) {}

    message(const message& rhs) : _message(_buffer) { copy_data(rhs); }

    message(message&& rhs) noexcept : _message(_buffer) { move_data(move(rhs)); }

    template <class T, disable_if_t<disjunction<is_same<message, decay_t<T>>,
                                                is_template_of<in_place_type_t, decay_t<T>>>::value>* = nullptr>
    explicit message(T&& data) : _message(_buffer)
    {
        static_assert(!is_same_template<message, decay_t<T>>::value, "size or align is different");
        static_assert(sizeof(decay_t<T>) <= SIZE, "Insufficient size");
        static_assert(ALIGN % alignof(decay_t<T>) == 0, "Alignment is incorrect");
        _emplace<decay_t<T>>(forward<T>(data));
    }

    template <class T, class... Args>
    explicit message(in_place_type_t<T>, Args&&... data) : _message(_buffer)
    {
        static_assert(!is_same_template<message, decay_t<T>>::value, "size or align is different");
        static_assert(sizeof(decay_t<T>) <= SIZE, "Insufficient size");
        static_assert(ALIGN % alignof(decay_t<T>) == 0, "Alignment is incorrect");
        _emplace<decay_t<T>>(forward<Args>(data)...);
    }

    message& operator=(const message& rhs)
    {
        if (this != &rhs)
        {
            copy_data(rhs);
        }
        return (*this);
    }

    message& operator=(message&& rhs) noexcept
    {
        if (this != &rhs)
        {
            move_data(move(rhs));
        }
        return (*this);
    }

    template <class T, disable_if_t<disjunction<is_same<message, decay_t<T>>,
                                                is_template_of<in_place_type_t, decay_t<T>>>::value>* = nullptr>
    message& operator=(T&& data)
    {
        static_assert(!is_same_template<message, decay_t<T>>::value, "size or align is different");
        static_assert(sizeof(decay_t<T>) <= SIZE, "Insufficient size");
        static_assert(ALIGN % alignof(decay_t<T>) == 0, "Alignment is incorrect");
        _emplace<decay_t<T>>(forward<T>(data));
        return (*this);
    }

    template <class T, class... Args>
    decay_t<T>& emplace(Args&&... args)
    {
        static_assert(sizeof(decay_t<T>) <= SIZE, "Insufficient size");
        static_assert(ALIGN % alignof(decay_t<T>) == 0, "Alignment is incorrect");
        return (_emplace<decay_t<T>>(forward<Args>(args)...));
    }

    void swap(message& rhs) noexcept
    {
        if (has_value() && rhs.has_value())
        {
            if (this != &rhs)
            {
                const message tmp{rhs};
                rhs   = *this;
                *this = tmp;
            }
        }
        else if (!has_value() && !rhs.has_value())
        {
            // NOP
        }
        else
        {
            message* const src = has_value() ? this : &rhs;
            message* const dst = has_value() ? &rhs : this;

            *dst = move(*src);
        }
    }
};

template <size_t SIZE, size_t ALIGN>
void swap(message<SIZE, ALIGN>& lhs, message<SIZE, ALIGN>& rhs) noexcept
{
    lhs.swap(rhs);
}

// template <class... Args>
// struct fitted_message
// {
//     static constexpr size_t SIZE  = largest<Args...>::SIZE;
//     static constexpr size_t ALIGN = largest<Args...>::ALIGN;

//     using type = message<SIZE, ALIGN>;
//     template <class Func>
//     using invoker_type = function<Func, SIZE, ALIGN>;
// };

// template <size_t SIZE, size_t ALIGN, class T, class... Args>
// message<SIZE, ALIGN> make_message(Args&&... args)
// {
//     return (message<SIZE, ALIGN>(in_place_type_v<T>::value, forward<Args>(args)...));
// }

// template <class T, class... Args>
// typename fitted_message<T>::type make_fitted_message(Args&&... args)
// {
//     return (typename fitted_message<T>::type(in_place_type_v<T>::value, forward<Args>(args)...));
// }

// class imail_sender
// {
// public:
//     void send(mail_address from, mail_address to, const message& message_) {}
// };

// class mail_sender : public imail_sender
// {};
}
