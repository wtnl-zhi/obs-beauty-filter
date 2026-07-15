#include "face_tracker.h"

#include <assert.h>
#include <stdio.h>

static struct beauty_face_observation observation(float x, float y, float confidence)
{
	return (struct beauty_face_observation){
		.center_x = x,
		.center_y = y,
		.radius_x = 0.15f,
		.radius_y = 0.20f,
		.left_eye_x = x - 0.05f,
		.left_eye_y = y - 0.04f,
		.right_eye_x = x + 0.05f,
		.right_eye_y = y - 0.04f,
		.mouth_x = x,
		.mouth_y = y + 0.07f,
		.confidence = confidence,
	};
}

int main(void)
{
	struct beauty_face_tracker tracker;
	const uint64_t stale_after_ns = 500000000ULL;
	beauty_face_tracker_init(&tracker, 0.5f, stale_after_ns);

	struct beauty_face_observation first = observation(0.30f, 0.40f, 0.90f);
	beauty_face_tracker_update(&tracker, &first, 1, 100000000ULL);
	assert(beauty_face_tracker_active_count(&tracker) == 1);
	assert(tracker.tracks[0].state.center_x == 0.30f);

	struct beauty_face_observation moved = observation(0.40f, 0.40f, 0.95f);
	beauty_face_tracker_update(&tracker, &moved, 1, 200000000ULL);
	assert(beauty_face_tracker_active_count(&tracker) == 1);
	assert(tracker.tracks[0].state.center_x > 0.34f);
	assert(tracker.tracks[0].state.center_x < 0.36f);

	struct beauty_face_observation second = observation(0.75f, 0.50f, 0.80f);
	struct beauty_face_observation people[] = {moved, second};
	beauty_face_tracker_update(&tracker, people, 2, 300000000ULL);
	assert(beauty_face_tracker_active_count(&tracker) == 2);

	struct beauty_face_observation candidates[] = {
		observation(0.1f, 0.1f, 1.0f), observation(0.2f, 0.2f, 1.0f),
		observation(0.3f, 0.3f, 1.0f), observation(0.4f, 0.4f, 1.0f),
		observation(0.5f, 0.5f, 1.0f),
	};
	candidates[0].radius_x = candidates[0].radius_y = 0.05f;
	candidates[1].radius_x = candidates[1].radius_y = 0.10f;
	candidates[2].radius_x = candidates[2].radius_y = 0.20f;
	candidates[3].radius_x = candidates[3].radius_y = 0.15f;
	candidates[4].radius_x = candidates[4].radius_y = 0.30f;
	struct beauty_face_observation selected[BEAUTY_MAX_FACES] = {0};
	assert(beauty_face_select_largest(candidates, 5, selected, BEAUTY_MAX_FACES) == 4);
	assert(selected[0].center_x == 0.5f && selected[3].center_x == 0.2f);

	beauty_face_tracker_update(&tracker, NULL, 0, 900000001ULL);
	assert(beauty_face_tracker_active_count(&tracker) == 0);

	puts("face tracker tests passed");
	return 0;
}
