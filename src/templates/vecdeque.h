/**
 * @file vecdeque.h
 * @author 9exa
 * @brief An dynamically-sized array where elements can be added on either end. Useful for queues
 *  Supposed to be like Rusts VecDeque, implemented with a ring_buffer
 * @version 0.1
 * @date 2023-04-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef MENGA_VECDEQUE
#define MENGA_VECDEQUE

#include <algorithm>
#include <cstdint>
#include <iostream>
#include "mengumath.h"

namespace Mengu {

template<class T>
class VecDeque {
private:
    T *_data = nullptr;

    uint32_t _front = 0;
    // uint32_t _end = 0;
    uint32_t _size = 0;
    uint32_t _capacity = 0;

public:
    VecDeque() {}
    VecDeque(const VecDeque &from) {
        _front = from._front;
        _size = from._size;
        _capacity = from._capacity;

        if (_capacity > 0) {
            _data = new T[_capacity];
            for (uint32_t i = 0; i < _capacity; i++) {
                _data[i] = from._data[i];
            }
        }
        
    }

    ~VecDeque() {
        if (_data != nullptr) {
            delete[] _data;
        }
    }

    inline uint32_t size() const {
        return _size;
    }

    inline uint32_t capacity() const {
        return _capacity;
    }

    inline const T *data() const {
        return _data;
    } 

    void reserve(const uint32_t &new_cap) {
        if (_capacity < new_cap) {
            T *new_data = new T[new_cap];

            // copy and initialise new array
            uint32_t i = 0;
            if (_data != nullptr) {
                for (; i < _size; i++) {
                    new_data[i] = std::move(_data[(_front + i) % _capacity]);
                }
                delete[] _data;
                _front = 0;
            }
            _data = new_data;
            _capacity = new_cap;
            if (_size > _capacity) {
                resize(_capacity);
            }
        }
    }

    void resize(const uint32_t &new_size) {
        if (_capacity < new_size) {
            uint32_t new_cap = MAX(_capacity, 1);
            while (new_cap < new_size) {
                new_cap = new_cap << 1; // if you leave out the new_cap = the optimizer just skips this loop. 
                                // Which makes this infinite loop bug hard to spot

            }
            reserve(new_cap);
        }
        if (new_size > _size) {
            // expand from back
            if ( _size > 0) {
                for (uint i = _size; i < new_size; i++) {
                    _data[(_front + i) % _capacity] = _data[(_front + _size - 1) % _capacity];
                }
            }
            else {
                for (uint i = _size; i < new_size; i++) {
                    _data[(_front + i) % _capacity] = T();
                }
            }
            
        }
        _size = new_size;
    }

    void resize(const uint32_t &new_size, const T &x) {
        resize(new_size);

        for (uint32_t i = 0; i < new_size; i++) {
            _data[(_front + i) % _capacity] = x;
        }
    }

    inline void push_back(const T &x) {
        resize(_size + 1);
        _data[(_front + _size - 1) % _capacity] = x;
    }

    inline void push_front(const T &x) {
        resize(_size + 1);
        _front = (_front == 0) ? _capacity - 1 : _front - 1;
        _data[_front] = x;
    }

    // adds elements to the end of the queue
    inline void extend_back(const T *array, const uint32_t n) {
        uint32_t old_size = _size;
        resize(_size + n);

        for (uint32_t i = 0; i < n; i++) {
            _data[(_front + old_size + i) % _capacity] = array[i];
        }
    }
    // moves at most n elements from the front of the queue to the array output. outputs memory must be validated elsewhere
    inline uint32_t pop_front_many(T *output, uint32_t n) {
        n = MIN(n, _size); 

        if (output != nullptr) {
            for (uint32_t i = 0; i < n; i++) {
                output[i] = _data[(_front + i) % _capacity];
            }
        }

        if (n != 0){
            _front = (_front + n) % _capacity;

            resize(_size - n);
        }
        

        return n;
    }

    inline uint32_t pop_back_many(T *output, uint32_t n) {
        n = MIN(n, _size);
        if (output != nullptr) {
            for (uint32_t i = 0; i < n; i++) {
                output[i] = _data[(_front + _size - n + i) % _capacity];
            }
        }
        resize(_size - n);

        return n;
    }

    void make_contiguous() {
        if (_size > 0 && _front > 0) {
            T *new_data = new T[_capacity];
            for (uint32_t i = 0; i < _size; i++) {
                new_data[i] = std::move(_data[(_front + i) % _capacity]);
            }

            delete[] _data;

            _data = new_data;
        }
    }

    //// Operators
    inline T &operator[](int i) {
        if (_size == 0) {
            resize(1);
        }
        return _data[(_front + posmod(i, _size)) % _capacity];
    }

    inline const T &operator[](int i) const {
        if (_size == 0) {
            resize(1);
        }
        return _data[(_front + posmod(i, _size)) % _capacity];
    }

    VecDeque &operator=(const VecDeque &from) {
        resize(from._size);
        _front = 0;

        for (uint32_t i = 0; i < _size; i++) {
            _data[i] = from._data[(from._front + _size % from._capacity)];
        }

        return *this;
    };

    // converts the first 'size' items into a contiguous array. -1 does the whole queue
    uint32_t to_array(T *out, int size = -1) {
        if (size == -1) {
            size = _size;
        }

        size = MIN(size, _size);

        for (uint32_t i = 0; i < size; i++) {
            out[i] = _data[(i + _front) % _capacity];
        }

        return size;
    }

    // writes the last 'size' items into a contiguous array
    uint32_t to_array_back(T *out, int size) {
        size = MIN(size, _size);

        for (uint32_t i = 0; i < size; i++) {
            out[i] = _data[(i + _front + _size - size) % _capacity];
        }

        return size;
    }

};

}

#endif