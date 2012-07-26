/*
 * SpanDSP - a series of DSP components for telephony
 *
 * tone_detect.h - General telephony tone detection.
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2001, 2005 Steve Underwood
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: tone_detect.h,v 1.1 2008-07-09 18:03:10 jack Exp $
 */

#if !defined(_SPANDSP_TONE_DETECT_H_)
#define _SPANDSP_TONE_DETECT_H_

/*!
    Goertzel filter descriptor.
*/
typedef struct
{
#if defined(SPANDSP_USE_FIXED_POINT)
    int16_t fac;
#else
    float fac;
#endif
    int samples;
} goertzel_descriptor_t;

/*!
    Goertzel filter state descriptor.
*/
typedef struct
{
#if defined(SPANDSP_USE_FIXED_POINT)
    int16_t v2;
    int16_t v3;
    int16_t fac;
#else
    float v2;
    float v3;
    float fac;
#endif
    int samples;
    int current_sample;
} goertzel_state_t;

#if defined(__cplusplus)
extern "C"
{
#endif

/*! \brief Create a descriptor for use with either a Goertzel transform */
void make_goertzel_descriptor(goertzel_descriptor_t *t,
                              float freq,
                              int samples);

/*! \brief Initialise the state of a Goertzel transform.
    \param s The Goertzel context. If NULL, a context is allocated with malloc.
    \param t The Goertzel descriptor.
    \return A pointer to the Goertzel state. */
goertzel_state_t *goertzel_init(goertzel_state_t *s,
                                goertzel_descriptor_t *t);

/*! \brief Reset the state of a Goertzel transform.
    \param s The Goertzel context. */
void goertzel_reset(goertzel_state_t *s);

/*! \brief Update the state of a Goertzel transform.
    \param s The Goertzel context.
    \param amp The samples to be transformed.
    \param samples The number of samples.
    \return The number of samples unprocessed */
int goertzel_update(goertzel_state_t *s,
                    const int16_t amp[],
                    int samples);

/*! \brief Evaluate the final result of a Goertzel transform.
    \param s The Goertzel context.
    \return The result of the transform. The expected result for a pure sine wave
            signal of level x dBm0, at the very centre of the bin is:
    [Floating point] ((samples_per_goertzel_block*32768.0/1.4142)*10^((x - DBM0_MAX_SINE_POWER)/20.0))^2
    [Fixed point] ((samples_per_goertzel_block*256.0/1.4142)*10^((x - DBM0_MAX_SINE_POWER)/20.0))^2 */
#if defined(SPANDSP_USE_FIXED_POINT)
int32_t goertzel_result(goertzel_state_t *s);
#else
float goertzel_result(goertzel_state_t *s);
#endif

/*! \brief Update the state of a Goertzel transform.
    \param s The Goertzel context.
    \param amp The sample to be transformed. */
static __inline__ void goertzel_sample(goertzel_state_t *s, int16_t amp)
{
#if defined(SPANDSP_USE_FIXED_POINT)
    int16_t x;
    int16_t v1;
#else
    float v1;
#endif

    v1 = s->v2;
    s->v2 = s->v3;
#if defined(SPANDSP_USE_FIXED_POINT)
    x = (((int32_t) s->fac*s->v2) >> 14);
    /* Scale down the input signal to avoid overflows. 9 bits is enough to
       monitor the signals of interest with adequate dynamic range and
       resolution. In telephony we generally only start with 13 or 14 bits,
       anyway. */
    s->v3 = x - v1 + (amp >> 7);
#else
    s->v3 = s->fac*s->v2 - v1 + amp;
#endif
    s->current_sample++;
}
/*- End of function --------------------------------------------------------*/

/* Scale down the input signal to avoid overflows. 9 bits is enough to
   monitor the signals of interest with adequate dynamic range and
   resolution. In telephony we generally only start with 13 or 14 bits,
   anyway. This is sufficient for the longest Goertzel we currently use. */
#if defined(SPANDSP_USE_FIXED_POINT)
#define goertzel_preadjust_amp(amp) (((int16_t) amp) >> 7)
#else
#define goertzel_preadjust_amp(amp) ((float) amp)
#endif

/* Minimal update the state of a Goertzel transform. This is similar to
   goertzel_sample, but more suited to blocks of Goertzels. It assumes
   the amplitude is pre-shifted, and does not update the per-state sample
   count.
    \brief Update the state of a Goertzel transform.
    \param s The Goertzel context.
    \param amp The adjusted sample to be transformed. */
#if defined(SPANDSP_USE_FIXED_POINT)
static __inline__ void goertzel_samplex(goertzel_state_t *s, int16_t amp)
#else
static __inline__ void goertzel_samplex(goertzel_state_t *s, float amp)
#endif
{
#if defined(SPANDSP_USE_FIXED_POINT)
    int16_t x;
    int16_t v1;
#else
    float v1;
#endif

    v1 = s->v2;
    s->v2 = s->v3;
#if defined(SPANDSP_USE_FIXED_POINT)
    x = (((int32_t) s->fac*s->v2) >> 14);
    s->v3 = x - v1 + amp;
#else
    s->v3 = s->fac*s->v2 - v1 + amp;
#endif
}
/*- End of function --------------------------------------------------------*/

#if defined(__cplusplus)
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
