/**
 * \file proj.h
 * \brief Project metadata for static and dynamic retrieval.
 * \copyright Copyright (C) 2023 Ethan Uppal. All rights reserved.
 * \author Ethan Uppal
 */

#pragma once

#define PROJECT_NAME "libcmdapp"
#define PROJECT_NAMESPACE ca

#define __join_eval(x, y) x##y
#define __join(x, y) __join_eval(x, y)
#define _PROJECT_NAMESPACE(x) __join(PROJECT_NAMESPACE, _##x)

struct _project_version {
    int major;
    int minor;
    int patch;
    const char* string;
};
#define PROJECT_VERSION_CREATE(major, minor, patch)                            \
    ((struct _project_version){                                                \
        major, minor, patch, "v" #major "." #minor "." #patch})

#define PROJECT_VERSION PROJECT_VERSION_CREATE(0, 0, 0)
#define PROJECT_AUTHOR "Ethan Uppal"
#define PROJECT_COPYRIGHT "Copyright (C) 2023 Ethan Uppal. All rights reserved."

void _PROJECT_NAMESPACE(project_print)(void);

#ifdef CA_PUBLIC_SRC
    #undef __join_eval
    #undef __join
    #undef _PROJECT_NAMESPACE
#endif
