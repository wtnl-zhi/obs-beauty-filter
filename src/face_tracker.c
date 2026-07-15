/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "face_tracker.h"

#include <math.h>
#include <string.h>

static bool observation_is_valid(const struct beauty_face_observation *observation)
{
	return observation->center_x >= 0.0f && observation->center_x <= 1.0f &&
	       observation->center_y >= 0.0f && observation->center_y <= 1.0f &&
	       observation->radius_x > 0.0f && observation->radius_x <= 1.0f &&
	       observation->radius_y > 0.0f && observation->radius_y <= 1.0f &&
	       observation->confidence > 0.0f;
}

size_t beauty_face_select_largest(const struct beauty_face_observation *candidates,
				  size_t candidate_count, struct beauty_face_observation *selected,
				  size_t selected_capacity)
{
	if (!candidates || !selected || !selected_capacity)
		return 0;
	size_t selected_count = 0;
	for (size_t candidate_index = 0; candidate_index < candidate_count; ++candidate_index) {
		const struct beauty_face_observation *candidate = &candidates[candidate_index];
		if (!observation_is_valid(candidate))
			continue;
		size_t insert_at = selected_count;
		const float area = candidate->radius_x * candidate->radius_y;
		while (insert_at && area > selected[insert_at - 1].radius_x * selected[insert_at - 1].radius_y)
			--insert_at;
		if (insert_at >= selected_capacity)
			continue;
		const size_t move_count = selected_count < selected_capacity ? selected_count : selected_capacity - 1;
		for (size_t index = move_count; index > insert_at; --index)
			selected[index] = selected[index - 1];
		selected[insert_at] = *candidate;
		if (selected_count < selected_capacity)
			++selected_count;
	}
	return selected_count;
}

static float squared_distance(const struct beauty_face_observation *left,
			      const struct beauty_face_observation *right)
{
	const float dx = left->center_x - right->center_x;
	const float dy = left->center_y - right->center_y;
	return dx * dx + dy * dy;
}

static float blend(float previous, float current, float alpha)
{
	return previous + (current - previous) * alpha;
}

static void smooth_observation(struct beauty_face_observation *state,
			       const struct beauty_face_observation *observation, float alpha)
{
	state->center_x = blend(state->center_x, observation->center_x, alpha);
	state->center_y = blend(state->center_y, observation->center_y, alpha);
	state->radius_x = blend(state->radius_x, observation->radius_x, alpha);
	state->radius_y = blend(state->radius_y, observation->radius_y, alpha);
	state->left_eye_x = blend(state->left_eye_x, observation->left_eye_x, alpha);
	state->left_eye_y = blend(state->left_eye_y, observation->left_eye_y, alpha);
	state->right_eye_x = blend(state->right_eye_x, observation->right_eye_x, alpha);
	state->right_eye_y = blend(state->right_eye_y, observation->right_eye_y, alpha);
	state->mouth_x = blend(state->mouth_x, observation->mouth_x, alpha);
	state->mouth_y = blend(state->mouth_y, observation->mouth_y, alpha);
	state->confidence = observation->confidence;
}

void beauty_face_tracker_init(struct beauty_face_tracker *tracker, float smoothing,
				      uint64_t stale_after_ns)
{
	if (!tracker)
		return;

	memset(tracker, 0, sizeof(*tracker));
	tracker->smoothing = fminf(fmaxf(smoothing, 0.01f), 1.0f);
	tracker->stale_after_ns = stale_after_ns;
}

static int find_nearest_track(const struct beauty_face_tracker *tracker,
			      const struct beauty_face_observation *observation, uint64_t now_ns)
{
	float best_distance = 0.0f;
	int best_index = -1;

	for (size_t index = 0; index < BEAUTY_MAX_FACES; index++) {
		const struct beauty_face_track *track = &tracker->tracks[index];
		if (!track->active || now_ns - track->last_seen_ns > tracker->stale_after_ns)
			continue;

		const float distance = squared_distance(&track->state, observation);
		if (distance > 0.04f)
			continue;
		if (best_index < 0 || distance < best_distance) {
			best_distance = distance;
			best_index = (int)index;
		}
	}

	return best_index;
}

static int find_available_track(const struct beauty_face_tracker *tracker, uint64_t now_ns)
{
	int weakest_index = 0;
	float weakest_confidence = tracker->tracks[0].state.confidence;

	for (size_t index = 0; index < BEAUTY_MAX_FACES; index++) {
		const struct beauty_face_track *track = &tracker->tracks[index];
		if (!track->active || now_ns - track->last_seen_ns > tracker->stale_after_ns)
			return (int)index;
		if (track->state.confidence < weakest_confidence) {
			weakest_confidence = track->state.confidence;
			weakest_index = (int)index;
		}
	}

	return weakest_index;
}

void beauty_face_tracker_update(struct beauty_face_tracker *tracker,
				const struct beauty_face_observation *observations, size_t count,
				uint64_t now_ns)
{
	if (!tracker)
		return;

	for (size_t index = 0; index < BEAUTY_MAX_FACES; index++) {
		struct beauty_face_track *track = &tracker->tracks[index];
		if (track->active && now_ns - track->last_seen_ns > tracker->stale_after_ns)
			track->active = false;
	}

	for (size_t index = 0; observations && index < count; index++) {
		const struct beauty_face_observation *observation = &observations[index];
		if (!observation_is_valid(observation))
			continue;

		int track_index = find_nearest_track(tracker, observation, now_ns);
		if (track_index < 0)
			track_index = find_available_track(tracker, now_ns);

		struct beauty_face_track *track = &tracker->tracks[track_index];
		if (!track->active) {
			track->state = *observation;
			track->active = true;
		} else {
			smooth_observation(&track->state, observation, tracker->smoothing);
		}
		track->last_seen_ns = now_ns;
	}
}

size_t beauty_face_tracker_active_count(const struct beauty_face_tracker *tracker)
{
	size_t count = 0;
	if (!tracker)
		return 0;

	for (size_t index = 0; index < BEAUTY_MAX_FACES; index++) {
		if (tracker->tracks[index].active)
			count++;
	}
	return count;
}
