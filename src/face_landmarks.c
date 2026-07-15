/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "face_landmarks.h"

#include <math.h>

enum mediapipe_face_landmark_index {
	LANDMARK_FOREHEAD = 10,
	LANDMARK_CHIN = 152,
	LANDMARK_LEFT_FACE = 234,
	LANDMARK_RIGHT_FACE = 454,
	LANDMARK_LEFT_EYE_OUTER = 33,
	LANDMARK_LEFT_EYE_INNER = 133,
	LANDMARK_RIGHT_EYE_INNER = 263,
	LANDMARK_RIGHT_EYE_OUTER = 362,
	LANDMARK_MOUTH_LEFT = 78,
	LANDMARK_MOUTH_RIGHT = 308,
};

static float midpoint(float first, float second)
{
	return (first + second) * 0.5f;
}

static bool valid_coordinate(float value)
{
	return isfinite(value) && value >= 0.0f && value <= 1.0f;
}

static bool valid_landmark(const struct beauty_landmark *landmark)
{
	return valid_coordinate(landmark->x) && valid_coordinate(landmark->y);
}

bool beauty_face_observation_from_mediapipe(const struct beauty_landmark *landmarks,
					    size_t landmark_count, float confidence,
					    struct beauty_face_observation *observation)
{
	if (!landmarks || !observation || landmark_count < BEAUTY_MEDIAPIPE_REQUIRED_LANDMARKS ||
	    !isfinite(confidence) || confidence <= 0.0f)
		return false;

	const struct beauty_landmark *forehead = &landmarks[LANDMARK_FOREHEAD];
	const struct beauty_landmark *chin = &landmarks[LANDMARK_CHIN];
	const struct beauty_landmark *left_face = &landmarks[LANDMARK_LEFT_FACE];
	const struct beauty_landmark *right_face = &landmarks[LANDMARK_RIGHT_FACE];
	const struct beauty_landmark *left_eye_outer = &landmarks[LANDMARK_LEFT_EYE_OUTER];
	const struct beauty_landmark *left_eye_inner = &landmarks[LANDMARK_LEFT_EYE_INNER];
	const struct beauty_landmark *right_eye_inner = &landmarks[LANDMARK_RIGHT_EYE_INNER];
	const struct beauty_landmark *right_eye_outer = &landmarks[LANDMARK_RIGHT_EYE_OUTER];
	const struct beauty_landmark *mouth_left = &landmarks[LANDMARK_MOUTH_LEFT];
	const struct beauty_landmark *mouth_right = &landmarks[LANDMARK_MOUTH_RIGHT];

	if (!valid_landmark(forehead) || !valid_landmark(chin) || !valid_landmark(left_face) ||
	    !valid_landmark(right_face) || !valid_landmark(left_eye_outer) ||
	    !valid_landmark(left_eye_inner) || !valid_landmark(right_eye_inner) ||
	    !valid_landmark(right_eye_outer) || !valid_landmark(mouth_left) ||
	    !valid_landmark(mouth_right))
		return false;

	const float face_width = fabsf(right_face->x - left_face->x);
	const float face_height = fabsf(chin->y - forehead->y);
	if (face_width < 0.02f || face_height < 0.02f)
		return false;

	*observation = (struct beauty_face_observation){
		.center_x = midpoint(left_face->x, right_face->x),
		.center_y = midpoint(forehead->y, chin->y),
		.radius_x = face_width * 0.57f,
		.radius_y = face_height * 0.55f,
		.left_eye_x = midpoint(left_eye_outer->x, left_eye_inner->x),
		.left_eye_y = midpoint(left_eye_outer->y, left_eye_inner->y),
		.right_eye_x = midpoint(right_eye_inner->x, right_eye_outer->x),
		.right_eye_y = midpoint(right_eye_inner->y, right_eye_outer->y),
		.mouth_x = midpoint(mouth_left->x, mouth_right->x),
		.mouth_y = midpoint(mouth_left->y, mouth_right->y),
		.confidence = fminf(confidence, 1.0f),
	};
	return true;
}
