/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "face_inference.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

int main(int argc, char **argv)
{
	assert(argc == 2);
	char error[512] = {};
	const std::string missing_model = std::string(argv[1]) + ".missing";
	if (beauty_mediapipe_face_inference_create(missing_model.c_str(), BEAUTY_MAX_FACES, error,
						   sizeof(error))) {
		std::fputs("missing model unexpectedly created inference\n", stderr);
		return 1;
	}
	if (!error[0]) {
		std::fputs("missing model did not return an error\n", stderr);
		return 1;
	}
	beauty_face_inference *inference =
		beauty_mediapipe_face_inference_create(argv[1], BEAUTY_MAX_FACES, error, sizeof(error));
	if (!inference) {
		std::fprintf(stderr, "create failed: %s\n", error);
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
	beauty_face_observation observations[BEAUTY_MAX_FACES] = {};
	size_t observation_count = 0;
	const beauty_face_inference_status status = beauty_face_inference_detect(
		inference, &frame, observations, BEAUTY_MAX_FACES, &observation_count, error,
		sizeof(error));
	if (status != BEAUTY_FACE_INFERENCE_OK) {
		std::fprintf(stderr, "detect failed: %s\n", error);
		beauty_face_inference_destroy(inference);
		return 1;
	}
	assert(observation_count == 0);
	beauty_face_inference_destroy(inference);
	std::puts("MediaPipe face inference blank-frame test passed");
	return 0;
}
