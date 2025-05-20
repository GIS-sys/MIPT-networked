#include "bitstream.h"

#include <iostream>


struct Simple {
    int a;
    char b;
    bool c;

    friend std::ostream& operator<<(std::ostream& os, const Simple& simple) {
        return os << "Simple(a=" << simple.a << ",b=" << simple.b << ",c=" << simple.c << ")";
    }
};


struct Complex {
    std::string a;
    Simple b;

    friend std::ostream& operator<<(std::ostream& os, const Complex& complex) {
        return os << "Complex(a=" << complex.a << ",b=" << complex.b << ")";
    }

    friend BitStream& operator<<(BitStream& bs, const Complex& complex) {
        bs << complex.a;
        bs << complex.b;
        return bs;
    }

    friend BitStream& operator>>(BitStream& bs, Complex& complex) {
        bs >> complex.a;
        bs >> complex.b;
        return bs;
    }
};


int main() {
    {
        Simple x{ .a = 1, .b = 'a', .c = true };
        BitStream bs;
        bs << x.a << x.b << x.c;

        Simple y;
        bs >> y.a >> y.b >> y.c;

        std::cout << "Was " << x << std::endl;
        std::cout << "Now " << y << std::endl;
    }
    {
        Complex x{ .a = "qwe", .b = { .a = -3, .b = 'z', .c = false} };
        BitStream bs;
        bs << x;

        Complex y;
        bs >> y;

        std::cout << "Was " << x << std::endl;
        std::cout << "Now " << y << std::endl;
    }
}
