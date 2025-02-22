#ifndef ARRAY_H
#define ARRAY_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <new>

template< typename T >
struct Array {
    unsigned char *elements = nullptr;
    int m_size = 0;
    int m_capacity = 0;
    bool capacity_locked = false;

    int size() const { return m_size; }
    int capacity() const { return m_capacity; }

    T *data() { return m_size == 0 ? nullptr : &(this->operator[](0)); }

    // Returns a pointer to the pushed element
    T *push(const T &value) {
        if (m_capacity == 0) {
            reserve(8);
        }
        else if (m_size >= m_capacity) {
            reserve(m_capacity*2);
        }
        new (get_element_ptr(m_size)) T{ value };
        ++m_size;
        return get_element_ptr(m_size-1);
    };

    void reserve(int new_capacity) {
        if (new_capacity <= m_capacity) {
            return;
        }

        if (capacity_locked) {
            fprintf(stderr, "Array::reserve: tried to reserve a larger element buffer but the array's capacity is locked.");
            exit(1);
        }

        if (!elements) {
            m_capacity = new_capacity;
            elements = (unsigned char*)malloc(m_capacity * sizeof(T));
            assert(elements);
            return;
        }

        unsigned char *new_elements = (unsigned char*)malloc(new_capacity * sizeof(T));
        assert(new_elements);
        // copy over elements to new buffer
        for (int i = 0; i < m_size; ++i) {
            unsigned char *new_element_ptr = new_elements + i * sizeof(T);
            new (new_element_ptr) T{ *get_element_ptr(i) };
        }
        // destruct elements in old buffer
        destruct_elements();
        // free old buffer
        free(elements);

        m_capacity = new_capacity;
        elements = new_elements;
    }

    // In combination with reserve very convenient for using Array as a fixed size arena
    // guaranteeing pointer-stability.
    // e.g.:
    // Array<Tree_Node> nodes;
    // nodes.reserve(1000);
    // nodes.lock_capacity();
    // // start pushing Tree_Nodes that can reference each other by pointers
    void lock_capacity() {
        capacity_locked = true;
    }

    void unlock_capacity() {
        capacity_locked = false;
    }

    void destroy() {
        if (!elements) { return; }
        destruct_elements();
        free(elements);
        elements = nullptr;
        m_size = 0;
        m_capacity = 0;
    }

    // Clears the array without deallocating the backing element buffer
    void clear() {
        destruct_elements();
        m_size = 0;
    }

    T &operator[](int index) {
        verify_index(index);
        return *get_element_ptr(index);
    }

    const T &operator[](int index) const {
        verify_index(index);
        return *get_element_ptr(index);
    }

    //
    // Helpers
    //
    void destruct_elements() {
        for (int i = 0; i < m_size; ++i) {
            T *element = get_element_ptr(i);
            element->~T();
        }
    }

    T *get_element_ptr(int index) {
        return (T*)(elements + index * sizeof(T));
    }

    void verify_index(int index) {
        if (index < 0 || index >= m_size) {
            fprintf(stderr, "Array: index out of bounds");
            exit(1);
        }
    }


};

#endif