#ifndef DYNAMIC_ARRAY
#define DYNAMIC_ARRAY

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

struct Dynamic_array {
    char*  data;
    uint32_t size;
    uint32_t capacity;
    int fill_byte;
};

typedef struct Dynamic_array Dynamic_array;
static const int Expansion_coeff = 2;

bool Dynamic_array_ctor(Dynamic_array *array, size_t initial_capacity, int fill_byte);
void Dynamic_array_dtor(Dynamic_array *array);
bool _Dynamic_array_resize_if_needed(Dynamic_array *array, int size);
void Dynamic_array_push_back(Dynamic_array *array, const void* element, int size);
bool Dynamic_array_pop_back(Dynamic_array *array, int size);
void Dynamic_array_resize(Dynamic_array *array, size_t size_to_add);

#define INSERT_NOP(array) _Dynamic_array_resize_if_needed(array, 1);\
                          array->size++
#define INSERT_NOPS(array, nops_amount) _Dynamic_array_resize_if_needed(array, nops_amount);\
                                        array->size += nops_amount

#define INSERT_BYTE_BY_PTR(array, byte_ptr) _Dynamic_array_resize_if_needed(array, 1);\
                                             memcpy(array->data+array->size, byte_ptr, sizeof(char));\
                                             array->size++
#define INSERT_BYTE(array, byte) _Dynamic_array_resize_if_needed(array, 1);\
                                 array->data[array->size] = byte;\
                                 array->size++
#define INSERT_WORD(array, word)   Dynamic_array_push_back(array, &word, sizeof(uint16_t))
#define INSERT_DWORD(array, dword) Dynamic_array_push_back(array, &dword, sizeof(uint32_t))
#define INSERT_QWORD(array, qword) Dynamic_array_push_back(array, &qword, sizeof(uint64_t))

#endif
