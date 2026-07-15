/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <obs-module.h>
#include <obs-source.h>
#include <util/platform.h>

#include "face_tracker.h"
#include "beauty_preset.h"

#include <stdio.h>

#ifdef OBS_BEAUTY_ENABLE_MEDIAPIPE
#include "face_frame_bridge.h"
#endif

struct beauty_filter {
	obs_source_t *context;
	gs_effect_t *effect;
	gs_eparam_t *smoothing_param;
	gs_eparam_t *detail_param;
	gs_eparam_t *brighten_param;
	gs_eparam_t *rosy_param;
	gs_eparam_t *sharpness_param;
	gs_eparam_t *texture_width_param;
	gs_eparam_t *texture_height_param;
	gs_eparam_t *image_param;
	gs_eparam_t *face_mask_enabled_param;
	gs_eparam_t *face_params[BEAUTY_MAX_FACES];
	gs_eparam_t *eyes_params[BEAUTY_MAX_FACES];
	gs_eparam_t *mouth_params[BEAUTY_MAX_FACES];

	bool enabled;
	float smoothing;
	float detail;
	float brighten;
	float rosy;
	float sharpness;
	float quality_scale;

#ifdef OBS_BEAUTY_ENABLE_MEDIAPIPE
	struct beauty_face_inference_worker *inference_worker;
	struct beauty_frame_bridge *frame_bridge;
	gs_texrender_t *source_render;
#endif
};

static const char *beauty_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Filter.Name");
}

static void beauty_filter_update(void *data, obs_data_t *settings)
{
	struct beauty_filter *filter = data;
	struct beauty_preset_values preset_values = {0};
	const int preset = (int)obs_data_get_int(settings, "preset");
	const int quality_mode = (int)obs_data_get_int(settings, "quality_mode");

	filter->enabled = obs_data_get_bool(settings, "enabled");
	if (beauty_preset_values_for(preset, &preset_values)) {
		filter->smoothing = preset_values.smoothing / 100.0f;
		filter->detail = preset_values.detail / 100.0f;
		filter->brighten = preset_values.brighten / 100.0f;
		filter->rosy = preset_values.rosy / 100.0f;
		filter->sharpness = preset_values.sharpness / 100.0f;
	} else {
		filter->smoothing = (float)obs_data_get_double(settings, "smoothing") / 100.0f;
		filter->detail = (float)obs_data_get_double(settings, "detail") / 100.0f;
		filter->brighten = (float)obs_data_get_double(settings, "brighten") / 100.0f;
		filter->rosy = (float)obs_data_get_double(settings, "rosy") / 100.0f;
		filter->sharpness = (float)obs_data_get_double(settings, "sharpness") / 100.0f;
	}

	/* P0 uses this only to select the shader sample radius. */
	switch (quality_mode) {
	case 1:
		filter->quality_scale = 0.75f;
		break;
	case 2:
		filter->quality_scale = 1.25f;
		break;
	default:
		filter->quality_scale = 1.0f;
		break;
	}
}

static void beauty_filter_store_preset(obs_data_t *settings, const struct beauty_preset_values *values)
{
	obs_data_set_double(settings, "smoothing", values->smoothing);
	obs_data_set_double(settings, "detail", values->detail);
	obs_data_set_double(settings, "brighten", values->brighten);
	obs_data_set_double(settings, "rosy", values->rosy);
	obs_data_set_double(settings, "sharpness", values->sharpness);
}

static bool beauty_filter_preset_modified(obs_properties_t *props, obs_property_t *property,
						   obs_data_t *settings)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
	struct beauty_preset_values values = {0};
	if (beauty_preset_values_for((int)obs_data_get_int(settings, "preset"), &values))
		beauty_filter_store_preset(settings, &values);
	return true;
}

static bool beauty_filter_manual_setting_modified(obs_properties_t *props, obs_property_t *property,
							  obs_data_t *settings)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
	obs_data_set_int(settings, "preset", BEAUTY_PRESET_CUSTOM);
	return true;
}

