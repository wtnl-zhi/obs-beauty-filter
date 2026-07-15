/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "face_inference.h"
#include "face_landmarks.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

#include "mediapipe/tasks/c/core/common.h"
#include "mediapipe/tasks/c/vision/face_landmarker/face_landmarker.h"

struct beauty_face_inference {
	MpFaceLandmarkerPtr landmarker = nullptr;
	size_t max_faces = 0;
	uint64_t last_timestamp_ms = 0;
	bool has_timestamp = false;
};

static void clear_error(char *error, size_t error_size)
{
	if (error && error_size)
		error[0] = '\0';
}

static void set_error(char *error, size_t error_size, const char *message)
{
	if (!error || !error_size)
		return;
	std::snprintf(error, error_size, "%s", message ? message : "未知推理错误");
}

static void set_mediapipe_error(char *error, size_t error_size, char *mediapipe_error,
				const char *fallback)
{
	set_error(error, error_size, mediapipe_error ? mediapipe_error : fallback);
	if (mediapipe_error)
		MpErrorFree(mediapipe_error);
}

extern "C" struct beauty_face_inference *beauty_mediapipe_face_inference_create(
	const char *model_path, size_t max_faces, char *error, size_t error_size)
{
	clear_error(error, error_size);
	if (!model_path || !model_path[0] || max_faces == 0 || max_faces > BEAUTY_MAX_FACES) {
		set_error(error, error_size, "人脸模型路径或最大人脸数无效");
		return nullptr;
	}

	auto inference = std::make_unique<beauty_face_inference>();
	FaceLandmarkerOptions options = {};
	options.base_options.model_asset_path = model_path;
	options.base_options.delegate = CPU;
	options.running_mode = VIDEO;
	options.num_faces = static_cast<int>(max_faces);
	options.min_face_detection_confidence = 0.5f;
	options.min_face_presence_confidence = 0.5f;
	options.min_tracking_confidence = 0.5f;

	char *mediapipe_error = nullptr;
	const MpStatus status =
		MpFaceLandmarkerCreate(&options, &inference->landmarker, &mediapipe_error);
	if (status != kMpOk || !inference->landmarker) {
		set_mediapipe_error(error, error_size, mediapipe_error, "无法加载 MediaPipe 人脸模型");
		return nullptr;
	}

	inference->max_faces = max_faces;
	return inference.release();
}

extern "C" enum beauty_face_inference_status beauty_face_inference_detect(
	struct beauty_face_inference *inference, const struct beauty_rgba_frame *frame,
	struct beauty_face_observation *observations, size_t observation_capacity,
	size_t *observation_count, char *error, size_t error_size)
{
	clear_error(error, error_size);
	if (observation_count)
		*observation_count = 0;
	if (!inference || !frame || !frame->pixels || !observations || !observation_count ||
	    frame->width <= 0 || frame->height <= 0 ||
	    frame->width > std::numeric_limits<int>::max() / 4 ||
	    frame->stride_bytes != frame->width * 4 ||
	    observation_capacity == 0 || observation_capacity > BEAUTY_MAX_FACES ||
	    observation_capacity > inference->max_faces ||
	    frame->timestamp_ms > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) ||
	    (inference->has_timestamp && frame->timestamp_ms <= inference->last_timestamp_ms)) {
		set_error(error, error_size, "人脸推理帧参数无效或时间戳未递增");
		return BEAUTY_FACE_INFERENCE_INVALID_ARGUMENT;
	}

	const size_t byte_count = static_cast<size_t>(frame->height) *
				  static_cast<size_t>(frame->stride_bytes);
	if (byte_count > static_cast<size_t>(std::numeric_limits<int>::max())) {
		set_error(error, error_size, "人脸推理帧尺寸过大");
		return BEAUTY_FACE_INFERENCE_INVALID_ARGUMENT;
	}

	MpImagePtr image = nullptr;
	char *mediapipe_error = nullptr;
	MpStatus status = MpImageCreateFromUint8Data(
		kMpImageFormatSrgba, frame->width, frame->height, frame->pixels,
		static_cast<int>(byte_count), &image, &mediapipe_error);
	if (status != kMpOk || !image) {
		set_mediapipe_error(error, error_size, mediapipe_error, "无法创建人脸推理图像");
		return BEAUTY_FACE_INFERENCE_RUNTIME_ERROR;
	}

	FaceLandmarkerResult result = {};
	mediapipe_error = nullptr;
	status = MpFaceLandmarkerDetectForVideo(inference->landmarker, image, nullptr,
						static_cast<int64_t>(frame->timestamp_ms), &result,
						&mediapipe_error);
	MpImageFree(image);
	if (status != kMpOk) {
		set_mediapipe_error(error, error_size, mediapipe_error, "人脸推理执行失败");
		return BEAUTY_FACE_INFERENCE_RUNTIME_ERROR;
	}

	const size_t face_count = std::min(static_cast<size_t>(result.face_landmarks_count),
					   std::min(observation_capacity, inference->max_faces));
	std::vector<beauty_landmark> landmarks;
	landmarks.reserve(478);
	for (size_t face_index = 0; face_index < face_count; ++face_index) {
		const NormalizedLandmarks &face = result.face_landmarks[face_index];
		landmarks.clear();
		for (uint32_t landmark_index = 0; landmark_index < face.landmarks_count;
		     ++landmark_index) {
			const NormalizedLandmark &landmark = face.landmarks[landmark_index];
			landmarks.push_back({landmark.x, landmark.y, landmark.z});
		}
		if (beauty_face_observation_from_mediapipe(landmarks.data(), landmarks.size(), 1.0f,
						   &observations[*observation_count]))
			++*observation_count;
	}
	MpFaceLandmarkerCloseResult(&result);
	inference->last_timestamp_ms = frame->timestamp_ms;
	inference->has_timestamp = true;
	return BEAUTY_FACE_INFERENCE_OK;
}

extern "C" void beauty_face_inference_destroy(struct beauty_face_inference *inference)
{
	if (!inference)
		return;
	if (inference->landmarker) {
		char *mediapipe_error = nullptr;
		MpFaceLandmarkerClose(inference->landmarker, &mediapipe_error);
		if (mediapipe_error)
			MpErrorFree(mediapipe_error);
	}
	delete inference;
}
