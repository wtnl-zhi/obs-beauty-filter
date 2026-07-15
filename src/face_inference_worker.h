/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "face_inference.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Owns one MediaPipe instance on a dedicated thread. submit() copies the
 * pixels into a bounded, latest-frame queue, so callers may immediately
 * release the source buffer and never block on model execution.
 */
struct beauty_face_inference_worker;

#ifdef __cplusplus
extern "C" {
#endif

struct beauty_face_inference_worker *beauty_face_inference_worker_create(
	const char *model_path, size_t max_faces, float smoothing, uint64_t stale_after_ns,
	char *error, size_t error_size);

bool beauty_face_inference_worker_submit(struct beauty_face_inference_worker *worker,
					 const struct beauty_rgba_frame *frame);

/* Waits until this or a newer frame has completed. Returns false on timeout or inference error. */
bool beauty_face_inference_worker_wait_until_processed(struct beauty_face_inference_worker *worker,
							uint64_t timestamp_ms, uint32_t timeout_ms,
							char *error, size_t error_size);

/* False after a model invocation error; callers should use their compatibility path. */
bool beauty_face_inference_worker_is_healthy(struct beauty_face_inference_worker *worker);

/* Returns the most recent model invocation duration, or zero before the first result. */
uint64_t beauty_face_inference_worker_last_inference_duration_ns(
	struct beauty_face_inference_worker *worker);

size_t beauty_face_inference_worker_copy_tracks(struct beauty_face_inference_worker *worker,
							 struct beauty_face_track *tracks, size_t capacity,
							 uint64_t now_ns);

void beauty_face_inference_worker_destroy(struct beauty_face_inference_worker *worker);

#ifdef __cplusplus
}
#endif