static void beauty_filter_destroy(void *data)
{
	struct beauty_filter *filter = data;

	#ifdef OBS_BEAUTY_ENABLE_MEDIAPIPE
	beauty_face_inference_worker_destroy(filter->inference_worker);
	filter->inference_worker = NULL;
	#endif

	if (filter->effect) {
		obs_enter_graphics();
		#ifdef OBS_BEAUTY_ENABLE_MEDIAPIPE
		beauty_frame_bridge_destroy(filter->frame_bridge);
		gs_texrender_destroy(filter->source_render);
		#endif
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	bfree(filter);
}

static void *beauty_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct beauty_filter *filter = bzalloc(sizeof(*filter));
	char *effect_path = obs_module_file("beauty.effect");

	filter->context = context;

	obs_enter_graphics();
	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	if (filter->effect) {
		filter->smoothing_param = gs_effect_get_param_by_name(filter->effect, "smoothing");
		filter->detail_param = gs_effect_get_param_by_name(filter->effect, "detail");
		filter->brighten_param = gs_effect_get_param_by_name(filter->effect, "brighten");
		filter->rosy_param = gs_effect_get_param_by_name(filter->effect, "rosy");
		filter->sharpness_param = gs_effect_get_param_by_name(filter->effect, "sharpness");
		filter->texture_width_param = gs_effect_get_param_by_name(filter->effect, "texture_width");
		filter->texture_height_param = gs_effect_get_param_by_name(filter->effect, "texture_height");
		filter->image_param = gs_effect_get_param_by_name(filter->effect, "image");
		filter->face_mask_enabled_param =
			gs_effect_get_param_by_name(filter->effect, "face_mask_enabled");
		for (size_t index = 0; index < BEAUTY_MAX_FACES; ++index) {
			char parameter_name[16] = {0};
			snprintf(parameter_name, sizeof(parameter_name), "face%zu", index);
			filter->face_params[index] =
				gs_effect_get_param_by_name(filter->effect, parameter_name);
			snprintf(parameter_name, sizeof(parameter_name), "eyes%zu", index);
			filter->eyes_params[index] =
				gs_effect_get_param_by_name(filter->effect, parameter_name);
			snprintf(parameter_name, sizeof(parameter_name), "mouth%zu", index);
			filter->mouth_params[index] =
				gs_effect_get_param_by_name(filter->effect, parameter_name);
		}
	}
	obs_leave_graphics();
	bfree(effect_path);

	if (!filter->effect) {
		beauty_filter_destroy(filter);
		return NULL;
	}

	#ifdef OBS_BEAUTY_ENABLE_MEDIAPIPE
	char *model_path = obs_module_file("face_landmarker.task");
	char error[512] = {0};
	filter->inference_worker = beauty_face_inference_worker_create(
		model_path, BEAUTY_MAX_FACES, 0.35f, UINT64_C(500000000), error, sizeof(error));
	bfree(model_path);
	if (!filter->inference_worker) {
		blog(LOG_WARNING, "[obs-beauty-filter] face inference disabled: %s", error);
	} else {
		filter->frame_bridge = beauty_frame_bridge_create(filter->inference_worker, 256,
									  UINT64_C(100000000));
	}
	#endif

	beauty_filter_update(filter, settings);
	return filter;
}

static void beauty_filter_set_face_mask_disabled(struct beauty_filter *filter)
{
	gs_effect_set_float(filter->face_mask_enabled_param, 0.0f);
}

#ifdef OBS_BEAUTY_ENABLE_MEDIAPIPE
static void beauty_filter_set_face_mask(struct beauty_filter *filter, uint64_t now_ns)
{
	struct beauty_face_track tracks[BEAUTY_MAX_FACES] = {0};
	beauty_face_inference_worker_copy_tracks(filter->inference_worker, tracks,
							BEAUTY_MAX_FACES, now_ns);
	gs_effect_set_float(filter->face_mask_enabled_param, 1.0f);
	for (size_t index = 0; index < BEAUTY_MAX_FACES; ++index) {
		struct vec4 face = {0};
		struct vec4 eyes = {0};
		struct vec4 mouth = {0};
		if (tracks[index].active) {
			const struct beauty_face_observation *state = &tracks[index].state;
			vec4_set(&face, state->center_x, state->center_y, state->radius_x,
				 state->radius_y);
			vec4_set(&eyes, state->left_eye_x, state->left_eye_y, state->right_eye_x,
				 state->right_eye_y);
			vec4_set(&mouth, state->mouth_x, state->mouth_y, state->radius_x * 0.34f,
				 state->radius_y * 0.18f);
		}
		gs_effect_set_vec4(filter->face_params[index], &face);
		gs_effect_set_vec4(filter->eyes_params[index], &eyes);
		gs_effect_set_vec4(filter->mouth_params[index], &mouth);
	}
}

