#ifndef RIFF_ALGORITHM_H
#define RIFF_ALGORITHM_H

#include "cbir/vector.h"
#include "cbir/pair.h"

#include <stdlib.h>

namespace riff {

int compare(const void* ptr1, const void* ptr2) {
	return 0;	// not implemented
}

template <class T>
void sort(T *begin, T* end) {
	qsort(begin, end-begin, sizeof(T*), compare);
}

template <class T>
T* max_element(T* start, T* end) {
	if (start == end) return start;

	T max_elem = *start;
	T* max_elem_iter = start;

	for (T* iter = start; iter != end; ++iter) {
		if (*iter > max_elem) {
			max_elem = *iter;
			max_elem_iter = iter;
		}
	}
	return max_elem_iter;
}

template <class T>
T* min_element(T* start, T* end) {
	if (start == end) return start;

	T min_elem = *start;
	T* min_elem_iter = start;

	for (T* iter = start; iter != end; ++iter) {
		if (*iter < min_elem) {
			min_elem = *iter;
			min_elem_iter = iter;
		}
	}
	return min_elem_iter;
}

template <class T>
T* find(T* start, T* end, T elem) {
	T* iter;
	for (iter = start; iter != end; ++iter) {
		if (*iter == elem) return iter;
	}
	return end;
}

}
#endif

