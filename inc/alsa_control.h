#pragma once
#include <pipewire/pipewire.h>
#include <stdbool.h>
bool alsa_control_init(struct pw_loop *loop);
void alsa_control_cleanup(void);

bool alsa_control_set_db(long volumeDB, bool external);
bool alsa_control_get_db(long *volumeDB);
bool alsa_control_toggle_mute(void);

typedef void (*alsa_control_event_callback_t)(void *userdata);
void alsa_control_set_event_callback(alsa_control_event_callback_t callback,
				     void *userdata);
