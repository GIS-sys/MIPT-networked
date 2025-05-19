#pragma once

#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>

class BitStream {
    // Stores everything in array data
    // ......#############%%%%%%%%%%%%%%%
    // ^     ^            ^              ^
    // 0   _skip    _skip + _size      _cap
    // where . is garbage data
    //       # is useful data
    //       % is not yet used space
    // Ensures that _skip <= _cap / 2
    char* _data;
    size_t _cap = 0;
    size_t _size = 0;
    size_t _skip = 0;

    void ensure_cap(size_t additional_size) {
        if (_skip + _size + additional_size < _cap) return;
        size_t new_cap = _cap;
        while (_skip + _size + additional_size >= new_cap) {
            new_cap *= 2;
        }
        char* new_data = new char[new_cap];
        memcpy(new_data, _data, _cap);
        delete[] _data;
        _data = new_data;
        _cap = new_cap;
    }

    void shrink() {
        if (_skip * 2 < _cap) return;
        size_t new_cap = 1;
        while (_cap - _skip >= new_cap) {
            new_cap *= 2;
        }
        char* new_data = new char[new_cap];
        memcpy(new_data, _data + _skip, _cap - _skip);
        delete[] _data;
        _data = new_data;
        _cap = new_cap;
        _skip = 0;
    }

public:
    BitStream() {
        _data = new char[1];
        _cap = 1;
        _size = 0;
        _skip = 0;
    }

    ~BitStream() {
        delete[] _data;
    }

    // Different WRITE operations
    BitStream& write(const char* ptr, size_t amount) {
        ensure_cap(amount);
        std::memcpy(_data + _skip + _size, ptr, amount);
        _size += amount;
        return *this;
    }

    template <typename T>
    BitStream& operator<<(const T& x) {
        write((char*)&x, sizeof(T));
        return *this;
    }

    BitStream& operator<<(const std::string& x) {
        *this << (size_t)x.size();
        write(x.c_str(), x.size());
        *this << '\0';
        return *this;
    }

    // Different READ operations
    char* read(size_t amount) { // CAREFUL - the memory may be freed at any next call!
        _skip += amount;
        _size -= amount;
        return _data + _skip - amount;
    }

    template <typename T>
    BitStream& operator>>(T* x) {
        size_t tsize = sizeof(T);
        std::memcpy((void*)x, _data + _skip, tsize);
        _skip += tsize;
        _size -= tsize;
        shrink();
        return *this;
    }

    BitStream& operator>>(size_t x) {
        throw std::runtime_error("BitStream got << size_t - please use explicitly pointers");
    }

    BitStream& operator>>(std::string* x) {
        size_t size;
        *this >> &size;
        *x = read(size + 1);
        return *this;
    }

    // Utils
    size_t size() const { return _size; }
    size_t cap() const { return _cap; }
    size_t skip() const { return _skip; }

    operator bool() const { return _size != 0; }
};
