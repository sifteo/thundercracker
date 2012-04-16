#ifndef BSEARCH_H
#define BSEARCH_H

#if defined (__cplusplus)
extern "C" {
#endif

void * bsearch(
	const void* key,
	const void* base,
	unsigned nmemb,
	unsigned size,
	int compar(const void*, const void*));

#if defined (__cplusplus)
}
#endif

#endif // BSEARCH_H
