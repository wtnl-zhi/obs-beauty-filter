#include "face_landmarks.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void set_landmark(struct beauty_landmark *landmarks, size_t index, float x, float y)
{
	landmarks[index] = (struct beauty_landmark){.x = x, .y = y, .z = 0.0f};
}

int main(void)
{
	struct beauty_landmark landmarks[478];
	struct beauty_face_observation observation;
	memset(landmarks, 0, sizeof(landmarks));

	set_landmark(landmarks, 10, 0.50f, 0.20f);
	set_landmark(landmarks, 152, 0.50f, 0.80f);
	set_landmark(landmarks, 234, 0.25f, 0.50f);
	set_landmark(landmarks, 454, 0.75f, 0.50f);
	set_landmark(landmarks, 33, 0.32f, 0.42f);
	set_landmark(landmarks, 133, 0.43f, 0.42f);
	set_landmark(landmarks, 263, 0.57f, 0.42f);
	set_landmark(landmarks, 362, 0.68f, 0.42f);
	set_landmark(landmarks, 78, 0.40f, 0.65f);
	set_landmark(landmarks, 308, 0.60f, 0.65f);

	assert(beauty_face_observation_from_mediapipe(landmarks, 478, 0.9f, &observation));
	assert(fabsf(observation.center_x - 0.50f) < 0.0001f);
	assert(fabsf(observation.center_y - 0.50f) < 0.0001f);
	assert(fabsf(observation.radius_x - 0.285f) < 0.0001f);
	assert(fabsf(observation.radius_y - 0.330f) < 0.0001f);
	assert(fabsf(observation.left_eye_x - 0.375f) < 0.0001f);
	assert(fabsf(observation.mouth_x - 0.50f) < 0.0001f);

	memset(landmarks, 0, sizeof(landmarks));
	set_landmark(landmarks, 10, 0.70f, 0.30f);
	set_landmark(landmarks, 152, 0.30f, 0.70f);
	set_landmark(landmarks, 234, 0.30f, 0.30f);
	set_landmark(landmarks, 454, 0.70f, 0.70f);
	set_landmark(landmarks, 33, 0.35f, 0.40f);
	set_landmark(landmarks, 133, 0.42f, 0.47f);
	set_landmark(landmarks, 263, 0.58f, 0.53f);
	set_landmark(landmarks, 362, 0.65f, 0.60f);
	set_landmark(landmarks, 78, 0.42f, 0.62f);
	set_landmark(landmarks, 308, 0.58f, 0.78f);
	assert(beauty_face_observation_from_mediapipe(landmarks, 478, 0.9f, &observation));
	assert(fabsf(observation.radius_x - 0.3224407f) < 0.0001f);
	assert(fabsf(observation.radius_y - 0.3111270f) < 0.0001f);

	assert(!beauty_face_observation_from_mediapipe(landmarks, 454, 0.9f, &observation));
	assert(!beauty_face_observation_from_mediapipe(landmarks, 478, 0.0f, &observation));

	puts("face landmark conversion tests passed");
	return 0;
}
