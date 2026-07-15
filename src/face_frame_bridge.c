/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "face_frame_bridge.h"

#include <obs-module.h>

struct beauty_frame_bridge {
	struct beauty_face_inference_worker *worker;
	gs_texrender_t *downscale_render;
	gs_stagesurf_t *stages[2];
	uint64_t staged_at_ns[2];
	uint32_t width;
	uint32_t height;
	uint32_t longest_edge;
	uint64_t interval_ns;
	uint64_t next_capture_ns;
	uint8_t next_stage;
};

static void calculate_size(uint32_t source_width, uint32_t source_height, uint32_t longest_edge,
			   uint32_t *width, uint32_t *height)
{
	const uint32_t longest = source_width > source_height ? source_width : source_height;
	if (!longest || !longest_edge) {
		*width = 0;
		*height = 0;
		return;
	}
	if (longest <= longest_edge) {
		*width = source_width;
		*height = source_height;
		return;
	}
	*width = (uint32_t)(((uint64_t)source_width * longest_edge + longest / 2) / longest);
	*height = (uint32_t)(((uint64_t)source_height * longest_edge + longest / 2) / longest);
	*width = *width ? *width : 1;
	*height = *height ? *height : 1;
}

static void release_surfaces(struct beauty_frame_bridge *bridge)
{
	for (size_t index = 0; index < OBS_COUNTOF(bridge->stages); ++index) {
		gs_stagesurface_destroy(bridge->stages[index]);
		bridge->stages[index] = NULL;
		bridge->staged_at_ns[index] = 0;
	}
}

static bool ensure_surfaces(struct beauty_frame_bridge *bridge, uint32_t width, uint32_t height)
{
	if (bridge->width == width && bridge->height == height && bridge->downscale_render &&
	    bridge->stages[0] && bridge->stages[1])
		return true;

	release_surfaces(bridge);
	if (bridge->downscale_render) {
		gs_texrender_destroy(bridge->downscale_render);
		bridge->downscale_render = NULL;
	}
	bridge->width = 0;
	bridge->height = 0;
	bridge->next_stage = 0;

	bridge->downscale_render = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	bridge->stages[0] = gs_stagesurface_create(width, height, GS_RGBA);
	bridge->stages[1] = gs_stagesurface_create(width, height, GS_RGBA);
	if (!bridge->downscale_render || !bridge->stages[0] || !bridge->stages[1]) {
		release_surfaces(bridge);
		gs_texrender_destroy(bridge->downscale_render);
		bridge->downscale_render = NULL;
		return false;
	}
	bridge->width = width;
	bridge->height = height;
	return true;
}

static void submit_ready_stage(struct beauty_frame_bridge *bridge, uint32_t stage_index)
{
	uint8_t *pixels = NULL;
	uint32_t stride = 0;
	if (!bridge->staged_at_ns[stage_index] ||
	    !gs_stagesurface_map(bridge->stages[stage_index], &pixels, &stride))
		return;

	const struct beauty_rgba_frame frame = {
		.pixels = pixels,
		.width = (int)bridge->width,
		.height = (int)bridge->height,
		.stride_bytes = (int)stride,
		.timestamp_ms = bridge->staged_at_ns[stage_index] / UINT64_C(1000000),
	};
	if (!beauty_face_inference_worker_submit(bridge->worker, &frame))
		blog(LOG_WARNING, "[obs-beauty-filter] dropped invalid or stale inference frame");
	gs_stagesurface_unmap(bridge->stages[stage_index]);
	bridge->staged_at_ns[stage_index] = 0;
}

struct beauty_frame_bridge *beauty_frame_bridge_create(struct beauty_face_inference_worker *worker,
								uint32_t longest_edge, uint64_t interval_ns)
{
	if (!worker || !longest_edge || !interval_ns)
		return NULL;
	struct beauty_frame_bridge *bridge = bzalloc(sizeof(*bridge));
	bridge->worker = worker;
	bridge->longest_edge = longest_edge;
	bridge->interval_ns = interval_ns;
	return bridge;
}

void beauty_frame_bridge_destroy(struct beauty_frame_bridge *bridge)
{
	if (!bridge)
		return;
	release_surfaces(bridge);
	gs_texrender_destroy(bridge->downscale_render);
	bfree(bridge);
}

void beauty_frame_bridge_tick(struct beauty_frame_bridge *bridge, gs_texture_t *source_texture,
				      uint64_t now_ns)
{
	if (!bridge || !source_texture || now_ns < bridge->next_capture_ns)
		return;

	uint32_t width = 0;
	uint32_t height = 0;
	calculate_size(gs_texture_get_width(source_texture), gs_texture_get_height(source_texture),
		       bridge->longest_edge, &width, &height);
	if (!width || !height || !ensure_surfaces(bridge, width, height))
		return;

	const uint32_t read_stage = (bridge->next_stage + 1) % OBS_COUNTOF(bridge->stages);
	submit_ready_stage(bridge, read_stage);

	if (!gs_texrender_begin(bridge->downscale_render, width, height))
		return;
	gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");
	gs_enable_blending(false);
	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
	if (gs_get_linear_srgb())
		gs_effect_set_texture_srgb(image, source_texture);
	else
		gs_effect_set_texture(image, source_texture);
	while (gs_effect_loop(effect, "Draw"))
		gs_draw_sprite(source_texture, 0, width, height);
	gs_enable_blending(true);
	gs_texrender_end(bridge->downscale_render);

	gs_texture_t *downscaled = gs_texrender_get_texture(bridge->downscale_render);
	if (!downscaled)
		return;
	gs_stage_texture(bridge->stages[bridge->next_stage], downscaled);
	bridge->staged_at_ns[bridge->next_stage] = now_ns;
	bridge->next_stage = (bridge->next_stage + 1) % OBS_COUNTOF(bridge->stages);
	bridge->next_capture_ns = now_ns + bridge->interval_ns;
}
