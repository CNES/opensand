/**
 * @file unused.h
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief Unused macro
 */
#ifndef UNUSED_H
#define UNUSED_H

/** unused macro to avoid compilation warning with unused parameters */
#ifdef __GNUC__
#  define UNUSED(x) x __attribute__((unused))
#elif __LCLINT__
#  define UNUSED(x) /*@unused@*/ x
#else               /* !__GNUC__ && !__LCLINT__ */
#  define UNUSED(x) x
#endif              /* !__GNUC__ && !__LCLINT__ */


#endif

