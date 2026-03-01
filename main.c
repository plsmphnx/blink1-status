#define USE_HIDAPI 1

#include <astal-wp.h>
#include <blink1-lib.h>
#include <glib-object.h>
#include <glib-unix.h>

typedef struct {
    guint count;
    blink1_device *device;
} State;

void recording_on(blink1_device *device) {
    blink1_setRGB(device, 136, 34, 34);
}

void recording_off(blink1_device *device) {
    blink1_setRGB(device, 0, 102, 0);
}

void on_recorder_added(AstalWpAudio*, AstalWpStream*, gpointer data) {
    State *state = (State*)data;
    if (!state->count++) recording_on(state->device);
}

void on_recorder_removed(AstalWpAudio*, AstalWpStream*, gpointer data) {
    State *state = (State*)data;
    if (!--state->count) recording_off(state->device);
}

gboolean on_signal(gpointer data) {
    g_main_loop_quit((GMainLoop*)data);
    return G_SOURCE_REMOVE;
}

void wait_for_signal() {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    g_unix_signal_add(SIGTERM, on_signal, loop);
    g_unix_signal_add(SIGINT, on_signal, loop);

    g_main_loop_run(loop);

    g_main_loop_unref(loop);
}

int main(int argc, char *argv[]) {
    AstalWpWp *wp = astal_wp_wp_get_default();
    AstalWpAudio *audio = astal_wp_wp_get_audio(wp);
    GList *list = astal_wp_audio_get_recorders(audio);

    State state = {0, blink1_open()};
    if (list != NULL) {
        state.count = g_list_length(list);
        g_object_unref(list);
    }
    if (state.count > 0)
        recording_on(state.device);
    else
        recording_off(state.device);

    gulong recorder_added_handle = g_signal_connect(
        audio,
        "recorder-added",
        G_CALLBACK(on_recorder_added),
        &state
    );
    gulong recorder_removed_handle = g_signal_connect(
        audio,
        "recorder-removed",
        G_CALLBACK(on_recorder_removed),
        &state
    );

    wait_for_signal();

    blink1_setRGB(state.device, 0, 0, 0);

    g_signal_handler_disconnect(audio, recorder_removed_handle);
    g_signal_handler_disconnect(audio, recorder_added_handle);
    blink1_close(state.device);
    g_object_unref(audio);
    g_object_unref(wp);
}
