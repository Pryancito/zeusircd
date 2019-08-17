/*--------------------------------------------------------------------------
 DPRINT(): a debug print macro.
 Compile with -DUSE_DPRINT to enable debug prints.

 Uses a cheesy trick to get the preprocessor to understand a
 macro that takes a variable number of arguments:
 have the caller enclose the args with an extra set of parentheses.

 Call it just like printf() whenever you have something interesting to
 print out, but use an extra pair of parentheses, 
 e.g.
   DPRINT(("line %d file %s\n", __LINE__, __FILE__));
 or
   DPRINT(("Sphere::Pulsating::animate(): setting speed to %d hz\n", speed));

 You can call it liberally without worrying about affecting your program's
 speed or cluttering its output, because compiling without -DUSE_DPRINT
 will turn them all off.

 Using EDPRINT just like DPRINT, but only for important errors.
 It prints to stdout regardless of any runtime settings.
--------------------------------------------------------------------------*/

#ifndef dprint_h
#define dprint_h

#include <stdio.h>

#ifdef USE_DPRINT

#ifdef __cplusplus
extern "C" {
#endif

extern int g_dprint_enabled;
extern FILE *g_dprint_fp;
void dprint_print(const char *format, ...);

/* Call DPRINT_ENABLE(bool) to turn DPRINTing on or off */
#define DPRINT_ENABLE(b) g_dprint_enabled = (b)

/* Call DPRINT_SETFP(fp) to send DPRINTs to a file */
#define DPRINT_SETFP(fp) g_dprint_fp = (fp)

/* Call DPRINT(("format string...", arg, arg, ...)) to log an event */
/* Note the double parentheses! */
#define DPRINT(s) do { \
	if (g_dprint_enabled) { \
		dprint_print s; \
	} \
} while (0)

/* Call EDPRINT(("format string...", arg, arg, ...)) to log an error */
/* Note the double parentheses! */
#define EDPRINT(s) do { \
	if (g_dprint_enabled) { \
		dprint_print s; \
	} \
	printf s; \
} while (0)

#ifdef __cplusplus
}
#endif

#else
#define DPRINT_ENABLE(b)
#define DPRINT_SETFP(fp)
#define DPRINT(s)
/* Call EDPRINT(("format string...", arg, arg, ...)) to log an error */
/* Note the double parentheses! */
#define EDPRINT(s) do { printf s; } while (0)
#endif

#define LOG_ERROR(s) DPRINT(s)
#define LOG_TRACE(s) DPRINT(s)
#endif
