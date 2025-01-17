﻿/*
 *  tslib/plugins/variance.c
 *
 *  Copyright (C) 2016 Martin Kepplinger
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Variance filter for touchscreen values.
 *
 * Problem: some touchscreens are sampled very roughly, thus even if
 * you hold the pen still, the samples can differ, sometimes substantially.
 * The worst happens when electric noise during sampling causes the result
 * to be substantially different from the real pen position; this causes
 * the mouse cursor to suddenly "jump" and then return back.
 *
 * Solution: delay sampled data by one timeslot. If we see that the last
 * sample read differs too much, we mark it as "suspicious". If next sample
 * read is close to the sample before the "suspicious", the suspicious sample
 * is dropped, otherwise we consider that a quick pen movement is in progress
 * and pass through both the "suspicious" sample and the sample after it.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#include "config.h"
#include "tslib.h"
#include "tslib-filter.h"

#define ENABLE_PATCH_MT_ISSUE

struct tslib_variance {
	struct tslib_module_info module;
	int delta;
        struct ts_sample last;
        struct ts_sample noise;
	unsigned int flags;
	short last_pen_down;
#define VAR_PENDOWN		0x00000001
#define VAR_LASTVALID		0x00000002
#define VAR_NOISEVALID		0x00000004
#define VAR_SUBMITNOISE		0x00000008
	int slots;
	int nr;
	struct ts_sample_mt **cur_mt;
	struct ts_sample *samp;
};

#ifdef ENABLE_PATCH_MT_ISSUE
static struct ts_sample VarNoise2;
#endif

static int p = 0;    /* for print message if p = 1 */

static int sqr (int x)
{
	return x * x;
}

