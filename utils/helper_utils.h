//
// Created by sajith.nandasena on 17.06.2024.
//

#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H


inline void abort_if_not_impl(bool condition, const char *condition_string, const char *file, const long line)
{
    if (!condition)
    {
        std::printf("[%s:%li] Assertion failed: %s\n", file, line, condition_string);
        std::abort();
    }
}

#define abort_if_not(condition) ::abort_if_not_impl((condition), #condition, __FILE__, __LINE__)

template<class... Args>
void silence_unused(Args &&... args)
{
    ((void) args, ...);
}

namespace utils
{
    struct RethrowFirstArg
    {
        template<class... T>
        void operator()(std::exception_ptr ep, T &&...)
        {
            if (ep)
            {
                std::rethrow_exception(ep);
            }
        }

        template<class... T>
        void operator()(T &&...)
        {
        }
    };
}


#endif //SERVER_UTILS_H
