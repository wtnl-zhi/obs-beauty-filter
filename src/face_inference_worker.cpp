/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "face_inference_worker.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

struct queued_frame {
	std::vector<uint8_t> pixels;
	int width = 0;
	int height = 0;
	int stride_bytes = 0;
	uint64_t timestamp_ms = 0;
};

struct beauty_face_inference_worker {
	std::mutex mutex;
	std::condition_variable wake_worker;
	std::condition_variable result_ready;
	std::thread thread;
	struct beauty_face_inference *inference = nullptr;
	struct beauty_face_tracker tracker = {};
	queued_frame pending;
	bool has_pending = false;
	bool stop = false;
	bool has_submitted_timestamp = false;
	uint64_t latest_submitted_timestamp_ms = 0;
	uint64_t latest_processed_timestamp_ms = 0;
	char last_error[512] = {};
};

static void clear_error(char *error, size_t error_size)
{
	if (error && error_size)
		error[0] = '\0';
}

static void set_error(char *error, size_t error_size, const char *message)
{
	if (error && error_size)
		std::snprintf(error, error_size, "%s", message ? message : "未知后台推理错误");
}

static void worker_main(struct beauty_face_inference_worker *worker)
{
	for (;;) {
		queued_frame frame;
		{
			std::unique_lock lock(worker->mutex);
			worker->wake_worker.wait(lock, [worker] { return worker->stop || worker->has_pending; });
			if (worker->stop)
				return;
			frame = std::move(worker->pending);
			worker->has_pending = false;
		}

		struct beauty_rgba_frame inference_frame = {
			.pixels = frame.pixels.data(),
			.width = frame.width,
			.height = frame.height,
			.stride_bytes = frame.stride_bytes,
			.timestamp_ms = frame.timestamp_ms,
		};
		struct beauty_face_observation observations[BEAUTY_MAX_FACES] = {};
		size_t observation_count = 0;
		char error[sizeof(worker->last_error)] = {};
		const enum beauty_face_inference_status status = beauty_face_inference_detect(
			worker->inference, &inference_frame, observations, BEAUTY_MAX_FACES,
			&observation_count, error, sizeof(error));

		{
			std::lock_guard lock(worker->mutex);
			worker->latest_processed_timestamp_ms = frame.timestamp_ms;
			if (status == BEAUTY_FACE_INFERENCE_OK) {
				beauty_face_tracker_update(&worker->tracker, observations, observation_count,
							 frame.timestamp_ms * UINT64_C(1000000));
				worker->last_error[0] = '\0';
			} else {
				set_error(worker->last_error, sizeof(worker->last_error), error);
			}
		}
		worker->result_ready.notify_all();
	}
}

extern "C" struct beauty_face_inference_worker *beauty_face_inference_worker_create(
	const char *model_path, size_t max_faces, float smoothing, uint64_t stale_after_ns,
	char *error, size_t error_size)
{
	clear_error(error, error_size);
	auto worker = std::make_unique<beauty_face_inference_worker>();
	worker->inference = beauty_mediapipe_face_inference_create(model_path, max_faces, error, error_size);
	if (!worker->inference)
		return nullptr;
	beauty_face_tracker_init(&worker->tracker, smoothing, stale_after_ns);
	try {
		worker->thread = std::thread(worker_main, worker.get());
	} catch (...) {
		beauty_face_inference_destroy(worker->inference);
		set_error(error, error_size, "无法创建人脸推理线程");
		return nullptr;
	}
	return worker.release();
}

extern "C" bool beauty_face_inference_worker_submit(struct beauty_face_inference_worker *worker,
							     const struct beauty_rgba_frame *frame)
{
	if (!worker || !frame || !frame->pixels || frame->width <= 0 || frame->height <= 0 ||
	    frame->width > std::numeric_limits<int>::max() / 4 ||
	    frame->stride_bytes < frame->width * 4)
		return false;
	const size_t row_bytes = static_cast<size_t>(frame->width) * 4;
	if (static_cast<size_t>(frame->height) > std::numeric_limits<size_t>::max() / row_bytes)
		return false;
	std::vector<uint8_t> pixels(static_cast<size_t>(frame->height) * row_bytes);
	for (int row = 0; row < frame->height; ++row)
		std::memcpy(pixels.data() + static_cast<size_t>(row) * row_bytes,
			    frame->pixels + static_cast<size_t>(row) * frame->stride_bytes, row_bytes);

	{
		std::lock_guard lock(worker->mutex);
		if (worker->has_submitted_timestamp && frame->timestamp_ms <= worker->latest_submitted_timestamp_ms)
			return false;
		worker->pending = {std::move(pixels), frame->width, frame->height, (int)row_bytes,
					   frame->timestamp_ms};
		worker->has_pending = true;
		worker->has_submitted_timestamp = true;
		worker->latest_submitted_timestamp_ms = frame->timestamp_ms;
	}
	worker->wake_worker.notify_one();
	return true;
}

extern "C" bool beauty_face_inference_worker_wait_until_processed(
	struct beauty_face_inference_worker *worker, uint64_t timestamp_ms, uint32_t timeout_ms,
	char *error, size_t error_size)
{
	clear_error(error, error_size);
	if (!worker)
		return false;
	std::unique_lock lock(worker->mutex);
	const bool ready = worker->result_ready.wait_for(
		lock, std::chrono::milliseconds(timeout_ms), [worker, timestamp_ms] {
			return worker->latest_processed_timestamp_ms >= timestamp_ms || worker->stop;
		});
	if (!ready || worker->latest_processed_timestamp_ms < timestamp_ms) {
		set_error(error, error_size, "等待人脸推理结果超时");
		return false;
	}
	if (worker->last_error[0]) {
		set_error(error, error_size, worker->last_error);
		return false;
	}
	return true;
}

extern "C" size_t beauty_face_inference_worker_copy_tracks(
	struct beauty_face_inference_worker *worker, struct beauty_face_track *tracks, size_t capacity)
{
	if (!worker || !tracks || capacity == 0)
		return 0;
	std::lock_guard lock(worker->mutex);
	const size_t count = std::min(capacity, static_cast<size_t>(BEAUTY_MAX_FACES));
	std::memcpy(tracks, worker->tracker.tracks, count * sizeof(*tracks));
	return count;
}

extern "C" void beauty_face_inference_worker_destroy(struct beauty_face_inference_worker *worker)
{
	if (!worker)
		return;
	{
		std::lock_guard lock(worker->mutex);
		worker->stop = true;
	}
	worker->wake_worker.notify_one();
	worker->result_ready.notify_all();
	if (worker->thread.joinable())
		worker->thread.join();
	beauty_face_inference_destroy(worker->inference);
	delete worker;
}
