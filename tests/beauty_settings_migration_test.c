/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "beauty_preset.h"
#include "beauty_settings_migration.h"

#include <stdio.h>

static int expect(bool condition, const char *message)
{
	if (condition)
		return 0;
	fprintf(stderr, "%s\n", message);
	return 1;
}

int main(void)
{
	struct beauty_settings_migration p0 = {
		.settings_version = 0,
		.preset = BEAUTY_PRESET_NATURAL,
	};
	struct beauty_settings_migration v1 = {
		.settings_version = 1,
		.preset = BEAUTY_PRESET_LIVE,
	};
	struct beauty_settings_migration current = {
		.settings_version = 2,
		.preset = BEAUTY_PRESET_CLEAR,
		.smoothing_enabled = false,
		.brighten_enabled = true,
		.rosy_enabled = false,
		.sharpness_enabled = true,
	};

	if (expect(beauty_settings_migrate(&p0), "P0 settings should migrate") ||
	    expect(p0.settings_version == 2, "P0 settings version should become 2") ||
	    expect(p0.preset == BEAUTY_PRESET_CUSTOM, "P0 sliders should keep Custom preset") ||
	    expect(p0.smoothing_enabled && p0.brighten_enabled && p0.rosy_enabled &&
		       p0.sharpness_enabled,
		   "P0 effects should preserve legacy enabled behavior") ||
	    expect(beauty_settings_migrate(&v1), "v1 settings should migrate") ||
	    expect(v1.settings_version == 2 && v1.preset == BEAUTY_PRESET_LIVE,
		   "v1 preset should survive switch migration") ||
	    expect(v1.smoothing_enabled && v1.brighten_enabled && v1.rosy_enabled &&
		       v1.sharpness_enabled,
		   "v1 switches should default to enabled") ||
	    expect(!beauty_settings_migrate(&current), "current settings should not migrate") ||
	    expect(current.preset == BEAUTY_PRESET_CLEAR && !current.smoothing_enabled &&
		       current.brighten_enabled && !current.rosy_enabled && current.sharpness_enabled,
		   "current settings must not be changed") ||
	    expect(!beauty_settings_migrate(NULL), "NULL migration should be a no-op"))
		return 1;

	puts("Beauty settings migration test passed");
	return 0;
}
