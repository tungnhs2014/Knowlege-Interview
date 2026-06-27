#ifndef CH01_MATH_UTILS_HPP
#define CH01_MATH_UTILS_HPP

inline int clamp_to_zero(int value) {
    return value < 0 ? 0 : value;
}

#endif