static int variance_read(struct tslib_module_info *info, struct ts_sample *samp,
			 int nr)
{
	struct tslib_variance *var = (struct tslib_variance *)info;
	struct ts_sample cur;
	struct ts_sample cur2;
	int count = 0, dist;
	struct ts_sample *ori=samp;
	struct ts_sample *nxt = (struct ts_sample *)samp->next;
	unsigned int tmp_flags;
	static unsigned int flag_1st = 1;

#ifdef ENABLE_PATCH_MT_ISSUE
	cur.next = (struct ts_sample *)&cur2;
	cur2 = samp[1];
#else
	cur.next = (struct ts_sample *)samp->next;
#endif

if(p)
{
	if(flag_1st)
	{
		printf("vRd:[%x,%x][%x, %x, %x],(%x)\n", samp, nxt, &cur, cur.next, &cur2, var->flags);
		flag_1st = 0;
	}
	else	printf("vRd:[%x,%x][%d, %d, %d]\n", samp, var->flags, samp->pressure, samp->x, samp->y);
}

	while (count < nr) {
		if (var->flags & VAR_SUBMITNOISE) {
			cur = var->noise;
#ifdef ENABLE_PATCH_MT_ISSUE
			ori->next = nxt;
			cur2 = VarNoise2;
#endif
			tmp_flags = var->flags;
			var->flags &= ~VAR_SUBMITNOISE;
			if(p)	printf("vRd:chgFlg1: %x, %x\n",tmp_flags,var->flags);
		} else {
			if (info->next->ops->read(info->next, &cur, 1) < 1)
			{
				if(p)	printf("vRd:rtn cnt = %d\n", count);
				return count;
			}
			if(p)	printf("vRd:cur[%x,%x][%d, %d, %d]\n", &cur, var->flags, cur.pressure, cur.x, cur.y);
		}

		if (cur.pressure == 0) {
			/* Flush the queue immediately when the pen is just
			 * released, otherwise the previous layer will
			 * get the pen up notification too late. This 
			 * will happen if info->next->ops->read() blocks.
			 * Update var->last with the recent value on pen-
			 * release.
			 */
			var->last = cur;

			/* Reset the state machine on pen up events. */
			tmp_flags = var->flags;
			var->flags &= ~(VAR_PENDOWN | VAR_NOISEVALID |
					VAR_LASTVALID | VAR_SUBMITNOISE);
			if(p)	printf("vRd:chgFlg2: %x, %x\n",tmp_flags,var->flags);
			goto acceptsample;
		} else {
			tmp_flags = var->flags;
			var->flags |= VAR_PENDOWN;
			if(p)	printf("vRd:chgFlg6: %x, %x\n",tmp_flags,var->flags);
		}

		if (!(var->flags & VAR_LASTVALID)) {
			var->last = cur;
			var->flags |= VAR_LASTVALID;
			if(p)	printf("goto accSmp.1\n");
			goto acceptsample;
		}

			if (var->flags & VAR_PENDOWN) {
			/* Compute distance between last sample and current */
			dist = sqr(cur.x - var->last.x) +
			       sqr(cur.y - var->last.y);

			if (dist > var->delta) {
				/* Do we suspect the previous sample was a
				 * noise?
				 */
				if (var->flags & VAR_NOISEVALID) {
					/* Two "noises": it's just a quick pen
					 * movement
					 */
					samp[count++] = var->last = var->noise;
#ifdef ENABLE_PATCH_MT_ISSUE
					ori->next = nxt;
					if(samp->finger>1)	samp[count] = cur2;
#endif
					tmp_flags = var->flags;
					var->flags = (var->flags &
						      ~VAR_NOISEVALID) |
						      VAR_SUBMITNOISE;

					if(p)	printf("vRd:chgFlg3: %x, %x\n",tmp_flags,var->flags);
				} else {
					/* got noise fisrt time */
					tmp_flags = var->flags;
					var->flags |= VAR_NOISEVALID;
					if(p)	printf("vRd:chgFlg4: %x, %x\n",tmp_flags,var->flags);
				}

				/* The pen jumped too far, maybe it's a noise */
				var->noise = cur;
#ifdef ENABLE_PATCH_MT_ISSUE
				VarNoise2 = cur2;
#endif
				continue;
			} else {
				//tmp_flags = var->flags;
				var->flags &= ~VAR_NOISEVALID;
				//printf("vRd:chgFlg5: %x, %x\n",tmp_flags,var->flags);
			}
		}

acceptsample:
#ifdef DEBUG
		fprintf(stderr, "VARIANCE----------------> %d %d %d\n",
			var->last.x, var->last.y, var->last.pressure);
#endif
		if(p)	printf("VAR[%d]---> %d %d %d\n\n", count, var->last.pressure, var->last.x, var->last.y);
			
		samp[count++] = var->last;
		
#ifdef ENABLE_PATCH_MT_ISSUE
		ori->next = nxt;
		if(ori->finger>1)	samp[count] = cur2;
#endif
		var->last = cur;
	}

	return count;
}

/* No multitouch support here! This behaves like the old variance_read() call.
 * Only slot 0 is read.
 */
