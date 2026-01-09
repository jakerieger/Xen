#ifndef X_OBJ_INSTANCE_H
#define X_OBJ_INSTANCE_H

#include "xobj.h"

typedef struct {
    xen_obj obj;
    xen_obj_class* class;
    array(xen_value) fields;  // property values, indexed by propery_def.index
} xen_obj_instance;

#define OBJ_AS_INSTANCE(value) ((xen_obj_instance*)VAL_AS_OBJ(value))
#define OBJ_IS_INSTANCE(value) xen_obj_is_type(value, OBJ_INSTANCE)

xen_obj_instance* xen_obj_instance_new(xen_obj_class* class);
bool xen_obj_instance_get(xen_obj_instance* inst, xen_obj_str* name, xen_value* out);
bool xen_obj_instance_set(xen_obj_instance* inst, xen_obj_str* name, xen_value value);

#endif