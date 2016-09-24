#ifndef RIFF_STL_H
#define RIFF_STL_H

#ifdef DISABLE_STL

	#include "cbir/pair.h"
	#include "cbir/vector.h"
	#include "cbir/algorithm.h"
	#include "cbir/numeric_limits.h"

	using namespace riff;
	namespace ns = riff;
#else
	#include <utility>
	#include <vector>
	#include <algorithm>
	#include <limits>

	using namespace std;
	namespace ns = std;
#endif

#endif