static gs_texture_t *beauty_filter_render_input(struct beauty_filter *filter, obs_source_t *target,
								 uint32_t width, uint32_t height,
								 enum gs_color_space color_space)
{
	if (!filter->source_render)
		filter->source_render = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	if (!filter->source_render ||
	    !gs_texrender_begin_with_color_space(filter->source_render, width, height, color_space))
		return NULL;

	struct vec4 clear_color;
	vec4_zero(&clear_color);
	gs_blend_state_push();
	gs_blend_function_separate(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, GS_BLEND_ONE,
				   GS_BLEND_INVSRCALPHA);
	gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
	obs_source_video_render(target);
	gs_blend_state_pop();
	gs_texrender_end(filter->source_render);
	return gs_texrender_get_texture(filter->source_render);
}
#endif

static enum gs_color_space beauty_filter_color_space(void *data, size_t count,
							      const enum gs_color_space *preferred_spaces)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(preferred_spaces);

	return GS_CS_SRGB;
}

static void beauty_filter_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	struct beauty_filter *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);

	if (!filter->enabled || !target) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	const enum gs_color_space preferred_spaces[] = {GS_CS_SRGB, GS_CS_SRGB_16F};
	const enum gs_color_space source_space =
		obs_source_get_color_space(target, OBS_COUNTOF(preferred_spaces), preferred_spaces);

	if (source_space == GS_CS_709_EXTENDED) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	const enum gs_color_format format = gs_get_format_from_space(source_space);
	if (!obs_source_process_filter_begin_with_color_space(filter->context, format, source_space,
								      OBS_ALLOW_DIRECT_RENDERING)) {
		return;
	}

	const float width = (float)obs_source_get_width(target);
	const float height = (float)obs_source_get_height(target);
	if (width < 1.0f || height < 1.0f) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	#ifdef OBS_BEAUTY_ENABLE_MEDIAPIPE
	if (filter->frame_bridge) {
		gs_texture_t *input = beauty_filter_render_input(filter, target, (uint32_t)width,
									 (uint32_t)height, source_space);
		if (!input) {
			obs_source_skip_video_filter(filter->context);
			return;
		}
		const uint64_t now_ns = os_gettime_ns();
		beauty_frame_bridge_tick(filter->frame_bridge, input, now_ns);
		beauty_filter_set_face_mask(filter, now_ns);
		const bool previous_linear = gs_set_linear_srgb(true);
		const bool previous_framebuffer_srgb = gs_framebuffer_srgb_enabled();
		gs_enable_framebuffer_srgb(gs_get_linear_srgb());
		if (gs_get_linear_srgb())
			gs_effect_set_texture_srgb(filter->image_param, input);
		else
			gs_effect_set_texture(filter->image_param, input);
		gs_effect_set_float(filter->smoothing_param, filter->smoothing * filter->quality_scale);
		gs_effect_set_float(filter->detail_param, filter->detail);
		gs_effect_set_float(filter->brighten_param, filter->brighten);
		gs_effect_set_float(filter->rosy_param, filter->rosy);
		gs_effect_set_float(filter->sharpness_param, filter->sharpness);
		gs_effect_set_float(filter->texture_width_param, width);
		gs_effect_set_float(filter->texture_height_param, height);
		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
		while (gs_effect_loop(filter->effect, "Draw"))
			gs_draw_sprite(input, 0, (uint32_t)width, (uint32_t)height);
		gs_blend_state_pop();
		gs_enable_framebuffer_srgb(previous_framebuffer_srgb);
		gs_set_linear_srgb(previous_linear);
		return;
	}
	#endif

	gs_effect_set_float(filter->smoothing_param, filter->smoothing * filter->quality_scale);
	beauty_filter_set_face_mask_disabled(filter);
	gs_effect_set_float(filter->detail_param, filter->detail);
	gs_effect_set_float(filter->brighten_param, filter->brighten);
	gs_effect_set_float(filter->rosy_param, filter->rosy);
	gs_effect_set_float(filter->sharpness_param, filter->sharpness);
	gs_effect_set_float(filter->texture_width_param, width);
	gs_effect_set_float(filter->texture_height_param, height);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);
	gs_blend_state_pop();
}

