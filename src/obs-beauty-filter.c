/*
 * OBS Beauty Filter
 * Copyright (C) 2026 OBS Beauty Filter contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <obs-module.h>
#include <obs-source.h>
#include <util/platform.h>

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

	bool enabled;
	float smoothing;
	float detail;
	float brighten;
	float rosy;
	float sharpness;
	float quality_scale;
};

static const char *beauty_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Filter.Name");
}

static void beauty_filter_update(void *data, obs_data_t *settings)
{
	struct beauty_filter *filter = data;
	const int quality_mode = (int)obs_data_get_int(settings, "quality_mode");

	filter->enabled = obs_data_get_bool(settings, "enabled");
	filter->smoothing = (float)obs_data_get_double(settings, "smoothing") / 100.0f;
	filter->detail = (float)obs_data_get_double(settings, "detail") / 100.0f;
	filter->brighten = (float)obs_data_get_double(settings, "brighten") / 100.0f;
	filter->rosy = (float)obs_data_get_double(settings, "rosy") / 100.0f;
	filter->sharpness = (float)obs_data_get_double(settings, "sharpness") / 100.0f;

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

static void beauty_filter_destroy(void *data)
{
	struct beauty_filter *filter = data;

	if (filter->effect) {
		obs_enter_graphics();
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
	}
	obs_leave_graphics();
	bfree(effect_path);

	if (!filter->effect) {
		beauty_filter_destroy(filter);
		return NULL;
	}

	beauty_filter_update(filter, settings);
	return filter;
}

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

	gs_effect_set_float(filter->smoothing_param, filter->smoothing * filter->quality_scale);
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
	quality = obs_properties_add_list(props, "quality_mode", obs_module_text("Filter.Quality"),
					  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(quality, obs_module_text("Quality.Auto"), 0);
	obs_property_list_add_int(quality, obs_module_text("Quality.Compatible"), 1);
	obs_property_list_add_int(quality, obs_module_text("Quality.High"), 2);

	obs_properties_add_float_slider(props, "smoothing", obs_module_text("Filter.Smoothing"), 0.0,
						100.0, 1.0);
	obs_properties_add_float_slider(props, "detail", obs_module_text("Filter.Detail"), 0.0, 100.0,
						1.0);
	obs_properties_add_float_slider(props, "brighten", obs_module_text("Filter.Brighten"), 0.0,
						100.0, 1.0);
	obs_properties_add_float_slider(props, "rosy", obs_module_text("Filter.Rosy"), 0.0, 100.0,
						1.0);
	obs_properties_add_float_slider(props, "sharpness", obs_module_text("Filter.Sharpness"), 0.0,
						100.0, 1.0);
	return props;
}

static void beauty_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "enabled", true);
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
