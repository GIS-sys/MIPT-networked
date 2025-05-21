#pragma once

#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <enet/enet.h>


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

    bool _is_view = false; // if generated from packet, then will not delete data itself

    void ensure_cap(size_t additional_size) {
        if (!_is_view) return;
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
        if (!_is_view) return;
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
        if (!_is_view) delete[] _data;
    }

    void check_view() {
        if (_is_view) throw std::runtime_error("BitStream was initialized from a buffer. It can't write anything else");
    }

    // Different WRITE operations
    BitStream& write(const char* ptr, size_t amount) {
        check_view();
        ensure_cap(amount);
        std::memcpy(_data + _skip + _size, ptr, amount);
        _size += amount;
        return *this;
    }

    template <typename T>
    BitStream& operator<<(const T& x) {
        check_view();
        write((char*)&x, sizeof(T));
        return *this;
    }

    template <typename T>
    BitStream& write(const T& x) { return *this << x; }

    BitStream& operator<<(const std::string& x) {
        check_view();
        *this << (size_t)x.size();
        write(x.c_str(), x.size());
        *this << '\0';
        return *this;
    }

    // Different READ operations
    char* read_n(size_t amount) { // CAREFUL - the memory may be freed at any next call!
        _skip += amount;
        _size -= amount;
        return _data + _skip - amount;
    }

    template <typename T>
    BitStream& operator>>(T& x) {
        size_t tsize = sizeof(T);
        std::memcpy((void*)&x, _data + _skip, tsize);
        _skip += tsize;
        _size -= tsize;
        shrink();
        return *this;
    }

    template <typename T>
    BitStream& read(T& x) { return *this >> x; }

    BitStream& operator>>(std::string& x) {
        size_t size;
        *this >> size;
        x = read_n(size + 1);
        return *this;
    }

    // Utils
    size_t size() const { return _size; }
    size_t cap() const { return _cap; }
    size_t skip() const { return _skip; }

    operator bool() const { return _size != 0; }

    ENetPacket* to_enet_packet(_ENetPacketFlag reliable_flag) const {
        ENetPacket* packet = enet_packet_create(nullptr, _size, reliable_flag);
        memcpy(packet->data, _data + _skip, _size);
        return packet;
    }

    ENetPacket* from_enet_packet(ENetPacket* packet) {
        delete[] _data;
        _data = (char*)packet->data;
        _cap = packet->dataLength;
        _size = packet->dataLength;
        _skip = 0;
        _is_view = true;
        return packet;
    }
};
