/*
 * utils.h
 *
 *  Created on: 30 May 2015
 *      Author: supermaja
 */

#ifndef UTILS_H_
#define UTILS_H_


#ifdef UNIT_TESTING

/* All functions in this object need to be exposed to the test application,
 * so redefine static to nothing. */
#define static

extern int example_test_fprintf(FILE * const file, const char *format, ...);
extern wint_t example_test_fgetwc(FILE *stream);

/* Redirect fprintf to a function in the test application so it's possible to
 * test error messages. */
#ifdef fprintf
#undef fprintf
#endif /* fprintf */
#define fprintf example_test_fprintf

#ifdef fgetwc
#undef fgetwc
#endif
#define fgetwc example_test_fgetwc

#endif /* UNIT_TESTING */


#endif /* UTILS_H_ */