static int variance_read_mt(struct tslib_module_info *info,
			    struct ts_sample_mt **samp_mt,
			    int max_slots, int nr)
{
	struct tslib_variance *var = (struct tslib_variance *)info;
	int count = 0, dist;
	int i, j;
	struct ts_sample cur;
	short pen_down = 1;
	int ret;

	if (!info->next->ops->read_mt)
		return -ENOSYS;

	if (var->samp == NULL) {
		var->samp = calloc(nr, sizeof(struct ts_sample));
		if (!var->samp)
			return -ENOMEM;
	} else {
		if (nr == var->nr)
			memset(var->samp, 0, nr * sizeof(struct ts_sample));
			}

	if (var->cur_mt == NULL || max_slots > var->slots || nr > var->nr) {
		printf("tslib: WARNING: no multitouch using VARIANCE filter: %x (%x,%x)(%x,%x)\n",var->cur_mt,max_slots,var->slots,nr ,var->nr);
		if (var->cur_mt) {
			for (i = 0; i < var->nr; i++) {
				if (var->cur_mt[i])
					free(var->cur_mt[i]);
			}
			free(var->cur_mt);
		}

		var->cur_mt = malloc(nr * sizeof(struct ts_sample_mt *));
		if (!var->cur_mt)
			return -ENOMEM;

		for (i = 0; i < nr; i++) {
			var->cur_mt[i] = malloc(max_slots * sizeof(struct ts_sample_mt));
			if (!var->cur_mt[i]) {
				for (j = 0; j <= i; j++)
					free(var->cur_mt[j]);

				free(var->cur_mt);
				free(var->samp);
				return -ENOMEM;
			}
			
			{
			    int j;
			    static int my_flag = 0;
			    
			    if(my_flag != max_slots) printf("variance max_slots = %d, %x\n",max_slots, samp_mt[0][0].next);
			        
			    for (j = 0; j < max_slots; j++) 
			        var->cur_mt[0][j].next = (struct ts_sample_mt *)samp_mt[0][j].next;
			        
			    my_flag = max_slots;
			}
		}

		var->slots = max_slots;
		var->nr = nr;
	}

	while (count < nr) {
		if (var->flags & VAR_SUBMITNOISE) {
			cur = var->noise;
			var->flags &= ~VAR_SUBMITNOISE;
		} else {
			ret = info->next->ops->read_mt(info->next, var->cur_mt,
						       max_slots, 1);
			if (ret < 0) {
				count = ret;
				goto out;
			} else if (ret == 0) {
				goto out;
			}

			for (i = 1; i < max_slots; i++) {
				if (var->cur_mt[0][i].valid & TSLIB_MT_VALID) {
				#ifdef DEBUG
					fprintf(stderr,
						"VARIANCE: MT data dropped.\n");
				#endif
					/* XXX Attention. You lose multitouch
					 * using ts_read_mt() with the variance
					 * filter
					 */
					var->cur_mt[0][i].valid = 0;
				}
			}
			if (!(var->cur_mt[0][0].valid & TSLIB_MT_VALID))
				continue;

			cur.x = var->cur_mt[0][0].x;
			cur.y = var->cur_mt[0][0].y;
			cur.pressure = var->cur_mt[0][0].pressure;
			cur.tv = var->cur_mt[0][0].tv;
			cur.finger = var->cur_mt[0][0].finger;
			cur.id = var->cur_mt[0][0].id;
			cur.next = var->cur_mt[0][0].next;
		}

		if (cur.pressure == 0) {
			/* Flush the queue immediately when the pen is just
			 * released, otherwise the previous layer will
			 * get the pen up notification too late. This
			 * will happen if info->next->ops->read() blocks.
			 */
			var->last = cur;
			var->last_pen_down = pen_down;

			/* Reset the state machine on pen up events. */
			var->flags &= ~(VAR_PENDOWN | VAR_NOISEVALID |
					VAR_LASTVALID | VAR_SUBMITNOISE);
			goto acceptsample;
		} else {
			var->flags |= VAR_PENDOWN;
		}

		if (!(var->flags & VAR_LASTVALID)) {
			var->last = cur;
			var->last_pen_down = pen_down;
			var->flags |= VAR_LASTVALID;
			goto acceptsample;
		}

		if (var->flags & VAR_PENDOWN) {
			/* Compute the distance between last sample and
			 * current
			 */
			dist = sqr(cur.x - var->last.x) +
			       sqr(cur.y - var->last.y);

			if (dist > var->delta) {
				/* Do we suspect the previous sample was a
				 * noise?
				 */
				if (var->flags & VAR_NOISEVALID) {
					/* Two "noises": it's just a quick pen
					 * movement
					 */
					var->samp[count] = var->last = var->noise;
					samp_mt[count][0].x = var->samp[count].x;
					samp_mt[count][0].y = var->samp[count].y;
					samp_mt[count][0].pressure = var->samp[count].pressure;
					samp_mt[count][0].tv = var->samp[count].tv;
					samp_mt[count][0].valid |= TSLIB_MT_VALID;
					samp_mt[count][0].slot = var->cur_mt[0][0].slot;
					samp_mt[count][0].tracking_id = var->cur_mt[0][0].tracking_id;
					
					samp_mt[count][0].next = var->samp[count].next;
					samp_mt[count][0].finger = var->samp[count].finger;
					samp_mt[count][0].id = var->samp[count].id;	
					
					samp_mt[count][0].pen_down = pen_down;
					count++;
					var->flags = (var->flags &
						      ~VAR_NOISEVALID) |
						     VAR_SUBMITNOISE;
				} else {
					var->flags |= VAR_NOISEVALID;
				}

				/* The pen jumped too far, maybe it's a noise */
				var->noise = cur;
				continue;
			} else {
				var->flags &= ~VAR_NOISEVALID;
			}
		}

acceptsample:
#ifdef DEBUG
		fprintf(stderr, "VARIANCE----------------> %d %d %d\n",
			var->last.x, var->last.y, var->last.pressure);
#endif
		var->samp[count] = var->last;
		samp_mt[count][0].x = var->samp[count].x;
		samp_mt[count][0].y = var->samp[count].y;
		samp_mt[count][0].pressure = var->samp[count].pressure;
		samp_mt[count][0].tv = var->samp[count].tv;
		samp_mt[count][0].valid |= TSLIB_MT_VALID;
		samp_mt[count][0].slot = var->cur_mt[0][0].slot;
		samp_mt[count][0].tracking_id = var->cur_mt[0][0].tracking_id;
		
		samp_mt[count][0].next = var->samp[count].next;
		samp_mt[count][0].finger = var->samp[count].finger;
		samp_mt[count][0].id = var->samp[count].id;		
		
		samp_mt[count][0].pen_down = var->last_pen_down;

		count++;
		var->last = cur;
	}

out:
	return count;
}

