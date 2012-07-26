/*
 * SpanDSP - a series of DSP components for telephony
 *
 * dtmf.c - DTMF generation and detection.
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2001-2003, 2005, 2006 Steve Underwood
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
 * $Id: dtmf.c,v 1.2 2008-07-09 18:07:38 jack Exp $
 */
 
/*! \file dtmf.h */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef _MSC_VER
typedef unsigned __int8		uint8_t;
typedef unsigned __int16	uint16_t;
typedef unsigned __int32	uint32_t;
typedef unsigned __int64    uint64_t;
typedef __int8		int8_t;
typedef __int16		int16_t;
typedef __int32		int32_t;
typedef __int64		int64_t;
#define inline __inline
#define __inline__ __inline
#else
#include <inttypes.h>
#endif

#include <stdlib.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>

//#include "spandsp/telephony.h"
//#include "spandsp/queue.h"
//#include "spandsp/complex.h"
//#include "spandsp/dds.h"
#include "tone_detect.h"
//#include "spandsp/tone_generate.h"
//#include "spandsp/super_tone_rx.h"
#include "dtmf.h"

#define DBM0_MAX_POWER          (3.14f + 3.02f)
#define DBM0_MAX_SINE_POWER     (3.14f)

#if !defined(M_PI)
/* C99 systems may not define M_PI */
#define M_PI 3.14159265358979323846264338327
#endif

#define SAMPLE_RATE             8000

#ifdef _MSC_VER
static inline int rintf(float a)
{
    int i;
    
    __asm
    {
        fld   a
        fistp i
    }
    return i;
}
#endif

void make_goertzel_descriptor(goertzel_descriptor_t *t, float freq, int samples)
{
#if defined(SPANDSP_USE_FIXED_POINT)
    t->fac = 16383.0f*2.0f*cosf(2.0f*M_PI*(freq/(float) SAMPLE_RATE));
#else
    t->fac = 2.0f*cosf(2.0f*M_PI*(freq/(float) SAMPLE_RATE));
#endif
    t->samples = samples;
}
/*- End of function --------------------------------------------------------*/

goertzel_state_t *goertzel_init(goertzel_state_t *s,
                                goertzel_descriptor_t *t)
{
    if (s == NULL)
    {
        if ((s = (goertzel_state_t *) malloc(sizeof(*s))) == NULL)
            return NULL;
    }
#if defined(SPANDSP_USE_FIXED_POINT)
    s->v2 =
    s->v3 = 0;
#else
    s->v2 =
    s->v3 = 0.0f;
#endif
    s->fac = t->fac;
    s->samples = t->samples;
    s->current_sample = 0;
    return s;
}
/*- End of function --------------------------------------------------------*/

void goertzel_reset(goertzel_state_t *s)
{
#if defined(SPANDSP_USE_FIXED_POINT)
    s->v2 =
    s->v3 = 0;
#else
    s->v2 =
    s->v3 = 0.0f;
#endif
    s->current_sample = 0;
}
/*- End of function --------------------------------------------------------*/

int goertzel_update(goertzel_state_t *s,
                    const int16_t amp[],
                    int samples)
{
    int i;
#if defined(SPANDSP_USE_FIXED_POINT)
    int16_t x;
    int16_t v1;
#else
    float v1;
#endif

    if (samples > s->samples - s->current_sample)
        samples = s->samples - s->current_sample;
    for (i = 0;  i < samples;  i++)
    {
        v1 = s->v2;
        s->v2 = s->v3;
#if defined(SPANDSP_USE_FIXED_POINT)
        x = (((int32_t) s->fac*s->v2) >> 14);
        /* Scale down the input signal to avoid overflows. 9 bits is enough to
           monitor the signals of interest with adequate dynamic range and
           resolution. In telephony we generally only start with 13 or 14 bits,
           anyway. */
        s->v3 = x - v1 + (amp[i] >> 7);
#else
        s->v3 = s->fac*s->v2 - v1 + amp[i];
#endif
    }
    s->current_sample += samples;
    return samples;
}
/*- End of function --------------------------------------------------------*/

