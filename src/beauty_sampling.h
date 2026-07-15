/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <stdint.h>

struct beauty_sampling_policy {
	uint32_t longest_edge;
	uint64_t interval_ns;
};

struct beauty_sampling_policy beauty_sampling_policy_for_quality(int quality_mode);