static int variance_fini(struct tslib_module_info *info)
{
	struct tslib_variance *var = (struct tslib_variance *)info;
	int i;

	if (var->cur_mt) {
		for (i = 0; i < var->nr; i++) {
			if (var->cur_mt[i])
				free(var->cur_mt[i]);
		}
		free(var->cur_mt);
	}
	if (var->samp)
		free(var->samp);

	free(info);

	return 0;
}

static const struct tslib_ops variance_ops = {
	variance_read,
	variance_read_mt,
	variance_fini,
};

static int variance_limit(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_variance *var = (struct tslib_variance *)inf;
	unsigned long v;
	int err = errno;

	v = strtoul(str, NULL, 0);

	if (v == ULONG_MAX && errno == ERANGE)
		return -1;

	errno = err;
	switch ((int)(intptr_t)data) {
	case 1:
		var->delta = v;
		break;

	default:
		return -1;
	}
	return 0;
}

static const struct tslib_vars variance_vars[] = {
	{ "delta",	(void *)1, variance_limit },
};

#define NR_VARS (sizeof(variance_vars) / sizeof(variance_vars[0]))

TSAPI struct tslib_module_info *variance_mod_init(__attribute__ ((unused)) struct tsdev *dev,
						  const char *params)
{
	struct tslib_variance *var;

	var = malloc(sizeof(struct tslib_variance));
	if (var == NULL)
		return NULL;

	var->module.ops = &variance_ops;

	var->delta = 30;
	var->flags = 0;
	var->last_pen_down = 1;
	var->slots = 0;
	var->cur_mt = NULL;
	var->nr = 0;
	var->samp = NULL;

	if (tslib_parse_vars(&var->module, variance_vars, NR_VARS, params)) {
		free(var);
		return NULL;
	}

        var->delta = sqr (var->delta);

	return &var->module;
}

#ifndef TSLIB_STATIC_VARIANCE_MODULE
	TSLIB_MODULE_INIT(variance_mod_init);
#endif
