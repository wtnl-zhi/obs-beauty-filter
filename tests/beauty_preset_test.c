/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "beauty_preset.h"

#include <stdio.h>

static int expect_values(int preset, const struct beauty_preset_values expected)
{
	struct beauty_preset_values actual = {0};
	if (!beauty_preset_values_for(preset, &actual) || actual.smoothing != expected.smoothing ||
	    actual.detail != expected.detail || actual.brighten != expected.brighten ||
	    actual.rosy != expected.rosy || actual.sharpness != expected.sharpness) {
		fprintf(stderr, "preset %d did not match expected values\n", preset);
		return 1;
	}
	return 0;
}

int main(void)
{
	if (expect_values(BEAUTY_PRESET_NATURAL,
			  (struct beauty_preset_values){35.0f, 70.0f, 15.0f, 10.0f, 10.0f}) ||
	    expect_values(BEAUTY_PRESET_CLEAR,
			  (struct beauty_preset_values){25.0f, 85.0f, 10.0f, 5.0f, 22.0f}) ||
	    expect_values(BEAUTY_PRESET_LIVE,
			  (struct beauty_preset_values){50.0f, 55.0f, 25.0f, 15.0f, 8.0f}) ||
	    beauty_preset_values_for(BEAUTY_PRESET_CUSTOM, NULL))
		return 1;
	puts("Beauty preset test passed");
	return 0;
}
