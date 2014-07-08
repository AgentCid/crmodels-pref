#ifndef CRMODELS_COMPILE_FLAGS_H
#define CRMODELS_COMPILE_FLAGS_H

/* [marcy 071706]
 *
 * Define to remove all debugging messages and experimental code.
 */
#ifndef DEVEL_VERSION
#  define RELEASE 1
#endif


/* verbose flags */
#define PRINT_TIMESTAMPS 1
#define DEBUG_BINARY_SEARCH 1
/**/

/* [marcy 041906] */
//#define RELEVANT_CRRULES 1	
#define CONFLICT_STRING "CONFLICT_ON:"

/* [marcy 051606] */
#define USE_BINARYSEARCH_STARTUP 1

/* [marcy 071206] */
#define USE_CONTINUOUS_BINARYSEARCH 1

/* Enable experimental code for set preferences */
#define DO_SETPREFS 1


/* [marcy 071706]
 *
 * Define to remove all debugging messages and experimental code.
 */
#ifdef RELEASE

/* experimental code */
#  undef USE_CONTINUOUS_BINARYSEARCH
#  undef DO_SETPREFS

/* debugging messages */
#  undef PRINT_TIMESTAMPS
#  undef DEBUG_BINARY_SEARCH
#endif

#endif /* CRMODELS_COMPILE_FLAGS_H */
