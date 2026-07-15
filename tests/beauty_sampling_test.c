/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "beauty_sampling.h"

#include <stdio.h>

static int expect_policy(int mode, uint32_t longest_edge, uint64_t interval_ns)
{
	const struct beauty_sampling_policy actual = beauty_sampling_policy_for_quality(mode);
	if (actual.longest_edge != longest_edge || actual.interval_ns != interval_ns) {
		fprintf(stderr, "unexpected policy for quality mode %d\n", mode);
		return 1;
	}
	return 0;
}

int main(void)
{
	return expect_policy(0, 256, UINT64_C(100000000)) ||
	       expect_policy(1, 192, UINT64_C(166666667)) ||
	       expect_policy(2, 384, UINT64_C(66666667)) ||
	       expect_policy(99, 256, UINT64_C(100000000)) ||
	       beauty_sampling_auto_is_degraded(false, UINT64_C(120000000)) ||
	       !beauty_sampling_auto_is_degraded(false, UINT64_C(120000001)) ||
	       !beauty_sampling_auto_is_degraded(true, UINT64_C(75000000)) ||
	       beauty_sampling_auto_is_degraded(true, UINT64_C(74999999));
}
