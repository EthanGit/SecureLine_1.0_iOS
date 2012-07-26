/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_VERSION_H__
#define __AM_VERSION_H__

#ifdef WIN32

#ifdef AMSIP_EXPORTS
#define PPL_DECLARE(type) __declspec(dllexport) type __stdcall
#else
#define PPL_DECLARE(type) __declspec(dllimport) type __stdcall
#endif

#else
#define PPL_DECLARE(type) type
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file am_version.h
 * @brief amsip version definition file
 *
 */

/**
 * @defgroup amsip_version version definition interface
 * @ingroup amsip_internal
 * @{
 */

#define AM_VERSION "4.7.0"
#define AM_TIMESTAMP __DATE__"/"__TIME__

/** @} */

#ifdef __cplusplus
}
#endif
#endif
