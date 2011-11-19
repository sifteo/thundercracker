#ifndef _UTILS_H
#define _UTILS_H

#define PRINTS_ON 1
#if PRINTS_ON
#define PRINT(...) printf (__VA_ARGS__)
#else
#define PRINT(...) 
#endif

enum
{
	UP,
	LEFT, 
	DOWN,
	RIGHT
};

#endif