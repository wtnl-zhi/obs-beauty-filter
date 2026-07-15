/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "face_inference_worker.h"

#include <cassert>
#include <cstdio>
#include <vector>

int main(int argc, char **argv)
{
	assert(argc == 2);
	char error[512] = {};
	beauty_face_inference_worker *worker = beauty_face_inference_worker_create(
		argv[1], BEAUTY_MAX_FACES, 0.35f, UINT64_C(500000000), error, sizeof(error));
	if (!worker) {
		std::fprintf(stderr, "worker create failed: %s\n", error);
		return 1;
	}

	constexpr int width = 64;
	constexpr int height = 64;
	std::vector<uint8_t> pixels(width * height * 4, 0);
	beauty_rgba_frame frame = {
		.pixels = pixels.data(),
		.width = width,
		.height = height,
		.stride_bytes = width * 4,
		.timestamp_ms = 1,
	};
	if (!beauty_face_inference_worker_submit(worker, &frame)) {
		std::fputs("worker frame submission failed\n", stderr);
		beauty_face_inference_worker_destroy(worker);
		return 1;
	}
	if (!beauty_face_inference_worker_wait_until_processed(worker, frame.timestamp_ms, 30000, error,
								  sizeof(error))) {
		std::fprintf(stderr, "worker inference failed: %s\n", error);
		beauty_face_inference_worker_destroy(worker);
		return 1;
	}

	/* GPU staging surfaces may add row padding; the worker must pack it before inference. */
	std::vector<uint8_t> padded_pixels(height * (width * 4 + 16), 0);
	frame.pixels = padded_pixels.data();
	frame.stride_bytes = width * 4 + 16;
	frame.timestamp_ms = 2;
	if (!beauty_face_inference_worker_submit(worker, &frame) ||
	    !beauty_face_inference_worker_wait_until_processed(worker, frame.timestamp_ms, 30000, error,
									  sizeof(error))) {
		std::fprintf(stderr, "padded worker inference failed: %s\n", error);
		beauty_face_inference_worker_destroy(worker);
		return 1;
	}
	frame.timestamp_ms = 1;
	if (beauty_face_inference_worker_submit(worker, &frame)) {
		std::fputs("worker accepted a non-monotonic timestamp\n", stderr);
		beauty_face_inference_worker_destroy(worker);
		return 1;
	}
	beauty_face_track tracks[BEAUTY_MAX_FACES] = {};
	if (beauty_face_inference_worker_copy_tracks(worker, tracks, BEAUTY_MAX_FACES,
								     UINT64_C(2000000)) !=
	    BEAUTY_MAX_FACES) {
		std::fputs("worker track copy failed\n", stderr);
		beauty_face_inference_worker_destroy(worker);
		return 1;
	}
	for (const beauty_face_track &track : tracks)
		if (track.active) {
			std::fputs("blank frame unexpectedly produced a face track\n", stderr);
			beauty_face_inference_worker_destroy(worker);
			return 1;
		}
	beauty_face_inference_worker_destroy(worker);
	std::puts("MediaPipe face inference worker blank-frame test passed");
	return 0;
}
