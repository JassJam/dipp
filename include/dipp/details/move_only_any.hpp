#pragma once

#include <bit>
#include <typeinfo>
#include <memory>
#include "result.hpp"

namespace dipp::details
{
    enum class any_storage_type
    {
        null,
        trivial_type,
        small_type,
        large_type
    };

    /// <summary>
    /// A move-only variant of std::any that can store objects of different types.
    /// </summary>
    class move_only_any
    {
    private:
        static constexpr unsigned long long SMALL_BUFFER_SIZE = 32ull;

        struct small_storage_rtti
        {
            void (*move)(void* dest, void* src);
            void (*destruct)(void* data);

            template<typename Ty>
            void init()
            {
                move = &do_move<Ty>;
                destruct = &do_destruct<Ty>;
            }

            template<typename Ty>
            static void do_move(void* dest, void* src)
            {
                std::construct_at(std::bit_cast<Ty*>(dest), std::move(*std::bit_cast<Ty*>(src)));
            }

            template<typename Ty>
            static void do_destruct(void* data)
            {
                std::destroy_at(std::bit_cast<Ty*>(data));
            }
        };

        struct large_storage_rtti
        {
            void (*destruct)(void* data);

            template<typename Ty>
            void init()
            {
                destruct = &do_destroy<Ty>;
            }

            template<typename Ty>
            static void do_destroy(void* data)
            {
                delete std::bit_cast<Ty*>(data);
            }
        };

        struct trivial_storage
        {
            alignas(std::max_align_t) std::byte buffer[SMALL_BUFFER_SIZE]{};
        };

        struct small_storage
        {
            alignas(std::max_align_t) std::byte buffer[SMALL_BUFFER_SIZE]{};
            small_storage_rtti rtti;
        };

        struct large_storage
        {
            void* data;
            large_storage_rtti rtti;
        };

        struct storage
        {
            const std::type_info* type_info{};
            union
            {
                trivial_storage trivial_type;
                small_storage small_type;
                large_storage large_type;
            } u{};
            any_storage_type type{any_storage_type::null};
        };

        template<typename Ty>
        static constexpr bool is_trivial = std::is_trivially_move_constructible_v<Ty> &&         //
                                           std::is_trivially_move_constructible_v<result<Ty>> && //
                                           std::is_trivially_move_assignable_v<Ty> &&            //
                                           std::is_trivially_move_assignable_v<result<Ty>> &&    //
                                           alignof(result<Ty>) <= alignof(std::max_align_t) &&   //
                                           sizeof(result<Ty>) <= SMALL_BUFFER_SIZE;

        template<typename Ty>
        static constexpr bool is_small = std::is_move_constructible_v<Ty> &&                 //
                                         std::is_move_constructible_v<result<Ty>> &&         //
                                         alignof(result<Ty>) <= alignof(std::max_align_t) && //
                                         sizeof(result<Ty>) <= SMALL_BUFFER_SIZE;

        template<typename Ty>
        static constexpr bool is_large = !is_trivial<Ty> && !is_small<Ty>;

    private:
        constexpr move_only_any() noexcept = default;

        template<typename Ty>
        constexpr move_only_any(result<Ty>&& value) noexcept(
            std::is_nothrow_move_assignable_v<result<Ty>>)
        {
            emplace_impl<Ty>(std::forward<result<Ty>>(value));
        }

        template<typename Ty>
        constexpr move_only_any(Ty&& value) noexcept(std::is_nothrow_move_assignable_v<Ty>)
        {
            emplace_impl<Ty>(std::forward<Ty>(value));
        }

    public:
        move_only_any(const move_only_any&) = delete;
        move_only_any& operator=(const move_only_any&) = delete;

        constexpr move_only_any(move_only_any&& other) noexcept
        {
            m_Storage.type_info = other.m_Storage.type_info;
            m_Storage.type = other.m_Storage.type;
            switch (m_Storage.type)
            {
                case any_storage_type::trivial_type:
                    std::memcpy(&m_Storage.u.trivial_type,
                                &other.m_Storage.u.trivial_type,
                                sizeof(trivial_storage));
                    break;
                case any_storage_type::small_type:
                    m_Storage.u.small_type.rtti = other.m_Storage.u.small_type.rtti;
                    m_Storage.u.small_type.rtti.move(&m_Storage.u.small_type.buffer,
                                                     &other.m_Storage.u.small_type.buffer);
                    other.m_Storage.u.small_type.rtti.destruct(
                        &other.m_Storage.u.small_type.buffer);
                    break;
                case any_storage_type::large_type:
                    m_Storage.u.large_type.rtti = other.m_Storage.u.large_type.rtti;
                    m_Storage.u.large_type.data =
                        std::exchange(other.m_Storage.u.large_type.data, nullptr);
                    break;
            }
            other.m_Storage.type = any_storage_type::null;
        }

