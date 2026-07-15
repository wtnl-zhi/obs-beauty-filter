/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BEAUTY_MAX_FACES 4
#define BEAUTY_MAX_DETECTED_FACES 8

/*
 * All coordinates are normalized to the input frame (0.0–1.0). The MediaPipe
 * adapter will derive these compact ellipses from its full landmark output;
 * the renderer therefore does not depend on a model-specific landmark count.
 */
struct beauty_face_observation {
	float center_x;
	float center_y;
	float radius_x;
	float radius_y;
	float left_eye_x;
	float left_eye_y;
	float right_eye_x;
	float right_eye_y;
	float mouth_x;
	float mouth_y;
	float confidence;
};

struct beauty_face_track {
	bool active;
	uint64_t last_seen_ns;
	struct beauty_face_observation state;
};

struct beauty_face_tracker {
	struct beauty_face_track tracks[BEAUTY_MAX_FACES];
	float smoothing;
	uint64_t stale_after_ns;
};

#ifdef __cplusplus
extern "C" {
#endif

void beauty_face_tracker_init(struct beauty_face_tracker *tracker, float smoothing,
				      uint64_t stale_after_ns);

/*
 * Associates each observation with the closest existing track, then applies
 * exponential smoothing. Observations with invalid geometry are ignored.
 */
void beauty_face_tracker_update(struct beauty_face_tracker *tracker,
				const struct beauty_face_observation *observations, size_t count,
				uint64_t now_ns);

/* Copies the largest valid faces, ordered by descending ellipse area. */
size_t beauty_face_select_largest(const struct beauty_face_observation *candidates,
				  size_t candidate_count, struct beauty_face_observation *selected,
				  size_t selected_capacity);

size_t beauty_face_tracker_active_count(const struct beauty_face_tracker *tracker);

#ifdef __cplusplus
}
#endif
