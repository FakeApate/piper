// SPDX-License-Identifier: MIT
/*
 * piper - PipeWire to ALSA volume sync
 * 
 * Copyright (c) 2025 Samuel Imboden
 */
 
#include "alsa_control.h"

#include <alsa/asoundlib.h>
#include <stdio.h>

static snd_mixer_t *handle = nullptr;
static snd_mixer_elem_t *elem = nullptr;
static long dbMin = 0;
static long dbMax = 0;
static long volume_offset_milli_db = 0;

static struct spa_source *alsa_source = nullptr;
static struct pw_loop *pw_loop_ctx = nullptr;

static alsa_control_event_callback_t event_cb = nullptr;
static void *event_cb_userdata = nullptr;

static void on_alsa_io(void * /*unused*/, int /*unused*/, uint32_t /*unused*/)
{
	if (!handle) {
		return;
	}

	snd_mixer_handle_events(handle);

	if (event_cb) {
		event_cb(event_cb_userdata);
	}
}

bool alsa_control_init(struct pw_loop *loop)
{
	if (snd_mixer_open(&handle, 0) < 0) {
		perror("snd_mixer_open");
		return false;
	}

	if (snd_mixer_attach(handle, "default") < 0 ||
	    snd_mixer_selem_register(handle, nullptr, nullptr) < 0 ||
	    snd_mixer_load(handle) < 0) {
		perror("snd_mixer_attach/load");
		snd_mixer_close(handle);
		handle = nullptr;
		return false;
	}

	snd_mixer_selem_id_t *sid = nullptr;
	snd_mixer_selem_id_malloc(&sid);
	snd_mixer_selem_id_set_index(sid, 1);
	snd_mixer_selem_id_set_name(sid, "PCM");

	elem = snd_mixer_find_selem(handle, sid);

	if (elem == nullptr) {
		(void)fprintf(stderr, "Could not find ALSA mixer control \n");
		snd_mixer_close(handle);
		handle = nullptr;
		return false;
	}

	if (snd_mixer_selem_get_playback_dB_range(elem, &dbMin, &dbMax) < 0) {
		perror("snd_mixer_selem_get_playback_dB_range");
		snd_mixer_close(handle);
		handle = nullptr;
		return false;
	}

	printf("ALSA mixer found: dB range %ld - %ld\n", dbMin, dbMax);

	pw_loop_ctx = loop;

	struct pollfd pfd;
	if (snd_mixer_poll_descriptors(handle, &pfd, 1) != 1) {
		(void)fprintf(stderr, "Failed to get ALSA poll descriptor\n");
		alsa_control_cleanup();
		return false;
	}

	alsa_source = pw_loop_add_io(loop, pfd.fd, SPA_IO_IN, false,
				     (spa_source_io_func_t)on_alsa_io, NULL);
	if (!alsa_source) {
		perror("pw_loop_add_io (alsa)");
		alsa_control_cleanup();
		return false;
	}

	return true;
}

void alsa_control_cleanup(void)
{
	if (alsa_source) {
		pw_loop_destroy_source(pw_loop_ctx, alsa_source);
		alsa_source = nullptr;
	}
	if (handle) {
		snd_mixer_close(handle);
		handle = nullptr;
		elem = nullptr;
	}
}

void alsa_control_set_event_callback(alsa_control_event_callback_t callback,
				     void *userdata)
{
	event_cb = callback;
	event_cb_userdata = userdata;
}

bool alsa_control_set_db(long volumeDB, bool external)
{
	if (!handle || !elem) {
		return false;
	}

	if (volumeDB < dbMin) {
		volumeDB = dbMin;
	}
	if (volumeDB > dbMax) {
		volumeDB = dbMax;
	}
	long before = 0;
	long after = 0;
	alsa_control_get_db(&before);
	printf("Set vol: %ld\n", volumeDB);
	if (snd_mixer_selem_set_playback_dB_all(elem, volumeDB, 0) < 0) {
		perror("snd_mixer_selem_set_playback_dB_all");
		return false;
	}

	if (external) {
		volume_offset_milli_db = 0;
	} else {
		alsa_control_get_db(&after);
		if (before != after) {
			volume_offset_milli_db = 0;
		}
	}
	return true;
}

bool alsa_control_get_db(long *volumeDB)
{
	if (!handle || !elem || !volumeDB) {
		return false;
	}

	long vol = 0;
	if (snd_mixer_selem_get_playback_dB(elem, SND_MIXER_SCHN_FRONT_LEFT,
					    &vol) < 0) {
		perror("snd_mixer_selem_get_playback_dB");
		return false;
	}

	*volumeDB = vol;
	return true;
}

bool alsa_control_toggle_mute(void)
{
	if (!handle || !elem) {
		return false;
	}

	int muted = 0;
	if (!snd_mixer_selem_has_playback_switch(elem)) {
		(void)fprintf(stderr,
			      "No playback switch available for mute\n");
		return false;
	}

	if (snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT,
						&muted) < 0) {
		perror("snd_mixer_selem_get_playback_switch");
		return false;
	}

	muted = !muted;

	if (snd_mixer_selem_set_playback_switch_all(elem, muted) < 0) {
		perror("snd_mixer_selem_set_playback_switch_all");
		return false;
	}
	return true;
}
