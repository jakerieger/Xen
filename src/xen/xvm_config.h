#ifndef X_VM_CONFIG_H
#define X_VM_CONFIG_H

#include "xcommon.h"

typedef struct {
    size_t mem_size_permanent;
    size_t mem_size_generation;
    size_t mem_size_temporary;
    size_t stack_size;
} xen_vm_config;

#endif
