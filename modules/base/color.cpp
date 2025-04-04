
#include "./color.h"

namespace eokas {
    
    Color::Color(f32_t r, f32_t g, f32_t b, f32_t a)
        : r(r), g(g), b(b), a(a) {
    }
    
    Color::Color(const Color& c)
        : r(c.r), g(c.g), b(c.b), a(c.a) {
    }
    
    Color::~Color() {
    }
    
    Color& Color::operator=(const Color& c) {
        r = c.r;
        g = c.g;
        b = c.b;
        a = c.a;
        return *this;
    }
    
    Color Color::operator-() {
        f32_t nr = (f32_t) (1.0 - r);
        f32_t ng = (f32_t) (1.0 - g);
        f32_t nb = (f32_t) (1.0 - b);
        f32_t na = (f32_t) (1.0 - a);
        return Color(nr, ng, nb, na);
    }
    
    Color Color::operator+(const Color& c) {
        return Color(r + c.r, g + c.g, b + c.b, a + c.a);
    }
    
    Color Color::operator-(const Color& c) {
        return Color(r - c.r, g - c.g, b - c.b, a - c.a);
    }
    
    Color Color::operator*(const Color& c) {
        return Color(r * c.r, g * c.g, b * c.b, a * c.a);
    }
    
    Color& Color::operator+=(const Color& c) {
        r += c.r;
        g += c.g;
        b += c.b;
        a += c.a;
        return *this;
    }
    
    Color& Color::operator-=(const Color& c) {
        r -= c.r;
        g -= c.g;
        b -= c.b;
        a -= c.a;
        return *this;
    }
    
    Color& Color::operator*=(const Color& c) {
        r *= c.r;
        g *= c.g;
        b *= c.b;
        a *= c.a;
        return *this;
    }
    
    bool Color::operator==(const Color& c) {
        return _FloatEqual(r, c.r) && _FloatEqual(g, c.g) && _FloatEqual(b, c.b) && _FloatEqual(a, c.a);
    }
    
    bool Color::operator!=(const Color& c) {
        return !(_FloatEqual(r, c.r) && _FloatEqual(g, c.g) && _FloatEqual(b, c.b) && _FloatEqual(a, c.a));
    }
    
    Color& Color::clamp(f32_t min, f32_t max) {
        r = r < min ? min : r;
        r = r > max ? max : r;
        g = g < min ? min : g;
        g = g > max ? max : g;
        b = b < min ? min : b;
        b = b > max ? max : b;
        a = a < min ? min : a;
        a = a > max ? max : a;
        return *this;
    }
    
}
