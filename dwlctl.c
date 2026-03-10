#include "river-control-unstable-v1-client-protocol.h"
#include "dwl-ipc-unstable-v2-client-protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-util.h>

struct wl_display *wl_display;
struct wl_registry *wl_registry;
struct wl_callback *sync_callback;
struct zriver_control_v1 *zriver_ctl = NULL;
struct zdwl_ipc_manager_v2 *ipc_manager = NULL;
struct zriver_command_callback_v1 *zriver_callback = NULL;
struct wl_seat *seat = NULL;

struct output_t {
    struct wl_output *wl_output;
    struct zdwl_ipc_output_v2 *ipc_output;
    struct wl_list link;
    uint32_t id;
    char *name;
    bool active;
};

struct dwlctl_command {
    const char *name;
    const char *args;
    const char *description;
};

static const struct dwlctl_command commands[] = {
    // IPC Commands
    { "tags", "<mask> [toggle]", "Set the active tags of the output" },
    { "client_tags", "<and_mask> <xor_mask>", "Set the tags of the focused client" },
    { "ipc_layout", "<index>", "Set the layout of the output via IPC" },
    // River / Window Manager Commands
    { "clear-binds", "", "Clear all keybindings" },
    { "clear-rules", "", "Clear all window rules" },
    { "enter-mode", "<mode>", "Enter a specific keybinding mode" },
    { "oneshot-mode", "<mode> <return_mode>", "Enter a mode for a single keypress, then return" },
    { "create-mode", "<mode>", "Create a new keybinding mode" },
    { "setfocuscolor", "<color>", "Set the color of the focused window border" },
    { "setbordercolor", "<color>", "Set the color of unfocused window borders" },
    { "seturgentcolor", "<color>", "Set the color of urgent window borders" },
    { "setborderpx", "<pixels>", "Set the border width in pixels" },
    { "setlayout", "<index>", "Set the current layout by index" },
    { "spawn", "<command...>", "Spawn a shell command" },
    { "focusstack", "<direction>", "Change focus in the stack (e.g. 1 or -1)" },
    { "setmfact", "<float>", "Set the master area size factor (e.g. 0.5)" },
    { "zoom", "", "Move focused window to the master area" },
    { "killclient", "", "Kill the focused client" },
    { "incnmaster", "<int>", "Increase/decrease the number of master windows" },
    { "togglefloating", "", "Toggle floating state of the focused window" },
    { "togglefullscreen", "", "Toggle fullscreen state of the focused window" },
    { "view", "<mask>", "View a specific tag mask" },
    { "toggleview", "<mask>", "Toggle viewing of a specific tag mask" },
    { "tagmon", "<direction>", "Move focused window to another monitor" },
    { "focusmon", "<direction>", "Move focus to another monitor" },
    { "tag", "<mask>", "Apply tag mask to focused window" },
    { "toggletag", "<mask>", "Toggle tag mask on focused window" },
    { "quit", "", "Quit the compositor" },
    // Opacity Commands
    { "set-opacity-focus", "<float>", "Set the opacity for focused windows" },
    { "set-opacity-unfocus", "<float>", "Set the opacity for unfocused windows" },
    { "toggle-opacity", "", "Toggle global opacity state" },
    // Info Commands
    { "get", "<tags|clients|master>", "Get information about tags, clients, or everything" },
    { "query", "<tag|display|client> <id>", "Get specific information about a tag, display, or client" },
};

struct wl_list outputs;
bool loop = true;
bool monitor_mode = false;
char **cmd_argv;
int cmd_argc;

static void print_help() {
    printf("Usage: dwlctl [options] [command] [args...]\n\n");
    printf("Options:\n");
    printf("  -h, --help    Show this help message and exit\n");
    printf("  -m            Run in monitor mode, printing IPC events to stdout\n\n");
    printf("Commands:\n");
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        printf("  %-20s %-25s %s\n", commands[i].name, commands[i].args, commands[i].description);
    }
}

static void callback_failure(void *data, struct zriver_command_callback_v1 *cb, const char *msg) {
    if (msg) printf("error: %s\n", msg);
    zriver_command_callback_v1_destroy(cb);
    zriver_control_v1_destroy(zriver_ctl);
    zriver_ctl = NULL;
    loop = false;
}

static void callback_success(void *data, struct zriver_command_callback_v1 *cb, const char *output) {
    if (output && output[0] != '\0') printf("%s\n", output);
    zriver_command_callback_v1_destroy(cb);
    loop = false;
}

struct zriver_command_callback_v1_listener zriver_callback_listener = {
    .success = callback_success,
    .failure = callback_failure,
};

