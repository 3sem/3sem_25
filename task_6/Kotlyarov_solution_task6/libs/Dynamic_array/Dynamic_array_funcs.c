#include "Dynamic_array_funcs.h"

bool Dynamic_array_ctor(Dynamic_array *array, size_t initial_capacity, int fill_byte) {

    array->data = (char*) malloc(initial_capacity * sizeof(char));
    if(!array->data) {

        fprintf(stderr, "ERROR in func 'Dynamic_array_ctor': memory was not allocated\n");
        return false;
    }

    array->size = 0;
    array->capacity = initial_capacity;
    array->fill_byte = fill_byte;
    memset(array->data, fill_byte, initial_capacity);

    return true;
}

void Dynamic_array_dtor(Dynamic_array *array) {

    if(array) {

        free(array->data);
        array->data = NULL;
        array->size = 0;
        array->capacity = 0;
    }
}

bool _Dynamic_array_resize_if_needed(Dynamic_array *array, size_t size) {
    if (array->size > SIZE_MAX - size) {
        fprintf(stderr, "ERROR: Size would overflow\n");
        return false;
    }
    
    size_t new_total_size = array->size + size;
    
    if (new_total_size > array->capacity) {
        size_t new_capacity = array->capacity;
        if (new_capacity > SIZE_MAX / Expansion_coeff) {
            new_capacity = new_total_size;
        } else {
            new_capacity = array->capacity * Expansion_coeff;
            if (new_capacity < new_total_size) {
                new_capacity = new_total_size;
            }
        }
        
        if (new_capacity < new_total_size) {
            fprintf(stderr, "ERROR: New capacity insufficient\n");
            return false;
        }
        
        char* tmp_ptr = (char*) realloc(array->data, new_capacity);
        if (!tmp_ptr) {
            new_capacity = new_total_size;
            tmp_ptr = (char*) realloc(array->data, new_capacity);
            if (!tmp_ptr) {
                fprintf(stderr, "ERROR in func '_Dynamic_array_resize_if_needed': memory was not allocated\n");
                return false;
            }
        }

        array->capacity = new_capacity;
        array->data = tmp_ptr;
        
        if (new_capacity > array->size) {
            memset(array->data + array->size, array->fill_byte, new_capacity - array->size);
        }
    }

    return true;
}


bool Dynamic_array_push_back(Dynamic_array *array, const void* element, int size) {

    if (!_Dynamic_array_resize_if_needed(array, size)) {
        return false;
    }

    memcpy(array->data + array->size, element, size);
    array->size += size;

    return true;
}

bool Dynamic_array_pop_back(Dynamic_array *array, int size) {

    if (array->size == 0) {

        fprintf(stderr, "Error: Dynamic_array is empty\n");
        return false;
    }

    if (array->size < (uint64_t) size) {

        fprintf(stderr, "Error: array size is less than element\n");
        return false;
    }

    array->size -= size;
    memset(array->data + array->size, array->fill_byte, size);

    return true;
}

void Dynamic_array_resize(Dynamic_array *array, size_t size_to_add) {

    array->size += size_to_add;
    _Dynamic_array_resize_if_needed(array, 0);
}
