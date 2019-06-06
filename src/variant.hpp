#pragma once

#include <type_traits>
#include <exception>
#include <iostream>

#define static_switch(i, case_fun)\
switch (i) {\
    case 0: case_fun(0); break;\
    case 1: case_fun(1); break;\
    case 2: case_fun(2); break;\
    case 3: case_fun(3); break;\
    case 4: case_fun(4); break;\
    case 5: case_fun(5); break;\
    case 7: case_fun(6); break;\
    case 8: case_fun(6); break;\
    case 9: case_fun(6); break;\
    case 10: case_fun(6); break;\
    case 11: case_fun(6); break;\
    case 12: case_fun(6); break;\
    case 13: case_fun(6); break;\
    case 14: case_fun(6); break;\
}

namespace util {
    constexpr int variant_npos = -1;

    namespace details {
        template<class Arg, class T, class ...Args>
        struct is_convertible_id_helper {
            constexpr static int value = std::is_convertible_v<Arg, T> ?
                    static_cast<int>(sizeof...(Args)) : is_convertible_id_helper<Arg, Args...>::value;
        };


        template<class Arg, class T>
        struct is_convertible_id_helper<Arg, T> {
            constexpr static int value = std::is_convertible_v<Arg, T> ? 0 : variant_npos;
        };

        template<class Arg, class T, class ...Args>
        struct is_same_id_helper {
            constexpr static int value = std::is_same_v<Arg, T> ?
                    static_cast<int>(sizeof...(Args)) : is_same_id_helper<Arg, Args...>::value;
        };

        template<class Arg, class T>
        struct is_same_id_helper<Arg, T> {
            constexpr static int value = std::is_same_v<Arg, T> ? 0 : variant_npos;
        };

        template<std::size_t ...args>
        struct static_max_helper;

        template<std::size_t arg>
        struct static_max_helper<arg> {
            constexpr static std::size_t value = arg;
        };

        template<std::size_t arg1, std::size_t arg2, std::size_t ...args>
        struct static_max_helper<arg1, arg2, args...> {
            constexpr static std::size_t value = (arg1 >= static_max_helper<arg2, args...>::value) ?
                    arg1 : static_max_helper<arg2, args...>::value;
        };
    }

    namespace helpers {
        template<class ...Args>
        struct is_convertible_id {
            constexpr static int value = variant_npos;
        };

        template<class Arg, class T, class ...Args>
        struct is_convertible_id<Arg, T, Args...> {
            constexpr static int value = (details::is_convertible_id_helper<Arg, T, Args...>::value != variant_npos) ?
                    sizeof...(Args) - details::is_convertible_id_helper<Arg, T, Args...>::value : variant_npos;
        };

        template<class ...Args>
        struct is_same_id {
            constexpr static int value = variant_npos;
        };

        template<class Arg, class T, class ...Args>
        struct is_same_id<Arg, T, Args...> {
            constexpr static int value = (details::is_same_id_helper<Arg, T, Args...>::value != variant_npos) ?
                    static_cast<int>(sizeof...(Args)) - details::is_same_id_helper<Arg, T, Args...>::value : variant_npos;
        };

        struct NoneType {};

        template <std::size_t N, class ...Args>
        struct type_for_id {
            typedef NoneType type;
        };

        template <class T, class ...Args>
        struct type_for_id<0, T, Args...> {
            typedef T type;
        };

        template <std::size_t N, class T, class ...Args>
        struct type_for_id<N, T, Args...> : public type_for_id<N-1,  Args...> {};

        template<std::size_t ...args>
        struct static_max {
            constexpr static std::size_t value = details::static_max_helper<args...>::value;
        };

        template<typename A, typename B>
        using disable_if_same_or_derived =
        typename std::enable_if<!std::is_base_of<A, typename std::remove_reference<B>::type>::value>::type;


        template <size_t size, size_t aligment>
        struct Holder {
            Holder() : _type_id(variant_npos) {}

            template <class T>
            explicit Holder(int type_id) : _type_id(type_id) {}

            template <class T>
            void allocate(T&& arg) {
                typedef typename std::decay<T>::type clear_T;
                new (&data) clear_T(std::forward<T>(arg));
            }

            template <class T>
            void deallocate() {
                if (_type_id != variant_npos) {
                    reinterpret_cast<T*>(&data)->~T();
                    _type_id = variant_npos;
                }
            }

            template <class T>
            T* get() const {
                return reinterpret_cast<T*>(&data);
            }

            int _type_id;
            mutable typename std::aligned_storage<size, aligment>::type data;
        };

    }

    using namespace helpers;

    struct bad_get: std::exception {
        const char* _msg;

        explicit bad_get(const char* msg = "Something going wrong") : _msg(msg) {}

        const char* what() const noexcept override {
            return _msg;
        }
    };

