/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "beauty_settings_migration.h"

#include "beauty_preset.h"

bool beauty_settings_migrate(struct beauty_settings_migration *settings)
{
	bool changed = false;

	if (!settings)
		return false;

	/* P0 saved sliders only. Keep those values by selecting Custom. */
	if (settings->settings_version < 1) {
		settings->preset = BEAUTY_PRESET_CUSTOM;
		changed = true;
	}
	/* P1 v2 introduced independent effect switches; legacy behavior was on. */
	if (settings->settings_version < 2) {
		settings->smoothing_enabled = true;
		settings->brighten_enabled = true;
		settings->rosy_enabled = true;
		settings->sharpness_enabled = true;
		settings->settings_version = 2;
		changed = true;
	}

	return changed;
}
