// Copyright (C) 2024 Ethan Uppal. All rights reserved.

#pragma once

#ifdef CA_PRIVATE_SRC

    #define ca_dynamic_new(T, __len, __cap)                                    \
        (__len = 0, __cap = 16, malloc(sizeof(T) * (__cap)))

    /**
     * @pre `__arrptr` has been allocated with ca_dynamic_new().
     */
    #define ca_dynamic_push(__arrptr, __len, __cap, __newelem)                 \
        do {                                                                   \
            if ((__len) + 1 >= (__cap)) {                                      \
                __cap *= 2;                                                    \
                *(__arrptr) = realloc(*__arrptr,                               \
                    sizeof(*(__arrptr)) * (__cap));                            \
                if (!*(__arrptr)) {                                            \
                    exit(1);                                                   \
                }                                                              \
            }                                                                  \
            (*(__arrptr))[(__len)++] = (__newelem);                            \
        } while (0)

#endif
