#ifndef BASIC_H
#define BASIC_H

#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

//
// Primitive type aliases
//
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef float     f32;
typedef double    f64;


//
// Pair types
//
template< typename T0, typename T1 >
struct Pair {
    T0 first;
    T1 second;
};

template< typename T >
struct Result {
    T value;
    bool ok;
};


//
// Math types
//

inline float random_float(float min, float max) {
    float w = max - min;
    return (rand()/float(RAND_MAX)) * w + min;
}

// generates random float between -1 and 1
inline float random_float() {
    return random_float(-1.0f, 1.0f);
}

// Returns a random integer in the range [min, max)
// Make sure that min < max
inline int random_int(int min, int max) {
    assert(min < max && "min must be less than max");
    return min + rand() % (max - min);
}

template <int N>
struct Vec {
    float v[N] {};

    Vec() {
        for (int i = 0; i < N; ++i) {
            v[i] = 0.0f;
        }
    }

    float& operator[](int index) {
        assert(index < N);
        return v[index];
    }

    const float& operator[](int index) const {
        assert(index < N);
        return v[index];
    }

    Vec operator+(const Vec& other) const {
        Vec result;
        for (int i = 0; i < N; ++i) {
            result.v[i] = v[i] + other.v[i];
        }
        return result;
    }

    Vec operator-(const Vec& other) const {
        Vec result;
        for (int i = 0; i < N; ++i) {
            result.v[i] = v[i] - other.v[i];
        }
        return result;
    }

    Vec operator*(float s) const {
        Vec result;
        for (int i = 0; i < N; ++i) {
            result[i] = v[i] * s;
        }
        return result;
    }

    Vec operator/(float s) const {
        Vec result;
        for (int i = 0; i < N; ++i) {
            result[i] = v[i] / s;
        }
        return result;
    }

    Vec operator-() const {
        return *this * -1.0f;
    }

    Vec& operator+=(const Vec& other) {
        *this = *this + other;
        return *this;
    }

    Vec& operator-=(const Vec& other) {
        *this = *this - other;
        return *this;
    }

    Vec& operator*=(float s) {
        *this = *this * s;
        return *this;
    }

    Vec& operator/=(float s) {
        *this = *this / s;
        return *this;
    }
};

template< int N >
inline Vec<N> operator*(float s, const Vec<N> &v) {
    return v * s;
}

template< int N >
inline float dot(const Vec<N> &v, const Vec<N> &w) {
    float result = 0;
    for (int i = 0; i < N; ++i) {
        result += v[i] * w[i];
    }
    return result;
}

template< int N >
inline float length(const Vec<N> &v) {
    return sqrtf(dot(v, v));
}

template< int N >
inline Vec<N> normalize(const Vec<N> &v) {
    // float len = length(v);
    // assert(len > 1e-6f && "Cannot normalize a near-zero vector");
    return v / length(v);
}

template< int N >
inline Vec<N> random_unit_vec() {
    while (true) {
        Vec<N> v;
        for (int i = 0; i < N; ++i) {
            v[i] = random_float();
        }
        float len = length(v);
        if (len <= 1.0f && len > 0.01) {
            return normalize(v);
        }
    }
}

struct Vec2 : public Vec<2> {
    Vec2() : Vec<2>{} {}

    Vec2(float x, float y) : Vec<2>{} {
        v[0] = x;
        v[1] = y;
    }

    Vec2(const Vec<2> &other) : Vec<2>{other} {}
    Vec2 &operator=(const Vec<2> &other) {
        if (this != &other) { // check for self-assignment.
            Vec<2>::operator=(other);
        }
        return *this;
    }

    float &x() { return this->v[0]; }
    const float &x() const { return this->v[0]; }

    float &y() { return this->v[1]; }
    const float &y() const { return this->v[1]; }
};

inline Vec2 rotate(Vec2 v, float angle_rad) {
    float cos_angle = cosf(angle_rad);
    float sin_angle = sinf(angle_rad);
    return {v.x() * cos_angle - v.y() * sin_angle, v.x() * sin_angle + v.y() * cos_angle};
}

struct Vec3 : public Vec<3> {
    Vec3() : Vec<3>{} {}

    Vec3(float x, float y, float z) : Vec<3>{} {
        v[0] = x;
        v[1] = y;
        v[2] = z;
    }

    Vec3(const Vec<3> &other) : Vec<3>{other} {}
    Vec3 &operator=(const Vec<3> &other) {
        if (this != &other) { // check for self-assignment.
            Vec<3>::operator=(other);
        }
        return *this;
    }

    float &x() { return this->v[0]; }
    const float &x() const { return this->v[0]; }

    float &y() { return this->v[1]; }
    const float &y() const { return this->v[1]; }

    float &z() { return this->v[2]; }
    const float &z() const { return this->v[2]; }

    float &r() { return this->v[0]; }
    const float &r() const { return this->v[0]; }

    float &g() { return this->v[1]; }
    const float &g() const { return this->v[1]; }

    float &b() { return this->v[2]; }
    const float &b() const { return this->v[2]; }
};

struct Vec4 : public Vec<4> {
    Vec4() : Vec<4>{} {}

    Vec4(float x, float y, float z, float w) : Vec<4>{} {
        v[0] = x;
        v[1] = y;
        v[2] = z;
        v[3] = w;
    }

    Vec4(const Vec<4> &other) : Vec<4>{other} {}
    Vec4 &operator=(const Vec<4> &other) {
        if (this != &other) { // check for self-assignment.
            Vec<4>::operator=(other);
        }
        return *this;
    }

    float &x() { return this->v[0]; }
    const float &x() const { return this->v[0]; }

    float &y() { return this->v[1]; }
    const float &y() const { return this->v[1]; }

    float &z() { return this->v[2]; }
    const float &z() const { return this->v[2]; }

    float &w() { return this->v[3]; }
    const float &w() const { return this->v[3]; }

    float &r() { return this->v[0]; }
    const float &r() const { return this->v[0]; }

    float &g() { return this->v[1]; }
    const float &g() const { return this->v[1]; }

    float &b() { return this->v[2]; }
    const float &b() const { return this->v[2]; }

    float &a() { return this->v[3]; }
    const float &a() const { return this->v[3]; }
};

//
// Defer
//
template <typename F>
struct privDefer {
	F f;
	privDefer(F f) : f(f) {}
	~privDefer() { f(); }
};

template <typename F>
inline privDefer<F> defer_func(F f) {
	return privDefer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})


#endif