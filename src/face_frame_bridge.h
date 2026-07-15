/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "face_inference_worker.h"

#include <graphics/graphics.h>

#include <stdbool.h>
#include <stdint.h>

/* All functions run on the OBS graphics/render thread. */
struct beauty_frame_bridge;

struct beauty_frame_bridge *beauty_frame_bridge_create(struct beauty_face_inference_worker *worker,
								uint32_t longest_edge, uint64_t interval_ns);
void beauty_frame_bridge_destroy(struct beauty_frame_bridge *bridge);

/* Stages a downscaled source texture and submits a previously-ready frame. */
void beauty_frame_bridge_tick(struct beauty_frame_bridge *bridge, gs_texture_t *source_texture,
				      uint64_t now_ns);