#if defined(SPANDSP_USE_FIXED_POINT)
int32_t goertzel_result(goertzel_state_t *s)
#else
float goertzel_result(goertzel_state_t *s)
#endif
{
#if defined(SPANDSP_USE_FIXED_POINT)
    int16_t v1;
    int32_t x;
    int32_t y;
#else
    float v1;
#endif

    /* Push a zero through the process to finish things off. */
    v1 = s->v2;
    s->v2 = s->v3;
#if defined(SPANDSP_USE_FIXED_POINT)
    x = (((int32_t) s->fac*s->v2) >> 14);
    s->v3 = x - v1;
#else
    s->v3 = s->fac*s->v2 - v1;
#endif
    /* Now calculate the non-recursive side of the filter. */
    /* The result here is not scaled down to allow for the magnification
       effect of the filter (the usual DFT magnification effect). */
#if defined(SPANDSP_USE_FIXED_POINT)
    x = (int32_t) s->v3*s->v3;
    y = (int32_t) s->v2*s->v2;
    x += y;
    y = ((int32_t) s->v3*s->fac) >> 14;
    y *= s->v2;
    x -= y;
    x <<= 1;
    goertzel_reset(s);
    /* The number returned in a floating point build will be 16384^2 times
       as big as for a fixed point build, due to the 14 bit shifts
       (or the square of the 7 bit shifts, depending how you look at it). */
    return x;
#else
    v1 = s->v3*s->v3 + s->v2*s->v2 - s->v2*s->v3*s->fac;
    v1 *= 2.0;
    goertzel_reset(s);
    return v1;
#endif
}

#if defined(SPANDSP_USE_FIXED_POINT)
#define DTMF_THRESHOLD              10438           /* -42dBm0 */
#define DTMF_NORMAL_TWIST           6.309f          /* 8dB */
#define DTMF_REVERSE_TWIST          2.512f          /* 4dB */
#define DTMF_RELATIVE_PEAK_ROW      6.309f          /* 8dB */
#define DTMF_RELATIVE_PEAK_COL      6.309f          /* 8dB */
#define DTMF_TO_TOTAL_ENERGY        83.868f         /* -0.85dB */
#define DTMF_POWER_OFFSET           68.251f         /* 10*log(256.0*256.0*DTMF_SAMPLES_PER_BLOCK) */
#define DTMF_SAMPLES_PER_BLOCK      102
#else
#define DTMF_THRESHOLD              171032462.0f    /* -42dBm0 [((DTMF_SAMPLES_PER_BLOCK*32768.0/1.4142)*10^((-42 - DBM0_MAX_SINE_POWER)/20.0))^2 => 171032462.0] */
#define DTMF_NORMAL_TWIST           6.309f          /* 8dB [10^(8/10) => 6.309] */
#define DTMF_REVERSE_TWIST          2.512f          /* 4dB */
#define DTMF_RELATIVE_PEAK_ROW      6.309f          /* 8dB */
#define DTMF_RELATIVE_PEAK_COL      6.309f          /* 8dB */
#define DTMF_TO_TOTAL_ENERGY        83.868f         /* -0.85dB [DTMF_SAMPLES_PER_BLOCK*10^(-0.85/10.0)] */
#define DTMF_POWER_OFFSET           110.395f        /* 10*log(32768.0*32768.0*DTMF_SAMPLES_PER_BLOCK) */
#define DTMF_SAMPLES_PER_BLOCK      102
#endif

static const float dtmf_row[] =
{
     697.0f,  770.0f,  852.0f,  941.0f
};
static const float dtmf_col[] =
{
    1209.0f, 1336.0f, 1477.0f, 1633.0f
};

static const char dtmf_positions[] = "123A" "456B" "789C" "*0#D";

static goertzel_descriptor_t dtmf_detect_row[4];
static goertzel_descriptor_t dtmf_detect_col[4];