static obs_properties_t *beauty_filter_properties(void *data)
{
	UNUSED_PARAMETER(data);
	obs_properties_t *props = obs_properties_create();
	obs_property_t *quality = NULL;

	obs_properties_add_bool(props, "enabled", obs_module_text("Filter.Enabled"));
	obs_property_t *preset = obs_properties_add_list(props, "preset", obs_module_text("Filter.Preset"),
								     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(preset, obs_module_text("Preset.Natural"), BEAUTY_PRESET_NATURAL);
	obs_property_list_add_int(preset, obs_module_text("Preset.Clear"), BEAUTY_PRESET_CLEAR);
	obs_property_list_add_int(preset, obs_module_text("Preset.Live"), BEAUTY_PRESET_LIVE);
	obs_property_list_add_int(preset, obs_module_text("Preset.Custom"), BEAUTY_PRESET_CUSTOM);
	obs_property_set_modified_callback(preset, beauty_filter_preset_modified);
	quality = obs_properties_add_list(props, "quality_mode", obs_module_text("Filter.Quality"),
					  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(quality, obs_module_text("Quality.Auto"), 0);
	obs_property_list_add_int(quality, obs_module_text("Quality.Compatible"), 1);
	obs_property_list_add_int(quality, obs_module_text("Quality.High"), 2);

	obs_property_t *manual = obs_properties_add_float_slider(
		props, "smoothing", obs_module_text("Filter.Smoothing"), 0.0, 100.0, 1.0);
	obs_property_set_modified_callback(manual, beauty_filter_manual_setting_modified);
	manual = obs_properties_add_float_slider(props, "detail", obs_module_text("Filter.Detail"), 0.0,
						    100.0, 1.0);
	obs_property_set_modified_callback(manual, beauty_filter_manual_setting_modified);
	manual = obs_properties_add_float_slider(props, "brighten", obs_module_text("Filter.Brighten"),
						    0.0, 100.0, 1.0);
	obs_property_set_modified_callback(manual, beauty_filter_manual_setting_modified);
	manual = obs_properties_add_float_slider(props, "rosy", obs_module_text("Filter.Rosy"), 0.0,
						    100.0, 1.0);
	obs_property_set_modified_callback(manual, beauty_filter_manual_setting_modified);
	manual = obs_properties_add_float_slider(props, "sharpness", obs_module_text("Filter.Sharpness"),
						    0.0, 100.0, 1.0);
	obs_property_set_modified_callback(manual, beauty_filter_manual_setting_modified);
	return props;
}

static void beauty_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "enabled", true);
	obs_data_set_default_int(settings, "preset", BEAUTY_PRESET_NATURAL);
	obs_data_set_default_int(settings, "quality_mode", 0);
	obs_data_set_default_double(settings, "smoothing", 35.0);
	obs_data_set_default_double(settings, "detail", 70.0);
	obs_data_set_default_double(settings, "brighten", 15.0);
	obs_data_set_default_double(settings, "rosy", 10.0);
	obs_data_set_default_double(settings, "sharpness", 10.0);
}

static struct obs_source_info beauty_filter_info = {
	.id = "obs_beauty_filter",
	.version = 2,
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = beauty_filter_name,
	.create = beauty_filter_create,
	.destroy = beauty_filter_destroy,
	.update = beauty_filter_update,
	.video_render = beauty_filter_render,
	.video_get_color_space = beauty_filter_color_space,
	.get_properties = beauty_filter_properties,
	.get_defaults = beauty_filter_defaults,
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-beauty-filter", "zh-CN")

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Module.Description");
}

bool obs_module_load(void)
{
	obs_register_source(&beauty_filter_info);
	blog(LOG_INFO, "[obs-beauty-filter] P0 shader filter loaded");
	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[obs-beauty-filter] unloaded");
}
