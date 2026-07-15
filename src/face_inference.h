/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "face_tracker.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * This is the model-neutral boundary between the OBS integration and an
 * inference runtime. Implementations must be called from a worker thread;
 * none of these functions touch OBS or graphics APIs.
 */
struct beauty_face_inference;

struct beauty_rgba_frame {
	const uint8_t *pixels;
	int width;
	int height;
	int stride_bytes;
	uint64_t timestamp_ms;
};

enum beauty_face_inference_status {
	BEAUTY_FACE_INFERENCE_OK = 0,
	BEAUTY_FACE_INFERENCE_INVALID_ARGUMENT,
	BEAUTY_FACE_INFERENCE_RUNTIME_ERROR,
};

#ifdef __cplusplus
extern "C" {
#endif

struct beauty_face_inference *beauty_mediapipe_face_inference_create(
	const char *model_path, size_t max_faces, char *error, size_t error_size);

enum beauty_face_inference_status beauty_face_inference_detect(
	struct beauty_face_inference *inference, const struct beauty_rgba_frame *frame,
	struct beauty_face_observation *observations, size_t observation_capacity,
	size_t *observation_count, char *error, size_t error_size);

void beauty_face_inference_destroy(struct beauty_face_inference *inference);

#ifdef __cplusplus
}
#endif
