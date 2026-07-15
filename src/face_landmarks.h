/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "face_tracker.h"

#include <stdbool.h>
#include <stddef.h>

#define BEAUTY_MEDIAPIPE_REQUIRED_LANDMARKS 455

struct beauty_landmark {
	float x;
	float y;
	float z;
};

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Converts MediaPipe Face Mesh normalized landmark indices into the compact
 * observation consumed by the model-independent tracker and renderer.
 *
 * Required indices: forehead 10, chin 152, left/right face 234/454,
 * eye corners 33/133/263/362, and mouth corners 78/308.
 */
bool beauty_face_observation_from_mediapipe(const struct beauty_landmark *landmarks,
					    size_t landmark_count, float confidence,
					    struct beauty_face_observation *observation);

#ifdef __cplusplus
}
#endif
