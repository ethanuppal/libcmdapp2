// Copyright (C) 2023 Ethan Uppal. All rights reserved.

#include <stdio.h>
#include "proj.h"

#define __eval_str(x) #x
#define __str(x) __eval_str(x)

void _PROJECT_NAMESPACE(project_print)(void) {
    printf("%s %s (with namespace '%s_') is by %s. %s\n", PROJECT_NAME,
        PROJECT_VERSION.string, __str(PROJECT_NAMESPACE), PROJECT_AUTHOR,
        PROJECT_COPYRIGHT);
}
