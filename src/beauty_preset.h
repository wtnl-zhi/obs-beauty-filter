/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <stdbool.h>

enum beauty_preset {
	BEAUTY_PRESET_NATURAL = 0,
	BEAUTY_PRESET_CLEAR,
	BEAUTY_PRESET_LIVE,
	BEAUTY_PRESET_CUSTOM,
};

struct beauty_preset_values {
	float smoothing;
	float detail;
	float brighten;
	float rosy;
	float sharpness;
};

/* Returns false for Custom or an unknown preset. Values use the UI's 0–100 scale. */
bool beauty_preset_values_for(int preset, struct beauty_preset_values *values);
