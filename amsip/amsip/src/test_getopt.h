/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2011  Aymeric MOIZARD - <amoizard@gmail.com>
*/

/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#ifndef _TEST_GETOPT_H_
#define _TEST_GETOPT_H_

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/**
 * @file test_getopt.h
 * @brief TEST Command Arguments (getopt)
 */
/**
 * @defgroup TEST_getopt Command Argument Parsing
 * @ingroup TEST
 * @{
 */

#define GETOPT_SUCCESS   0
#define GETOPT_EOF       -1
#define GETOPT_BADCH     -2
#define GETOPT_BADARG    -3
	
  typedef void (test_getopt_err_fn_t) (void *arg, const char *err, ...);

  typedef struct test_getopt_t test_getopt_t;
/**
 * Structure to store command line argument information.
 */
  struct test_getopt_t
  {
    /** function to print error message (NULL == no messages) */
    test_getopt_err_fn_t *errfn;
    /** user defined first arg to pass to error message  */
    void *errarg;
    /** index into parent argv vector */
    int ind;
    /** character checked for validity */
    int opt;
    /** reset getopt */
    int reset;
    /** count of arguments */
    int argc;
    /** array of pointers to arguments */
    const char **argv;
    /** argument associated with option */
    char const *place;
    /** set to nonzero to support interleaving options with regular args */
    int interleave;
    /** start of non-option arguments skipped for interleaving */
    int skip_start;
    /** end of non-option arguments skipped for interleaving */
    int skip_end;
  };

  typedef struct test_getopt_option_t test_getopt_option_t;

/**
 * Structure used to describe options that getopt should search for.
 */
  struct test_getopt_option_t
  {
    /** long option name, or NULL if option has no long name */
    const char *name;
    /** option letter, or a value greater than 255 if option has no letter */
    int optch;
    /** nonzero if option takes an argument */
    int has_arg;
    /** a description of the option */
    const char *description;
  };

/** Filename_of_pathname returns the final element of the pathname.
 * Using the current platform's filename syntax.
 *   "/foo/bar/gum" -> "gum"
 *   "/foo/bar/gum/" -> ""
 *   "gum" -> "gum"
 *   "wi\\n32\\stuff" -> "stuff
 *
 * Corrected Win32 to accept "a/b\\stuff", "a:stuff"
 */
  const char * test_filename_of_pathname (const char *pathname);

/**
 * Initialize the arguments for parsing by test_getopt().
 * @param os   The options structure created for test_getopt()
 * @param cont The pool to operate on
 * @param argc The number of arguments to parse
 * @param argv The array of arguments to parse
 * @remark Arguments 2 and 3 are most commonly argc and argv from main(argc, argv)
 * The errfn is initialized to fprintf(stderr... but may be overridden.
 */
  int test_getopt_init (test_getopt_t ** os,
                                              int argc, const char *const *argv);

/**
 * Parse the options initialized by test_getopt_init().
 * @param os     The test_opt_t structure returned by test_getopt_init()
 * @param opts   A string of characters that are acceptable options to the 
 *               program.  Characters followed by ":" are required to have an 
 *               option associated
 * @param option_ch  The next option character parsed
 * @param option_arg The argument following the option character:
 * @return There are four potential status values on exit. They are:
 * <PRE>
 *             GETOPT_EOF      --  No more options to parse
 *             GETOPT_BADCH    --  Found a bad option character
 *             GETOPT_BADARG   --  No argument followed @parameter:
 *             GETOPT_SUCCESS  --  The next option was found.
 * </PRE>
 */
  int test_getopt (test_getopt_t * os,
                                         const char *opts, char *option_ch,
                                         const char **option_arg);

/**
 * Parse the options initialized by test_getopt_init(), accepting long
 * options beginning with "--" in addition to single-character
 * options beginning with "-".
 * @param os     The test_getopt_t structure created by test_getopt_init()
 * @param opts   A pointer to a list of test_getopt_option_t structures, which
 *               can be initialized with { "name", optch, has_args }.  has_args
 *               is nonzero if the option requires an argument.  A structure
 *               with an optch value of 0 terminates the list.
 * @param option_ch  Receives the value of "optch" from the test_getopt_option_t
 *                   structure corresponding to the next option matched.
 * @param option_arg Receives the argument following the option, if any.
 * @return There are four potential status values on exit.   They are:
 * <PRE>
 *             GETOPT_EOF      --  No more options to parse
 *             GETOPT_BADCH    --  Found a bad option character
 *             GETOPT_BADARG   --  No argument followed @parameter:
 *             GETOPT_SUCCESS  --  The next option was found.
 * </PRE>
 * When GETOPT_SUCCESS is returned, os->ind gives the index of the first
 * non-option argument.  On error, a message will be printed to stdout unless
 * os->err is set to 0.  If os->interleave is set to nonzero, options can come
 * after arguments, and os->argv will be permuted to leave non-option arguments
 * at the end (the original argv is unaffected).
 */
  int test_getopt_long (test_getopt_t * os,
                                              const test_getopt_option_t *
                                              opts, int *option_ch,
                                              const char **option_arg);
/** @} */

#ifdef __cplusplus
}
#endif

#endif                          /* ! TEST_GETOPT_H */
