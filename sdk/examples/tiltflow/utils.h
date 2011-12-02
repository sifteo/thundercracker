#ifndef _UTILS_H
#define _UTILS_H

#define PRINTS_ON 0
#if PRINTS_ON
#define PRINT(...) printf (__VA_ARGS__)
#else
#define PRINT(...) 
#endif

typedef enum
{
	UP,
	LEFT, 
	DOWN,
	RIGHT
} Side;

#endif
