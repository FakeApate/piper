// SPDX-License-Identifier: MIT
/*
 * piper - PipeWire to ALSA volume sync
 * 
 * Copyright (c) 2025 Samuel Imboden
 */

#include <alsa/asoundlib.h>
#include <math.h>
#include <pipewire/node.h>
#include <pipewire/pipewire.h>
#include <spa/debug/pod.h>
#include <spa/param/props.h>
#include <spa/pod/iter.h>
#include <spa/pod/pod.h>
#include <spa/utils/dict.h>
#include <string.h>

#define NODE_NAME_KEY "node.name"
#define NODE_NAME \
	"alsa_output.usb-EPOS_EPOS_GSX_1000_Speaker_A004550224704125-00.analog-output-surround71"
#define LOGF 20.0F

struct data {
	struct pw_main_loop *loop;
	struct pw_context *context;
	struct pw_core *core;

	struct pw_registry *registry;
	struct spa_hook registry_listener;

	struct pw_node *node;
	struct spa_hook node_listener;
};

static inline float volume_to_dB(float volume)
{
	return (volume <= 0.0F) ? -INFINITY : LOGF * log10f(volume);
}

static void set_alsa_volume(float volume)
{
	snd_mixer_t *handle = nullptr;
	snd_mixer_selem_id_t *sid = nullptr;
	snd_mixer_elem_t *elem = nullptr;

	if (snd_mixer_open(&handle, 0) < 0) {
		return;
	}
	if (snd_mixer_attach(handle, "default") < 0) {
		goto error;
	}
	if (snd_mixer_selem_register(handle, nullptr, nullptr) < 0) {
		goto error;
	}
	if (snd_mixer_load(handle) < 0) {
		goto error;
	}

	snd_mixer_selem_id_malloc(&sid);
	snd_mixer_selem_id_set_index(sid, 1);
	snd_mixer_selem_id_set_name(sid, "PCM");

	elem = snd_mixer_find_selem(handle, sid);
	if (elem) {
		long desiredmB = lroundf(volume * 100.0F);

		long mindB = 0;
		long maxdB = 0;
		snd_mixer_selem_get_playback_dB_range(elem, &mindB, &maxdB);

		if (desiredmB < mindB) {
			desiredmB = mindB;
		}
		if (desiredmB > maxdB) {
			desiredmB = maxdB;
		}
		printf("\tdesiredmB Volume: %ld\n", desiredmB);
		snd_mixer_selem_set_playback_dB_all(elem, desiredmB, 0);
	}

	snd_mixer_selem_id_free(sid);
error:
	snd_mixer_close(handle);
}

static void node_info(void *object, const struct pw_node_info * /*unused*/)
{
	struct data *data = object;

	pw_node_enum_params(data->node, 0, SPA_PARAM_Props, 0, UINT32_MAX,
			    nullptr);
}

void node_param(void * /*unused*/, int /*unused*/, uint32_t paramId,
		uint32_t /*unused*/, uint32_t /*unused*/,
		const struct spa_pod *param)
{
	if (paramId == SPA_PARAM_Props && param != nullptr) {
		struct spa_pod_object *obj = (struct spa_pod_object *)param;
		struct spa_pod_prop *prop = nullptr;

		SPA_POD_OBJECT_FOREACH(obj, prop)
		{
			if (prop->key == SPA_PROP_channelVolumes) {
				uint32_t nValues = 0;
				float *volumes = spa_pod_get_array(&prop->value,
								   &nValues);

				if (volumes != nullptr) {
					float avgVolume = 0.0F;
					for (uint32_t i = 0; i < nValues; i++) {
						avgVolume += volumes[i];
					}
					avgVolume /= (float)nValues;
					float avgdB = volume_to_dB(avgVolume);
					set_alsa_volume(avgdB);
				}
			}
		}
	}
}

static const struct pw_node_events nodeEvents = {
	PW_VERSION_NODE_EVENTS,
	.info = node_info,
	.param = node_param,
};

static void registry_event_global(void *_data, uint32_t eventId,
				  uint32_t /*unused*/, const char *type,
				  uint32_t /*unused*/,
				  const struct spa_dict *dict)
{
	struct data *data = _data;
	const struct spa_dict_item *item = nullptr;

	if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0) {
		item = spa_dict_lookup_item(dict, NODE_NAME_KEY);
		if (item != nullptr && strcmp(NODE_NAME, item->value) == 0) {
			data->node = pw_registry_bind(data->registry, eventId,
						      type, PW_VERSION_NODE, 0);
			pw_node_add_listener(data->node, &data->node_listener,
					     &nodeEvents, data);
			pw_node_enum_params(data->node, 0, SPA_PARAM_Props, 0,
					    UINT32_MAX, nullptr);
		}
	}
}

static const struct pw_registry_events registryEvents = {
	PW_VERSION_REGISTRY_EVENTS,
	.global = registry_event_global,
};

int main(int argc, char *argv[])
{
	struct data data;

	spa_zero(data);

	pw_init(&argc, &argv);

	data.loop = pw_main_loop_new(nullptr /* properties */);
	data.context = pw_context_new(pw_main_loop_get_loop(data.loop),
				      nullptr /* properties */,
				      0 /* user_data size */);

	data.core = pw_context_connect(data.context, nullptr /* properties */,
				       0 /* user_data size */);

	data.registry = pw_core_get_registry(data.core, PW_VERSION_REGISTRY,
					     0 /* user_data size */);

	pw_registry_add_listener(data.registry, &data.registry_listener,
				 &registryEvents, &data);

	pw_main_loop_run(data.loop);

	pw_proxy_destroy((struct pw_proxy *)data.node);
	pw_proxy_destroy((struct pw_proxy *)data.registry);
	pw_core_disconnect(data.core);
	pw_context_destroy(data.context);
	pw_main_loop_destroy(data.loop);

	return 0;
}
