#ifndef NUMERIC_LIMITS_H
#define NUMERIC_LIMITS_H

#include <limits.h>
#include <float.h>

namespace riff {

template <class T>
class numeric_limits {
public:
	static T min() { return 0; }
	static T max() { return 255; }

};

template <>
class numeric_limits<unsigned char> {
public:
	static unsigned char min() { return 0; }
	static unsigned char max() { return 255; }
};

template <>
class numeric_limits<char> {
public:
	static char min() { return -128; }
	static char max() { return 127; }
};

template <>
class numeric_limits<unsigned int> {
public:
	static unsigned int min() { return 0; }
	static unsigned int max() { return UINT_MAX; }
};

template <>
class numeric_limits<int> {
public:
	static int min() { return INT_MIN; } 
	static int max() { return INT_MAX; }
};

template <>
class numeric_limits<float> {
public:
	static float min() { return FLT_MIN; }
	static float max() { return FLT_MAX; }
};

}	// namespace

#endif