static void ipc_output_toggle_visibility(void *data, struct zdwl_ipc_output_v2 *out) {}
static void ipc_output_active(void *data, struct zdwl_ipc_output_v2 *out, uint32_t active) {
    if (monitor_mode) printf("output %p active: %u\n", data, active);
    ((struct output_t*)data)->active = active;
}
static void ipc_output_tag(void *data, struct zdwl_ipc_output_v2 *out, uint32_t tag, uint32_t state, uint32_t clients, uint32_t focused) {
    if (monitor_mode) printf("output %p tag: %u state: %u clients: %u focused: %u\n", data, tag, state, clients, focused);
}
static void ipc_output_layout(void *data, struct zdwl_ipc_output_v2 *out, uint32_t layout) {
    if (monitor_mode) printf("output %p layout: %u\n", data, layout);
}
static void ipc_output_title(void *data, struct zdwl_ipc_output_v2 *out, const char *title) {
    if (monitor_mode) printf("output %p title: %s\n", data, title);
}
static void ipc_output_appid(void *data, struct zdwl_ipc_output_v2 *out, const char *appid) {
    if (monitor_mode) printf("output %p appid: %s\n", data, appid);
}
static void ipc_output_layout_symbol(void *data, struct zdwl_ipc_output_v2 *out, const char *layout) {
    if (monitor_mode) printf("output %p layout_symbol: %s\n", data, layout);
}
static void ipc_output_frame(void *data, struct zdwl_ipc_output_v2 *out) {
    if (monitor_mode) {
        printf("output %p frame\n", data);
        fflush(stdout);
    }
}
static void ipc_output_fullscreen(void *data, struct zdwl_ipc_output_v2 *out, uint32_t is_fullscreen) {
    if (monitor_mode) printf("output %p fullscreen: %u\n", data, is_fullscreen);
}
static void ipc_output_floating(void *data, struct zdwl_ipc_output_v2 *out, uint32_t is_floating) {
    if (monitor_mode) printf("output %p floating: %u\n", data, is_floating);
}

struct zdwl_ipc_output_v2_listener ipc_output_listener = {
    .toggle_visibility = ipc_output_toggle_visibility,
    .active = ipc_output_active,
    .tag = ipc_output_tag,
    .layout = ipc_output_layout,
    .title = ipc_output_title,
    .appid = ipc_output_appid,
    .layout_symbol = ipc_output_layout_symbol,
    .frame = ipc_output_frame,
    .fullscreen = ipc_output_fullscreen,
    .floating = ipc_output_floating,
};

static void ipc_manager_tags(void *data, struct zdwl_ipc_manager_v2 *mgr, uint32_t amount) {
    if (monitor_mode) printf("manager tags amount: %u\n", amount);
}
static void ipc_manager_layout(void *data, struct zdwl_ipc_manager_v2 *mgr, const char *name) {
    if (monitor_mode) printf("manager layout: %s\n", name);
}

struct zdwl_ipc_manager_v2_listener ipc_manager_listener = {
    .tags = ipc_manager_tags,
    .layout = ipc_manager_layout,
};

static void wl_output_geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel_routing, const char *make, const char *model, int32_t transform) {}
static void wl_output_mode(void *data, struct wl_output *wl_output, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {}
static void wl_output_done(void *data, struct wl_output *wl_output) {}
static void wl_output_scale(void *data, struct wl_output *wl_output, int32_t factor) {}
static void wl_output_name(void *data, struct wl_output *wl_output, const char *name) {
    struct output_t *out = data;
    out->name = strdup(name);
}
static void wl_output_description(void *data, struct wl_output *wl_output, const char *description) {}

struct wl_output_listener output_listener = {
    .geometry = wl_output_geometry,
    .mode = wl_output_mode,
    .done = wl_output_done,
    .scale = wl_output_scale,
    .name = wl_output_name,
    .description = wl_output_description,
};

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    if (strcmp(interface, zriver_control_v1_interface.name) == 0) {
        zriver_ctl = wl_registry_bind(registry, name, &zriver_control_v1_interface, 1);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
    } else if (strcmp(interface, zdwl_ipc_manager_v2_interface.name) == 0) {
        ipc_manager = wl_registry_bind(registry, name, &zdwl_ipc_manager_v2_interface, 2);
        zdwl_ipc_manager_v2_add_listener(ipc_manager, &ipc_manager_listener, NULL);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        struct output_t *out = calloc(1, sizeof(struct output_t));
        out->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 4);
        out->id = name;
        wl_output_add_listener(out->wl_output, &output_listener, out);
        wl_list_insert(&outputs, &out->link);
    }
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
    struct output_t *out, *tmp;
    wl_list_for_each_safe(out, tmp, &outputs, link) {
        if (out->id == name) {
            wl_list_remove(&out->link);
            if (out->ipc_output) zdwl_ipc_output_v2_destroy(out->ipc_output);
            wl_output_destroy(out->wl_output);
            free(out->name);
            free(out);
            break;
        }
    }
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove
};

