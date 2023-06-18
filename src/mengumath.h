#ifndef MENGA_MATH
#define MENGA_MATH

#include <cmath>
#include <cstdint>


namespace Mengu {
// Copied from Godot

#define MATH_PI 3.1415926535897932
#define MATH_TAU 6.283185307179586

// Make room for our constexpr's below by overriding potential system-specific macros.
#undef ABS
#undef SIGN
#undef MIN
#undef MAX
#undef CLAMP

// Generic ABS function, for math uses please use Math::abs.
template <typename T>
constexpr T ABS(T m_v) {
	return m_v < 0 ? -m_v : m_v;
}

template <typename T>
constexpr const T SIGN(const T m_v) {
	return m_v == 0 ? 0.0f : (m_v < 0 ? -1.0f : +1.0f);
}

template <typename T, typename T2>
constexpr auto MIN(const T m_a, const T2 m_b) {
	return m_a < m_b ? m_a : m_b;
}

template <typename T, typename T2>
constexpr auto MAX(const T m_a, const T2 m_b) {
	return m_a > m_b ? m_a : m_b;
}

template <typename T, typename T2, typename T3>
constexpr auto CLAMP(const T m_a, const T2 m_min, const T3 m_max) {
	return m_a < m_min ? m_min : (m_a > m_max ? m_max : m_a);
}

//// ##### 

template <typename T>
inline T min(T a, T b) {
	return a < b ? a : b;
}

template <typename T>
inline T max(T a, T b) {
	return a > b ? a : b;
}

template <typename T>
inline T sign(T x) {
	return static_cast<T>(SIGN(x));
}

template <typename T>
inline T abs(T x) {
	return std::abs(x);
}

template <typename T>
inline T pow(T x, T e) {
	return std::pow(x, e);
}

template <typename T>
inline T exp2(T x) {
	return std::exp2(x);
}

template <typename T>
inline T log2(T x) {
	return std::log2(x);
}

template <typename T>
inline T log10(T x) {
	return std::log10(x);
}

template <typename T>
inline T logb(T base, T x) {
	return std::log2(x) / std::log2(base);
}


template <typename T>
inline T sqrt(T x) {
	return std::sqrt(x);
}

template <typename T>
inline T lerp(T min_v, T max_v, float w) {
    return min_v + w * (max_v - min_v);
}

inline float inverse_lerp(float min_v, float max_v, float v) {
    return (v - min_v) / (max_v - min_v);
}

// linearly interpelates between the shortest path between 2 angles. Does NOT gaurentee th output be within [-pi, pi]
inline float lerp_angle(float min_v, float max_v, float w) {
	if ((max_v - min_v) > MATH_PI) { min_v += MATH_TAU; }
	else if ((min_v - max_v) > MATH_PI) { max_v += MATH_TAU; }

	return lerp(min_v, max_v, w);
}

template <typename T>
inline T modf(T x, T *iptr) {
	return std::modf(x, iptr);
}

inline int posmod(int x, int y) {
    int v = x % y;
    if (v < 0) {
        v += y;
    }
    return v;
}

inline float fposmod(float x, float y) {
	float v = std::fmod(x, y);
    if (v * y < 0.0f) {
        v += y;
    }
    return v;
}

inline bool is_pow_2(uint32_t x) {
	return x != 0 && ((x - 1) & x) == 0;
}

inline uint32_t last_pow_2(uint32_t x) {
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x - (x >> 1);
}

inline uint32_t next_pow_2(uint32_t x) {
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return (x << 1) & ~x;
}



};

#endif
