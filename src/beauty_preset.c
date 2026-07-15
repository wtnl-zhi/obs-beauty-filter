/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "beauty_preset.h"

bool beauty_preset_values_for(int preset, struct beauty_preset_values *values)
{
	if (!values)
		return false;

	switch (preset) {
	case BEAUTY_PRESET_NATURAL:
		*values = (struct beauty_preset_values){35.0f, 70.0f, 15.0f, 10.0f, 10.0f};
		return true;
	case BEAUTY_PRESET_CLEAR:
		*values = (struct beauty_preset_values){25.0f, 85.0f, 10.0f, 5.0f, 22.0f};
		return true;
	case BEAUTY_PRESET_LIVE:
		*values = (struct beauty_preset_values){50.0f, 55.0f, 25.0f, 15.0f, 8.0f};
		return true;
	default:
		return false;
	}
}