        constexpr move_only_any& operator=(move_only_any&& other) noexcept
        {
            if (this != &other)
            {
                reset();
                m_Storage.type_info = other.m_Storage.type_info;
                m_Storage.type = other.m_Storage.type;
                switch (m_Storage.type)
                {
                    case any_storage_type::trivial_type:
                        std::memcpy(&m_Storage.u.trivial_type,
                                    &other.m_Storage.u.trivial_type,
                                    sizeof(trivial_storage));
                        break;
                    case any_storage_type::small_type:
                        m_Storage.u.small_type.rtti = other.m_Storage.u.small_type.rtti;
                        m_Storage.u.small_type.rtti.move(&m_Storage.u.small_type.buffer,
                                                         &other.m_Storage.u.small_type.buffer);
                        other.m_Storage.u.small_type.rtti.destruct(
                            &other.m_Storage.u.small_type.buffer);
                        break;
                    case any_storage_type::large_type:
                        m_Storage.u.large_type.rtti = other.m_Storage.u.large_type.rtti;
                        m_Storage.u.large_type.data =
                            std::exchange(other.m_Storage.u.large_type.data, nullptr);
                        break;
                }
                other.m_Storage.type = any_storage_type::null;
            }
            return *this;
        }

        constexpr ~move_only_any()
        {
            reset();
        }

        template<typename Ty, typename... Args>
        [[nodiscard]] static move_only_any make(Args&&... args)
        {
            move_only_any any;
            any.emplace<Ty>(std::forward<Args>(args)...);
            return any;
        }

#ifdef DIPP_USE_RESULT
        template<typename... Args>
        [[nodiscard]] static move_only_any make_error(error_id id)
        {
            move_only_any any;
            any.emplace<error_id>(id);
            return any;
        }
#endif

    public:
        constexpr void reset()
        {
            switch (m_Storage.type)
            {
                case any_storage_type::small_type:
                    m_Storage.u.small_type.rtti.destruct(m_Storage.u.small_type.buffer);
                    break;
                case any_storage_type::large_type:
                    m_Storage.u.large_type.rtti.destruct(m_Storage.u.large_type.data);
                    break;
            }
            m_Storage.type = any_storage_type::null;
        }

        template<typename Ty, typename... Args>
        constexpr result<Ty>& emplace(Args&&... args)
        {
            reset();
            return emplace_impl<Ty>(std::forward<Args>(args)...);
        }

        template<typename Ty>
        [[nodiscard]] result<Ty>* cast() noexcept
        {
            using resulty_type = result<Ty>;

            if (m_Storage.type_info == &typeid(Ty))
            {
                if constexpr (is_trivial<Ty>)
                {
                    return std::bit_cast<resulty_type*>(&m_Storage.u.trivial_type.buffer);
                }
                else if constexpr (is_small<Ty>)
                {
                    return std::bit_cast<resulty_type*>(&m_Storage.u.small_type.buffer);
                }
                else
                {
                    return std::bit_cast<resulty_type*>(m_Storage.u.large_type.data);
                }
            }
            return nullptr;
        }

        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return m_Storage.type == any_storage_type::null;
        }

#ifdef DIPP_USE_RESULT
        [[nodiscard]] error_id error() noexcept
        {
            return cast<error_id>()->error();
        }

        [[nodiscard]] constexpr bool has_error() const noexcept
        {
            return m_Storage.type_info == &typeid(error_id);
        }
#endif

        [[nodiscard]] constexpr const std::type_info& type() const noexcept
        {
            if (empty())
            {
                return typeid(void);
            }
            return *m_Storage.type_info;
        }

        void swap(move_only_any& other) noexcept
        {
            std::swap(m_Storage, other.m_Storage);
        }

    private:
        template<typename Ty, typename... Args>
        constexpr result<Ty>& emplace_impl(Args&&... args)
        {
            using result_type = result<Ty>;
            using pointer_type = std::add_pointer_t<result_type>;

            m_Storage.type_info = &typeid(Ty);
            if constexpr (is_trivial<Ty>)
            {
                auto obj = std::bit_cast<pointer_type>(&m_Storage.u.trivial_type.buffer);
                construct_result(obj, std::forward<Args>(args)...);

                m_Storage.type = any_storage_type::trivial_type;

                return *obj;
            }
            else if constexpr (is_small<Ty>)
            {
                auto obj = std::bit_cast<pointer_type>(&m_Storage.u.small_type.buffer);
                construct_result(obj, std::forward<Args>(args)...);

                m_Storage.type = any_storage_type::small_type;
                m_Storage.u.small_type.rtti.init<result_type>();

                return *obj;
            }
            else
            {
                auto obj = new result_type(std::forward<Args>(args)...);
                m_Storage.u.large_type.data = obj;

                m_Storage.type = any_storage_type::large_type;
                m_Storage.u.large_type.rtti.init<result_type>();

                return *obj;
            }
        }

        template<typename Ty, typename... Args>
        constexpr void construct_result(result<Ty>* ptr, Args&&... args)
        {
            using result_type = result<Ty>;
            if constexpr (std::is_constructible_v<result_type, Args...>)
            {
                if (std::is_constant_evaluated())
                {
                    std::construct_at(ptr, std::forward<Args>(args)...);
                }
                else
                {
                    new (ptr) result_type(std::forward<Args>(args)...);
                }
            }
            else
            {
                if (std::is_constant_evaluated())
                {
                    std::construct_at(ptr, Ty{std::forward<Args>(args)...});
                }
                else
                {
                    new (ptr) result_type(Ty{std::forward<Args>(args)...});
                }
            }
        }

    private:
        storage m_Storage;
    };

    /// <summary>
    /// Create a move_only_any object with the given type and arguments
    /// </summary>
    template<typename Ty, typename... ArgsTy>
    [[nodiscard]] constexpr auto make_any(ArgsTy&&... args)
    {
        return move_only_any::make<Ty>(std::forward<ArgsTy>(args)...);
    }
}