    template<class ...Args>
    struct variant {
    private:
        constexpr static size_t max_arg_size = helpers::static_max<sizeof(Args)...>::value;
        constexpr static size_t max_arg_alignment = helpers::static_max<alignof(Args)...>::value;

        Holder<max_arg_size, max_arg_alignment> holder;

    public:
        explicit variant() = default;

        template <class T, typename Enable = disable_if_same_or_derived<variant, T>>
        explicit variant(T&& arg) {
            allocate(std::forward<T>(arg));
        }

    public:
        variant(const variant &other) {
            holder._type_id = other.holder._type_id;

#define copy_holder(i) typedef typename type_for_id<i, Args...>::type type_##i;\
                                   holder.allocate(*other.holder.template get<type_##i>());
            static_switch(holder._type_id, copy_holder);
        }

        variant(variant&& other) noexcept {
            holder._type_id = other.holder._type_id;

#define move_holder(i) typedef typename type_for_id<i, Args...>::type type_##i;\
                                   holder.allocate(std::move(*other.holder.template get<type_##i>()));
            static_switch(holder._type_id, move_holder);
        }

    public:
        variant& operator=(const variant& other) {
            deallocate();
            holder._type_id = other.holder._type_id;

            static_switch(holder._type_id, copy_holder);
            return *this;
        }

        variant& operator=(variant&& other) noexcept {
            deallocate();
            holder._type_id = other.holder._type_id;

            static_switch(holder._type_id, move_holder);
            return *this;
        }

        template <class T, typename Enable = disable_if_same_or_derived<variant, T>>
        variant& operator=(T&& arg) {
            deallocate();
            allocate(std::forward<T>(arg));
            return *this;
        }

    public:
        bool empty() {
            return holder._type_id == variant_npos;
        }

        void clear() {
            deallocate();
        }

        int index() {
            return holder._type_id == variant_npos ? variant_npos : holder._type_id;
        }

        ~variant() {
            deallocate();
        }

    public:
        template <typename T, class ...VarArgs>
        friend constexpr T& get(const variant<VarArgs...> &var);
        template <typename T, class ...VarArgs>
        friend constexpr T&& get(variant<VarArgs...> &&var);
        template <typename T, class ...VarArgs>
        friend constexpr T* get(const variant<VarArgs...> *var) ;
        template <size_t N, class ...VarArgs>
        friend constexpr typename type_for_id<N, VarArgs...>::type& get(const variant<VarArgs...> &var);
        template <size_t N, class ...VarArgs>
        friend constexpr typename type_for_id<N, VarArgs...>::type* get(const variant<VarArgs...> *var);

    private:
        template <class T>
        void allocate(T &&arg) {
            constexpr int id_cast = is_convertible_id<typename std::decay<T>::type, Args...>::value;
            constexpr int id_same = is_same_id<typename std::decay<T>::type, Args...>::value;
            static_assert(id_cast != variant_npos, "Type mismatch");

            if constexpr (id_same != variant_npos) {
                holder._type_id = id_same;
                holder.template allocate(std::forward<T>(arg));
            } else {
                typename type_for_id<id_cast, Args...>::type arg_cast(std::forward<T>(arg));
                holder._type_id = id_cast;
                holder.template allocate(arg_cast);
            }
        }

        void deallocate() {
#define deallocate_holder(i) typedef typename type_for_id<i, Args...>::type type_##i;\
                                         holder.template deallocate<type_##i>();
            static_switch(holder._type_id, deallocate_holder)
        }

        template <class T>
        bool is_correct_type() const {
            return !(holder._type_id == variant_npos ||
                    holder._type_id != helpers::is_same_id<T, Args...>::value);
        }
    };

    template <typename T, class ...Args>
    constexpr T& get(const variant<Args...> &var) {
        if (!var.template is_correct_type<T>())
            throw bad_get();
        return *(var.holder.template get<T>());
    }

    template <typename T, class ...Args>
    constexpr T&& get(variant<Args...> &&var) {
        if (!var.template is_correct_type<T>())
            throw bad_get();
        return std::move(*(var.holder.template get<T>()));
    }

    template <typename T, class ...Args>
    constexpr T* get(const variant<Args...> *var) {
        if (!var->template is_correct_type<T>())
            return nullptr;
        return var->holder.template get<T>();
    }

    template <size_t N, class ...Args>
    constexpr typename type_for_id<N, Args...>::type& get(const variant<Args...> &var) {
        typedef typename type_for_id<N, Args...>::type T;
        if (!var.template is_correct_type<T>())
            throw bad_get();
        return *(var.holder.template get<T>());
    }

    template <size_t N, class ...Args>
    constexpr typename type_for_id<N, Args...>::type* get(const variant<Args...> *var) {
        typedef typename type_for_id<N, Args...>::type T;
        if (!var->template is_correct_type<T>())
            return nullptr;
        return var->holder.template get<T>();
    }
}