#ifndef QSORT_H
#define QSORT_H

#if defined (__cplusplus)
extern "C" {
#endif

void qsort(
	void *a,
	unsigned n,
	unsigned es,
	int (*cmp)(const void*, const void*));

#if defined (__cplusplus)
}
#endif

#endif // QSORT_H
