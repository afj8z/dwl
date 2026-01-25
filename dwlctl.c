#include "river-control-unstable-v1-client-protocol.h"
#include "river-control-unstable-v1-protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

struct wl_display *wl_display;
struct wl_registry *wl_registry;
struct wl_callback *sync_callback;
struct zriver_control_v1 *zriver_ctl = NULL;
struct zriver_command_callback_v1 *zriver_callback = NULL;
struct wl_seat *seat = NULL;
struct wl_callback *sync_callback;
bool loop = true;
char **argv;
int argc;

static void
callback_failure(void *data,
                 struct zriver_command_callback_v1 *zriver_command_callback_v1,
                 const char *failure_message) {
  if (failure_message != NULL) {
    printf("error: %s\n", failure_message);
  }
  zriver_command_callback_v1_destroy(zriver_command_callback_v1);
  zriver_control_v1_destroy(zriver_ctl);
  zriver_ctl = NULL;
  loop = false;
}
static void
callback_success(void *data,
                 struct zriver_command_callback_v1 *zriver_command_callback_v1,
                 const char *output) {
  if (output[0] != '\0') {
    printf("%s\n", output);
  }
  zriver_command_callback_v1_destroy(zriver_command_callback_v1);
  loop = false;
}

struct zriver_command_callback_v1_listener zriver_callback_listener = {
    .success = callback_success,
    .failure = callback_failure,
};

static void registry_handle_global(void *data, struct wl_registry *registry,
                                   uint32_t name, const char *interface,
                                   uint32_t version) {
  if (strcmp(interface, zriver_control_v1_interface.name) == 0) {
    zriver_ctl =
        wl_registry_bind(registry, name, &zriver_control_v1_interface, 1);
  } else if (strcmp(interface, wl_seat_interface.name) == 0) {
    seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
  }
}

static void add_arguments() {
  for (char **p = argv + 1; *p != NULL; p++) {
    zriver_control_v1_add_argument(zriver_ctl, *p);
  }
  zriver_callback = zriver_control_v1_run_command(zriver_ctl, seat);
  zriver_command_callback_v1_add_listener(zriver_callback,
                                          &zriver_callback_listener, NULL);
}

static void sync_handle_done(void *data, struct wl_callback *wl_callback,
                             uint32_t irrelevant) {
  wl_callback_destroy(wl_callback);
  sync_callback = NULL;
  if (seat == NULL) {
    fputs("compositor doesn't support wl_seat?\n", stderr);
    loop = false;
    return;
  }
  if (zriver_ctl == NULL) {
    fputs("compositor doesn't support riverctl.\n", stderr);
    loop = false;
    return;
  }
  add_arguments();
}

static const struct wl_callback_listener sync_callback_listener = {
    .done = sync_handle_done,
};

static void registry_handle_global_remove(void *a, struct wl_registry *b,
                                          uint32_t c) {
  /* this does nothing but handles global remove to prevent issues */
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove};

static bool init_wayland(void) {
  const char *display_name = getenv("WAYLAND_DISPLAY");
  if (display_name == NULL) {
    fputs("WAYLAND_DISPLAY is not set.\n", stderr);
    return false;
  }

  wl_display = wl_display_connect(display_name);
  if (wl_display == NULL) {
    fputs("Can not connect to Wayland server.\n", stderr);
    return false;
  }

  /* The registry is a global object which is used to advertise all
   * available global objects.
   */
  wl_registry = wl_display_get_registry(wl_display);
  wl_registry_add_listener(wl_registry, &registry_listener, NULL);

  sync_callback = wl_display_sync(wl_display);
  wl_callback_add_listener(sync_callback, &sync_callback_listener, NULL);

  return true;
}

int main(int argc_local, char *argv_local[]) {
  argc = argc_local;
  argv = argv_local;
  if (init_wayland()) {
    while (loop && wl_display_dispatch(wl_display) != -1) {
    };
  }
  // cleanup();
  return 0;
}
