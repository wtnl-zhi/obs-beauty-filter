#include <obs-module.h>
#include <obs-properties.h>

#include <stdio.h>
#include <string.h>

static int fail(const char *message)
{
	fprintf(stderr, "%s\n", message);
	return 1;
}

int main(int argc, char **argv)
{
	obs_module_t *module = NULL;
	obs_properties_t *properties = NULL;
	obs_property_t *preset = NULL;
	int result = 1;

	if (argc != 3)
		return fail("usage: obs-module-smoke-test <module-binary> <resource-directory>");
	if (!obs_startup("zh-CN", NULL, NULL))
		return fail("obs_startup failed");
	if (obs_open_module(&module, argv[1], argv[2]) != MODULE_SUCCESS) {
		result = fail("obs_open_module failed");
		goto cleanup;
	}
	if (!obs_init_module(module)) {
		result = fail("obs_init_module failed");
		goto cleanup;
	}

	/* The plugin registers source-info version 2, so libobs exposes the
	 * versioned ID.  Keep this assertion aligned with OBS's registration API. */
	properties = obs_get_source_properties("obs_beauty_filter_v2");
	if (!properties) {
		result = fail("obs_beauty_filter source type was not registered");
		goto cleanup;
	}
	preset = obs_properties_get(properties, "preset");
	if (!preset ||
	    !obs_properties_get(properties, "quality_mode") ||
	    !obs_properties_get(properties, "show_mask")) {
		result = fail("obs_beauty_filter properties are incomplete");
		goto cleanup;
	}
	if (strcmp(obs_property_description(preset), "预设") != 0) {
		result = fail("zh-CN plugin locale was not loaded");
		goto cleanup;
	}
	result = 0;

cleanup:
	obs_properties_destroy(properties);
	obs_shutdown();
	return result;
}
