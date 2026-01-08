#ifndef X_TYPEID_H
#define X_TYPEID_H

#include "xobject.h"

#define TYPEID_UNDEFINED (-1)
#define TYPEID_BOOL (VAL_BOOL)
#define TYPEID_NULL (VAL_NULL)
#define TYPEID_NUMBER (VAL_NUMBER)
#define TYPEID_STRING (VAL_OBJECT + OBJ_STRING)
#define TYPEID_FUNC (VAL_OBJECT + OBJ_FUNCTION)
#define TYPEID_NATIVE_FUNC (VAL_OBJECT + OBJ_NATIVE_FUNC)
#define TYPEID_NAMESPACE (VAL_OBJECT + OBJ_NAMESPACE)
#define TYPEID_ARRAY (VAL_OBJECT + OBJ_ARRAY)
#define TYPEID_BOUND_METHOD (VAL_OBJECT + OBJ_BOUND_METHOD)
#define TYPEID_DICT (VAL_OBJECT + OBJ_DICT)
#define TYPEID_CLASS (VAL_OBJECT + OBJ_CLASS)
#define TYPEID_INSTANCE (VAL_OBJECT + OBJ_INSTANCE)
#define TYPEID_U8ARRAY (VAL_OBJECT + OBJ_U8ARRAY)

inline static i32 xen_typeid_get(xen_value value) {
    switch (value.type) {
        case VAL_BOOL:
            return TYPEID_BOOL;
        case VAL_NULL:
            return TYPEID_NULL;
        case VAL_NUMBER:
            return TYPEID_NUMBER;
        case VAL_OBJECT: {
            switch (OBJ_TYPE(value)) {
                case OBJ_STRING:
                    return TYPEID_STRING;
                case OBJ_FUNCTION:
                    return TYPEID_FUNC;
                case OBJ_NATIVE_FUNC:
                    return TYPEID_NATIVE_FUNC;
                case OBJ_NAMESPACE:
                    return TYPEID_NAMESPACE;
                case OBJ_ARRAY:
                    return TYPEID_ARRAY;
                case OBJ_BOUND_METHOD:
                    return TYPEID_BOUND_METHOD;
                case OBJ_DICT:
                    return TYPEID_DICT;
                case OBJ_CLASS:
                    return TYPEID_CLASS;
                case OBJ_INSTANCE:
                    return TYPEID_NAMESPACE;
                case OBJ_U8ARRAY:
                    return TYPEID_U8ARRAY;
            }
        }
    }
    return TYPEID_UNDEFINED;
}

#endif