static void run_command() {
    if (monitor_mode) {
        return; // Just keep looping
    }

    if (cmd_argc > 1) {
        if (strcmp(cmd_argv[1], "tags") == 0 || strcmp(cmd_argv[1], "client_tags") == 0 || strcmp(cmd_argv[1], "ipc_layout") == 0) {
            if (!ipc_manager) {
                fprintf(stderr, "IPC manager not available\n");
                loop = false;
                return;
            }
            // Send command to all active outputs, or first if none active
            struct output_t *target = NULL, *out;
            wl_list_for_each(out, &outputs, link) {
                if (out->active) { target = out; break; }
            }
            if (!target && !wl_list_empty(&outputs)) {
                target = wl_container_of(outputs.next, target, link);
            }

            if (!target || !target->ipc_output) {
                fprintf(stderr, "No IPC output available\n");
                loop = false;
                return;
            }

            if (strcmp(cmd_argv[1], "tags") == 0) {
                uint32_t mask = cmd_argc > 2 ? atoi(cmd_argv[2]) : 0;
                uint32_t toggle = cmd_argc > 3 ? atoi(cmd_argv[3]) : 0;
                zdwl_ipc_output_v2_set_tags(target->ipc_output, mask, toggle);
            } else if (strcmp(cmd_argv[1], "client_tags") == 0) {
                uint32_t and_t = cmd_argc > 2 ? atoi(cmd_argv[2]) : 0;
                uint32_t xor_t = cmd_argc > 3 ? atoi(cmd_argv[3]) : 0;
                zdwl_ipc_output_v2_set_client_tags(target->ipc_output, and_t, xor_t);
            } else if (strcmp(cmd_argv[1], "ipc_layout") == 0) {
                uint32_t idx = cmd_argc > 2 ? atoi(cmd_argv[2]) : 0;
                zdwl_ipc_output_v2_set_layout(target->ipc_output, idx);
            }
            wl_display_roundtrip(wl_display);
            loop = false;
            return;
        }
    }

    if (zriver_ctl && seat) {
        for (int i = 1; i < cmd_argc; i++) {
            zriver_control_v1_add_argument(zriver_ctl, cmd_argv[i]);
        }
        zriver_callback = zriver_control_v1_run_command(zriver_ctl, seat);
        zriver_command_callback_v1_add_listener(zriver_callback, &zriver_callback_listener, NULL);
    } else {
        fprintf(stderr, "River control not available\n");
        loop = false;
    }
}

static void sync_handle_done(void *data, struct wl_callback *cb, uint32_t time) {
    wl_callback_destroy(cb);
    sync_callback = NULL;

    // Now bind IPC outputs for all wl_outputs
    if (ipc_manager) {
        struct output_t *out;
        wl_list_for_each(out, &outputs, link) {
            out->ipc_output = zdwl_ipc_manager_v2_get_output(ipc_manager, out->wl_output);
            zdwl_ipc_output_v2_add_listener(out->ipc_output, &ipc_output_listener, out);
        }
        // Need one more roundtrip to get active state before running command
        wl_display_roundtrip(wl_display);
    }

    run_command();
}

static const struct wl_callback_listener sync_callback_listener = {
    .done = sync_handle_done,
};

static bool init_wayland(void) {
    const char *display_name = getenv("WAYLAND_DISPLAY");
    if (!display_name) {
        fputs("WAYLAND_DISPLAY is not set.\n", stderr);
        return false;
    }

    wl_display = wl_display_connect(display_name);
    if (!wl_display) {
        fputs("Can not connect to Wayland server.\n", stderr);
        return false;
    }

    wl_list_init(&outputs);
    wl_registry = wl_display_get_registry(wl_display);
    wl_registry_add_listener(wl_registry, &registry_listener, NULL);

    sync_callback = wl_display_sync(wl_display);
    wl_callback_add_listener(sync_callback, &sync_callback_listener, NULL);

    return true;
}

int main(int argc_local, char *argv_local[]) {
    cmd_argc = argc_local;
    cmd_argv = argv_local;

    if (cmd_argc > 1 && (strcmp(cmd_argv[1], "-h") == 0 || strcmp(cmd_argv[1], "--help") == 0)) {
        print_help();
        return 0;
    }

    if (cmd_argc == 1 || (cmd_argc == 2 && strcmp(cmd_argv[1], "-m") == 0)) {
        monitor_mode = true;
    }

    if (init_wayland()) {
        while (loop && wl_display_dispatch(wl_display) != -1) {
        }
    }

    return 0;
}
