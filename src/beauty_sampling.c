/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "beauty_sampling.h"

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
