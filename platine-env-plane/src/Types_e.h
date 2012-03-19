#ifndef Types_e
#   define Types_e

/*  Cartouche AD  */

typedef signed char T_INT8;
typedef signed short T_INT16;
typedef signed long T_INT32;

/* Define T_INT32 maximum and minimum values */
#   define C_INT32_MAX_VALUE	0x7FFFFFFF
#   define C_INT32_MIN_VALUE	0x80000000

typedef signed long long T_INT64;

typedef unsigned char T_UINT8;
typedef unsigned short T_UINT16;
typedef unsigned long T_UINT32;

/* Define T_UINT32 maximum and minimum values */
#   define C_UINT32_MAX_VALUE	0xFFFFFFFF
#   define C_UINT32_MIN_VALUE	0

typedef unsigned long long T_UINT64;

typedef float T_FLOAT;
typedef double T_DOUBLE;
/* Define T_DOUBLE anf T_FLOAT maximum value */
#   define C_DOUBLE_MAX_VALUE 1.7E308
#   define C_FLOAT_MAX_VALUE 3.4E38

#   ifndef TRUE
//enum {FALSE = 0, TRUE = 1 };
#      define FALSE 0
#      define TRUE  1
#   endif
typedef T_UINT8 T_BOOL;

typedef unsigned char T_BYTE;
typedef T_BYTE *T_BUFFER;

typedef char T_CHAR;
typedef T_CHAR *T_STRING;

#   define SPRINT_MAX_LEN        512

/* just used for the compilation */
#   ifndef __cplusplus
extern long int lrint(double);
extern long long int llrint(double x);
#   endif

#endif