int dtmf_rx(dtmf_rx_state_t *s, const int16_t amp[], int samples)
{
#if defined(SPANDSP_USE_FIXED_POINT)
    int32_t row_energy[4];
    int32_t col_energy[4];
    int16_t xamp;
    float famp;
#else
    float row_energy[4];
    float col_energy[4];
    float xamp;
    float famp;
#endif
    float v1;
    int i;
    int j;
    int sample;
    int best_row;
    int best_col;
    int limit;
    uint8_t hit;

    hit = 0;
    for (sample = 0;  sample < samples;  sample = limit)
    {
        /* The block length is optimised to meet the DTMF specs. */
        if ((samples - sample) >= (DTMF_SAMPLES_PER_BLOCK - s->current_sample))
            limit = sample + (DTMF_SAMPLES_PER_BLOCK - s->current_sample);
        else
            limit = samples;
        /* The following unrolled loop takes only 35% (rough estimate) of the 
           time of a rolled loop on the machine on which it was developed */
        for (j = sample;  j < limit;  j++)
        {
            xamp = amp[j];
            if (s->filter_dialtone)
            {
                famp = xamp;
                /* Sharp notches applied at 350Hz and 440Hz - the two common dialtone frequencies.
                   These are rather high Q, to achieve the required narrowness, without using lots of
                   sections. */
                v1 = 0.98356f*famp + 1.8954426f*s->z350[0] - 0.9691396f*s->z350[1];
                famp = v1 - 1.9251480f*s->z350[0] + s->z350[1];
                s->z350[1] = s->z350[0];
                s->z350[0] = v1;

                v1 = 0.98456f*famp + 1.8529543f*s->z440[0] - 0.9691396f*s->z440[1];
                famp = v1 - 1.8819938f*s->z440[0] + s->z440[1];
                s->z440[1] = s->z440[0];
                s->z440[0] = v1;
                xamp = famp;
            }
            xamp = goertzel_preadjust_amp(xamp);
#if defined(SPANDSP_USE_FIXED_POINT)
            s->energy += ((int32_t) xamp*xamp);
#else
            s->energy += xamp*xamp;
#endif
            goertzel_samplex(&s->row_out[0], xamp);
            goertzel_samplex(&s->col_out[0], xamp);
            goertzel_samplex(&s->row_out[1], xamp);
            goertzel_samplex(&s->col_out[1], xamp);
            goertzel_samplex(&s->row_out[2], xamp);
            goertzel_samplex(&s->col_out[2], xamp);
            goertzel_samplex(&s->row_out[3], xamp);
            goertzel_samplex(&s->col_out[3], xamp);
        }
        s->current_sample += (limit - sample);
        if (s->current_sample < DTMF_SAMPLES_PER_BLOCK)
            continue;

        /* We are at the end of a DTMF detection block */
        /* Find the peak row and the peak column */
        row_energy[0] = goertzel_result(&s->row_out[0]);
        best_row = 0;
        col_energy[0] = goertzel_result(&s->col_out[0]);
        best_col = 0;
        for (i = 1;  i < 4;  i++)
        {
            row_energy[i] = goertzel_result(&s->row_out[i]);
            if (row_energy[i] > row_energy[best_row])
                best_row = i;
            col_energy[i] = goertzel_result(&s->col_out[i]);
            if (col_energy[i] > col_energy[best_col])
                best_col = i;
        }
        hit = 0;
        /* Basic signal level test and the twist test */
        if (row_energy[best_row] >= s->threshold
            &&
            col_energy[best_col] >= s->threshold
            &&
            col_energy[best_col] < row_energy[best_row]*s->reverse_twist
            &&
            col_energy[best_col]*s->normal_twist > row_energy[best_row])
        {
            /* Relative peak test ... */
            for (i = 0;  i < 4;  i++)
            {
                if ((i != best_col  &&  col_energy[i]*DTMF_RELATIVE_PEAK_COL > col_energy[best_col])
                    ||
                    (i != best_row  &&  row_energy[i]*DTMF_RELATIVE_PEAK_ROW > row_energy[best_row]))
                {
                    break;
                }
            }
            /* ... and fraction of total energy test */
            if (i >= 4
                &&
                (row_energy[best_row] + col_energy[best_col]) > DTMF_TO_TOTAL_ENERGY*s->energy)
            {
                /* Got a hit */
                hit = dtmf_positions[(best_row << 2) + best_col];
            }
        }
        /* The logic in the next test should ensure the following for different successive hit patterns:
                -----ABB = start of digit B.
                ----B-BB = start of digit B
                ----A-BB = start of digit B
                BBBBBABB = still in digit B.
                BBBBBB-- = end of digit B
                BBBBBBC- = end of digit B
                BBBBACBB = B ends, then B starts again.
                BBBBBBCC = B ends, then C starts.
                BBBBBCDD = B ends, then D starts.
           This can work with:
                - Back to back differing digits. Back-to-back digits should
                  not happen. The spec. says there should be a gap between digits.
                  However, many real phones do not impose a gap, and rolling across
                  the keypad can produce little or no gap.
                - It tolerates nasty phones that give a very wobbly start to a digit.
                - VoIP can give sample slips. The phase jumps that produces will cause
                  the block it is in to give no detection. This logic will ride over a
                  single missed block, and not falsely declare a second digit. If the
                  hiccup happens in the wrong place on a minimum length digit, however
                  we would still fail to detect that digit. Could anything be done to
                  deal with that? Packet loss is clearly a no-go zone.
                  Note this is only relevant to VoIP using A-law, u-law or similar.
                  Low bit rate codecs scramble DTMF too much for it to be recognised,
                  and often slip in units larger than a sample. */
        if (hit != s->in_digit)
        {
            if (s->last_hit != s->in_digit)
            {
                /* We have two successive indications that something has changed. */
                /* To declare digit on, the hits must agree. Otherwise we declare tone off. */
                hit = (hit  &&  hit == s->last_hit)  ?  hit   :  0;
                if (s->realtime_callback)
                {
                    /* Avoid reporting multiple no digit conditions on flaky hits */
                    if (s->in_digit  ||  hit)
                    {
                        i = (s->in_digit  &&  !hit)  ?  -99  :  rintf(log10f(s->energy)*10.0f - DTMF_POWER_OFFSET + DBM0_MAX_POWER);
                        s->realtime_callback(s->realtime_callback_data, hit, i, 0);
                    }
                }
                else
                {
                    if (hit)
                    {
                        if (s->current_digits < MAX_DTMF_DIGITS)
                        {
                            s->digits[s->current_digits++] = (char) hit;
                            s->digits[s->current_digits] = '\0';
                            if (s->digits_callback)
                            {
                                s->digits_callback(s->digits_callback_data, s->digits, s->current_digits);
                                s->current_digits = 0;
                            }
                        }
                        else
                        {
                            s->lost_digits++;
                        }
                    }
                }
                s->in_digit = hit;
            }
        }
        s->last_hit = hit;
#if defined(SPANDSP_USE_FIXED_POINT)
        s->energy = 0;
#else
        s->energy = 0.0f;
#endif
        s->current_sample = 0;
    }
    if (s->current_digits  &&  s->digits_callback)
    {
        s->digits_callback(s->digits_callback_data, s->digits, s->current_digits);
        s->digits[0] = '\0';
        s->current_digits = 0;
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/

int dtmf_rx_status(dtmf_rx_state_t *s)
{
    if (s->in_digit)
        return s->in_digit;
    if (s->last_hit)
        return 'x';
    return 0;
}
/*- End of function --------------------------------------------------------*/

size_t dtmf_rx_get(dtmf_rx_state_t *s, char *buf, int max)
{
    if (max > s->current_digits)
        max = s->current_digits;
    if (max > 0)
    {
        memcpy(buf, s->digits, max);
        memmove(s->digits, s->digits + max, s->current_digits - max);
        s->current_digits -= max;
    }
    buf[max] = '\0';
    return  max;
}
/*- End of function --------------------------------------------------------*/

void dtmf_rx_set_realtime_callback(dtmf_rx_state_t *s,
                                   tone_report_func_t callback,
                                   void *user_data)
{
    s->realtime_callback = callback;
    s->realtime_callback_data = user_data;
}
/*- End of function --------------------------------------------------------*/

void dtmf_rx_parms(dtmf_rx_state_t *s,
                   int filter_dialtone,
                   int twist,
                   int reverse_twist,
                   int threshold)
{
    float x;

    if (filter_dialtone >= 0)
    {
        s->z350[0] = 0.0f;
        s->z350[1] = 0.0f;
        s->z440[0] = 0.0f;
        s->z440[1] = 0.0f;
        s->filter_dialtone = filter_dialtone;
    }
    if (twist >= 0)
        s->normal_twist = powf(10.0f, twist/10.0f);
    if (reverse_twist >= 0)
        s->reverse_twist = powf(10.0f, reverse_twist/10.0f);
    if (threshold > -99)
    {
        x = (DTMF_SAMPLES_PER_BLOCK*32768.0f/1.4142f)*powf(10.0f, (threshold - DBM0_MAX_SINE_POWER)/20.0f);
        s->threshold = x*x;
    }
}
/*- End of function --------------------------------------------------------*/

dtmf_rx_state_t *dtmf_rx_init(dtmf_rx_state_t *s,
                              digits_rx_callback_t callback,
                              void *user_data)
{
    int i;
    static int initialised = 0;

    if (s == NULL)
    {
        if ((s = (dtmf_rx_state_t *) malloc(sizeof (*s))) == NULL)
            return  NULL;
    }
    s->digits_callback = callback;
    s->digits_callback_data = user_data;
    s->realtime_callback = NULL;
    s->realtime_callback_data = NULL;
    s->filter_dialtone = 0;
    s->normal_twist = DTMF_NORMAL_TWIST;
    s->reverse_twist = DTMF_REVERSE_TWIST;
    s->threshold = DTMF_THRESHOLD;

    s->in_digit = 0;
    s->last_hit = 0;

    if (!initialised)
    {
        for (i = 0;  i < 4;  i++)
        {
            make_goertzel_descriptor(&dtmf_detect_row[i], dtmf_row[i], DTMF_SAMPLES_PER_BLOCK);
            make_goertzel_descriptor(&dtmf_detect_col[i], dtmf_col[i], DTMF_SAMPLES_PER_BLOCK);
        }
        initialised = 1;
    }
    for (i = 0;  i < 4;  i++)
    {
        goertzel_init(&s->row_out[i], &dtmf_detect_row[i]);
        goertzel_init(&s->col_out[i], &dtmf_detect_col[i]);
    }
#if defined(SPANDSP_USE_FIXED_POINT)
    s->energy = 0;
#else
    s->energy = 0.0f;
#endif
    s->current_sample = 0;
    s->lost_digits = 0;
    s->current_digits = 0;
    s->digits[0] = '\0';
    return s;
}
/*- End of function --------------------------------------------------------*/

int dtmf_rx_free(dtmf_rx_state_t *s)
{
    free(s);
    return 0;
}
/*- End of function --------------------------------------------------------*/

/*- End of file ------------------------------------------------------------*/
