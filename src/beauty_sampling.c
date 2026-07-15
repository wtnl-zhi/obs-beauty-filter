/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "beauty_sampling.h"

#define AUTO_DEGRADE_AFTER_NS UINT64_C(120000000)
#define AUTO_RECOVER_BELOW_NS UINT64_C(75000000)

struct beauty_sampling_policy beauty_sampling_policy_for_quality(int quality_mode)
{
	switch (quality_mode) {
	case 1: /* Compatible */
		return (struct beauty_sampling_policy){192, UINT64_C(166666667)};
	case 2: /* High */
		return (struct beauty_sampling_policy){384, UINT64_C(66666667)};
	default: /* Auto */
		return (struct beauty_sampling_policy){256, UINT64_C(100000000)};
	}
}

bool beauty_sampling_auto_is_degraded(bool currently_degraded, uint64_t inference_duration_ns)
{
	if (!inference_duration_ns)
		return currently_degraded;
	if (currently_degraded)
		return inference_duration_ns >= AUTO_RECOVER_BELOW_NS;
	return inference_duration_ns > AUTO_DEGRADE_AFTER_NS;
}
