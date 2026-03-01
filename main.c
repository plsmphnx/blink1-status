#define USE_HIDAPI 1

#include <astal-wp.h>
#include <blink1-lib.h>
#include <glib-object.h>
#include <glib-unix.h>

typedef struct {
    uint recorders;
    blink1_device *device;
    uint brightness;
} State;

void set_rgb(State *state, uint r, uint g, uint b) {
    blink1_setRGB(
        state->device,
        state->brightness * r,
        state->brightness * g,
        state->brightness * b
    );
}

void set_state(State *state) {
    if (state->recorders > 0)
        set_rgb(state, 4, 1, 1);
    else
        set_rgb(state, 0, 3, 0);
}

void on_recorder_added(AstalWpAudio*, AstalWpStream*, gpointer data) {
    State *state = (State*)data;
    state->recorders++;
    set_state(state);
}

void on_recorder_removed(AstalWpAudio*, AstalWpStream*, gpointer data) {
    State *state = (State*)data;
    state->recorders--;
    set_state(state);
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

    State state = {0, blink1_open(), 51};

    GList *list = astal_wp_audio_get_recorders(audio);
    if (list != NULL) {
        state.recorders = g_list_length(list);
        g_object_unref(list);
    }

    if (argc > 1) state.brightness = atoi(argv[1]);

    set_state(&state);

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
