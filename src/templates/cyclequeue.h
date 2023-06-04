/**
 * @file cyclevector.h
 * @author 9exa
 * @brief An user-determined-size array where pushing an item removes one from the other end. 
 *  Useful for not having to reallocate memory for rapidly appended contiguious data (i.e. sampling)
 * @version 0.1
 * @date 2023-02-21
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef MENGA_CYCLE_QUEUE
#define MENGA_CYCLE_QUEUE

#include <cstdint>
#include <vector>
#include <cstring>
#include <iostream>
#include "mengumath.h"

namespace Mengu {

template <class T>
class CycleQueue {
private:
    T *_data = nullptr;
    uint32_t _size = 0;
    // what part of the data array is the front of the queue and first to be replaced on a push_back()
    uint32_t _front = 0; 
    uint32_t _capacity = 0;

    inline uint32_t start_inc_down1() {
        // assert(size != 0)
        // equiv to posmod (_front + _size - 1) % _size but faster?????
        return _front == 0 ? _size - 1 : _front - 1;
    }

public:
    CycleQueue() {}
    CycleQueue(uint32_t size) {
        if (size > 0) {
            resize(size);
        }
    }
    // implement "copy" so they don't share the same data array
    CycleQueue(const CycleQueue &from) {
        _capacity = from._capacity;
        _data = new T[_capacity];
        std::cout << "from something\n";
        memcpy(_data, from._data, _capacity * sizeof(T));

        _size = from._size;
        _front = from._front;
    }

    ~CycleQueue() {
        delete[] _data;
    }

    inline uint32_t size() const {
        return _size;
    }


    inline void push_back(const T &x) {
        if (_size == 0) return;
        _data[_front] = x;
        _front = (_front + 1) % _size;        
    }

    inline void push_front(const T &x) {
        if (_size == 0) return;
        _front = (_front == 0) ? _size - 1 : _front - 1;
        _data[_front] = x;
    }

    void resize(const uint32_t &new_size) {
        if (_capacity < new_size) {
            uint32_t new_cap = MAX(_capacity, 1);
            while (new_cap < new_size) {
                new_cap = new_cap << 1; // if you leave out the new_cap = the optimizer just skips this loop. 
                                // Which makes this infinite loop bug hard to spot

            }
            reserve(new_cap);
        } else if (new_size < _size) {
            //move element of an array such that those at the front are removed
            uint32_t shift;
            uint32_t i;
            if (_front <= new_size) {
                // shift elements after _front down
                shift = _size - new_size;
                i = _front;
            }
            else {
                // shift elements before _front up
                shift = _front - new_size;
                i = 0;
                _front = 0;
            }
            for (; i < new_size; i++) {
                _data[i] = _data[i + shift];
            }

            // set the deleted slots to be ready for future resizes
            for (; i < _size; i++) {
                _data[i] = T();
            }
        }
        else { // (new_size > size) but reserve() has yet to initialize values
            uint32_t i = _size;
            for (; i < _front; i++) {
                _data[(i + _size) % new_size] = _data[i];
                _data[i] = T();
            }
        }
        _size = new_size;
    }

    inline uint32_t capacity() const {
        return _capacity;
    }

    const T *data() const {
        return _data;
    }

    void reserve(const uint32_t &new_cap) {
        if (_capacity < new_cap) {
            T *new_data = new T[new_cap];
            // copy and initialise new array
            uint32_t i = 0;
            if (_data != nullptr) {
                for (; i < _capacity; i++) {
                    new_data[i] = std::move(_data[i]);
                }
                delete[] _data;

            }
            
            for (; i< new_cap; i++) {
                new_data[i] = T();
            }

            _data = new_data;
            _capacity = new_cap;
            if (_size > _capacity) {
                resize(_capacity);
            }
        }
    }

    // rotates the data array so that ir begins with _friont
    void make_contiguous() {
        T *new_data = new T[_capacity];

        uint32_t i = 0;

        if (_data != nullptr) {
            for (; i < _size; i++) {
                new_data[i] = std::move(_data[(i + _front) % _size]);
            }
            delete[] _data;
        }

        for (; i< _capacity; i++) {
            new_data[i] = T();
        }

        _data = new_data;

    }

    void set(int i, const T &item) {
        if (i > _size) {
            std::cout << "tried to set item " << i << "on CycleQueue of size i. Out of range"<< std::endl;
        }
        _data[posmod(i +_front, _size)] = item;
    }

    const T get(int i) const {
        if (i > _size) {
            std::cout << "tried to get item " << i << "on CycleQueue of size i. Out of range"<< std::endl;
        }
        return _data[posmod(i + _front, _size)];
    }

    // just moves the front of the Ccle queue by an amount
    void rotate(int by) {
        _front = posmod(_front + by, _size);
    }

    //// Operators
    inline T &operator[](int i) {
        return _data[posmod(i + _front, _size)];
    }

    inline const T &operator[](int i) const {
        // std::cout << (i + _front) % _size << std::endl;
        return _data[posmod(i + _front, _size)];
    }

    CycleQueue &operator=(const CycleQueue &from) {
        if (from._size != _size) {
            resize(from._size);
        }
        _front = from._front;
        memcpy(_data, from._data, _size);

        return *this;
    };



    //// Conversions
    // converts the first 'size' items into a contiguous array. -1 does the whole queue
    T* to_array(T *out, int size = -1) const {
        if (size == -1) {
            size = _size;
        }
        for (uint32_t i = 0; i < size; i++) {
            out[i] = _data[(i + _front) % _size];
        }

        return out;
    }

    // converts the first 'size' items into a vector. -1 does the whole queue
    std::vector<T> to_vector(int size = -1) const {
        if (size == -1) {
            size = _size;
        }

        std::vector<T> out;
        out.resize(size);
        for (uint32_t i = 0; i < size; i++) {
            out[i] = _data[(i + _front) % _size];
        }

        return out;
    }
};

template<typename T>
std::string to_string(const CycleQueue<T> &cq) {
    using std::to_string;
    std::string out_string;

    if (cq.size() == 0) {
        return out_string;
    }

    out_string.reserve(to_string(cq[0]).size() * cq.size());
    for (uint32_t i = 0; i < cq.size(); i++) {
        out_string += to_string(cq[i]);
    }
    return out_string;
}

};

#endif