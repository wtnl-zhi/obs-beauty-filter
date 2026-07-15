/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <stdbool.h>

struct beauty_settings_migration {
	int settings_version;
	int preset;
	bool smoothing_enabled;
	bool brighten_enabled;
	bool rosy_enabled;
	bool sharpness_enabled;
};

/* Migrates saved P0/P1 settings in-place. Returns whether data changed. */
bool beauty_settings_migrate(struct beauty_settings_migration *settings);
