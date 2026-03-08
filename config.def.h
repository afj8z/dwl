/* Taken from https://github.com/djpohly/dwl/issues/466 */
#define COLOR(hex)                                                            \
    { ((hex >> 24) & 0xFF) / 255.0f, ((hex >> 16) & 0xFF) / 255.0f,           \
      ((hex >> 8) & 0xFF) / 255.0f, (hex & 0xFF) / 255.0f }
/* appearance */
static const int sloppyfocus = 1; /* focus follows mouse */
static const int bypass_surface_visibility
    = 0; /* 1 means idle inhibitors will disable idle tracking even if it's
            surface isn't visible  */
static unsigned int borderpx = 0; /* border pixel of windows */
static const float rootcolor[] = COLOR (0x222222ff);
static float bordercolor[] = COLOR (0x444444ff);
static float focuscolor[] = COLOR (0x005577ff);
static float urgentcolor[] = COLOR (0xff0000ff);
/* This conforms to the xdg-protocol. Set the alpha to zero to restore the old
 * behavior */
static const float fullscreen_bg[]
    = { 0.1f, 0.1f, 0.1f, 0.0f }; /* You can also use glsl colors */

static const int respect_monitor_reserved_area
    = 0; /* 1 to monitor center while respecting the monitor's reserved area, 0
            to monitor center */
/* VANITYGAPS PATCH */
static const int smartgaps
    = 0; /* 1 means no outer gap when there is only one window */
static const int monoclegaps = 0; /* 1 means outer gaps in monocle layout */
static const unsigned int gappih = 5; /* horiz inner gap between windows */
static const unsigned int gappiv = 5; /* vert inner gap between windows */
static const unsigned int gappoh
    = 25; /* horiz outer gap between windows and screen edge */
static const unsigned int gappov
    = 25; /* vert outer gap between windows and screen edge */
          /* VANITYGAPS PATCH END */

/* CLIENT OPACITY PATCH */
static const float default_opacity_unfocus = 0.90f;

static const float default_opacity_focus = 1.00f;

/* SCENEFX PATCH
 * ADD: removed opacity handling.
 */
static int shadow = 0; /* flag to enable shadow */
static const int shadow_only_floating
    = 0; /* only apply shadow to floating windows */
static const float shadow_color[4] = COLOR (0x000000ff);
static const float shadow_color_focus[4] = COLOR (0x222222ff);
static const int shadow_blur_sigma = 20;
static const int shadow_blur_sigma_focus = 40;
static const char *const shadow_ignore_list[]
    = { NULL };                     /* list of app-id to ignore */
static int corner_radius = 0;       /* 0 disables corner_radius */
static int corner_radius_inner = 0; /* 0 disables corner_radius */
static const int corner_radius_only_floating
    = 0; /* only apply corner_radius and corner_radius_inner to floating
          * windows
          */
static const int blur = 0;      /* flag to enable blur */
static const int blur_xray = 0; /* flag to make transparent fs and floating
                                   windows display your background */
static const int blur_ignore_transparent = 1;
static const struct blur_data blur_data = {
    .radius = 4,
    .num_passes = 3,
    .noise = (float)0.02,
    .brightness = (float)0.9,
    .contrast = (float)0.9,
    .saturation = (float)1.4,
};
/* SCENEFX PATCH END */

/* tagging - TAGCOUNT must be no greater than 31 */
#define TAGCOUNT (9)

/* logging */
static int log_level = WLR_ERROR;

#define USE_RULES
#define NEW_RULES_OVERRIDE
static Rule rules[] = {
    /* app_id  title  tags mask  isfloating  alpha  monitor  x y width height
     */
    { "foot_FLOAT_C", NULL, 0, 1, 1, -1, 500, 250, 920,
      580 }, /* Start on currently visible tags floating, not tiled */
    { "CLIENT_FLOAT", NULL, 0, 1, 1, -1, -2, -2, -1,
      -1 }, /* window centered; sizing defered back to wayland client */
};

/* layout(s) */
static const Layout layouts[] = {
    /* symbol     arrange function */
    { "[]=", tile },
    { "><>", NULL }, /* no layout function means floating behavior */
    { "[M]", monocle },
};

/* monitors */
/* (x=-1, y=-1) is reserved as an "autoconfigure" monitor position indicator
 * WARNING: negative values other than (-1, -1) cause problems with Xwayland
 * clients due to https://gitlab.freedesktop.org/xorg/xserver/-/issues/899 */
static const MonitorRule monrules[] = {
    /* name        mfact  nmaster scale layout       rotate/reflect x    y
     * example of a HiDPI laptop monitor:
     { "eDP-1",    0.5f,  1,      2,    &layouts[0],
     WL_OUTPUT_TRANSFORM_NORMAL, -1,  -1 }, */
    { NULL, 0.5f, 1, 1, &layouts[0], WL_OUTPUT_TRANSFORM_NORMAL, -1, -1 },
    /* default monitor rule: can be changed but cannot be eliminated; at least
       one monitor rule must exist */
};

/* keyboard */
static const struct xkb_rule_names xkb_rules = {
    /* can specify fields: rules, model, layout, variant, options */
    /* example:
    .options = "ctrl:nocaps",
    */
    .options = "compose:menu"
};

static const int repeat_rate = 25;
static const int repeat_delay = 200;

/* Trackpad */
static const int tap_to_click = 1;
static const int tap_and_drag = 1;
static const int drag_lock = 1;
static const int natural_scrolling = 1;
static const int disable_while_typing = 1;
static const int left_handed = 0;
static const int middle_button_emulation = 0;
/* You can choose between:
LIBINPUT_CONFIG_SCROLL_NO_SCROLL
LIBINPUT_CONFIG_SCROLL_2FG
LIBINPUT_CONFIG_SCROLL_EDGE
LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN
*/
static const enum libinput_config_scroll_method scroll_method
    = LIBINPUT_CONFIG_SCROLL_2FG;

/* You can choose between:
LIBINPUT_CONFIG_CLICK_METHOD_NONE
LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS
LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER
*/
static const enum libinput_config_click_method click_method
    = LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS;

/* You can choose between:
LIBINPUT_CONFIG_SEND_EVENTS_ENABLED
LIBINPUT_CONFIG_SEND_EVENTS_DISABLED
LIBINPUT_CONFIG_SEND_EVENTS_DISABLED_ON_EXTERNAL_MOUSE
*/
static const uint32_t send_events_mode = LIBINPUT_CONFIG_SEND_EVENTS_ENABLED;

/* You can choose between:
LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT
LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE
*/
static const enum libinput_config_accel_profile accel_profile
    = LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
static const double accel_speed = 0.0;

/* You can choose between:
LIBINPUT_CONFIG_TAP_MAP_LRM -- 1/2/3 finger tap maps to left/right/middle
LIBINPUT_CONFIG_TAP_MAP_LMR -- 1/2/3 finger tap maps to left/middle/right
*/
static const enum libinput_config_tap_button_map button_map
    = LIBINPUT_CONFIG_TAP_MAP_LRM;

/* If you want to use the windows key for MODKEY, use WLR_MODIFIER_LOGO */
#define MODKEY WLR_MODIFIER_LOGO

#define TAGKEYS(KEY, SKEY, TAG)                                               \
    { MODKEY, KEY, view, { .ui = 1 << TAG } },                                \
        { MODKEY | WLR_MODIFIER_CTRL, KEY, toggleview, { .ui = 1 << TAG } },  \
        { MODKEY | WLR_MODIFIER_SHIFT, SKEY, tag, { .ui = 1 << TAG } },       \
    {                                                                         \
        MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT, SKEY, toggletag,     \
        {                                                                     \
            .ui = 1 << TAG                                                    \
        }                                                                     \
    }

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd)                                                            \
    {                                                                         \
        .v = (const char *[]) { "/bin/sh", "-c", cmd, NULL }                  \
    }

/* commands */
static const char *termcmd[] = { "foot", NULL };
static const char *menucmd[] = { "bemenu-run", "-p", "run ", NULL };

/* media controls */
static const char *volup[] = { "wpctl", "set-volume",           "-l",
                               "1",     "@DEFAULT_AUDIO_SINK@", "5%+",
                               NULL };
static const char *voldown[] = { "wpctl", "set-volume",           "-l",
                                 "1",     "@DEFAULT_AUDIO_SINK@", "5%-",
                                 NULL };
static const char *mute[]
    = { "wpctl", "set-mute", "@DEFAULT_AUDIO_SINK@", "toggle", NULL };
static const char *lightup[] = { "notify-med", "bright_up", NULL };
static const char *lightdown[] = { "notify-med", "bright_down", NULL };

/* note keys gets cleared with riverctl clear-binds but the keys_always are
 * excluded from being cleared this is to have a list of fallback keybinds if
 * your riverctl script fails if you won't like to have keys[] declared
 * commented out the KEYS_USED macro bellow to disable the functionality*/
#define KEYS_USED
static Key keys[] = {
    /* Note that Shift changes certain key codes: 2 -> at, etc. */
    /* modifier                  key                  function argument
     */
    // { MODKEY, XKB_KEY_g, spawn, SHCMD ("dmenu-webapps") },
    // { MODKEY, XKB_KEY_a, spawn, SHCMD ("rofi-system-menu") },
    // { MODKEY, XKB_KEY_d, spawn, { .v = menucmd } },
    { MODKEY, XKB_KEY_Return, spawn, { .v = termcmd } },
    { MODKEY, XKB_KEY_j, focusstack, { .i = +1 } },
    { MODKEY, XKB_KEY_k, focusstack, { .i = -1 } },
    { MODKEY, XKB_KEY_i, incnmaster, { .i = +1 } },
    { MODKEY, XKB_KEY_o, incnmaster, { .i = -1 } },
    { MODKEY, XKB_KEY_h, setmfact, { .f = -0.05f } },
    { MODKEY, XKB_KEY_l, setmfact, { .f = +0.05f } },
    { MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_Return, zoom, { 0 } },
    { MODKEY, XKB_KEY_Tab, view, { 0 } },
    { MODKEY, XKB_KEY_q, killclient, { 0 } },
    { MODKEY, XKB_KEY_t, setlayout, { .v = &layouts[0] } },
    { MODKEY, XKB_KEY_e, setlayout, { .v = &layouts[1] } },
    { MODKEY, XKB_KEY_m, setlayout, { .v = &layouts[2] } },
    { MODKEY, XKB_KEY_space, setlayout, { 0 } },
    { MODKEY, XKB_KEY_b, togglebar, { 0 } },
    { MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_space, togglefloating, { 0 } },
    { MODKEY, XKB_KEY_f, togglefullscreen, { 0 } },
    { MODKEY, XKB_KEY_0, view, { .ui = ~0 } },

    /* VANITYGAPS PATCH KEYS */

    { MODKEY, XKB_KEY_equal, incogaps, { .i = +8 } },
    { MODKEY, XKB_KEY_minus, incogaps, { .i = -8 } },
    { MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_plus, incigaps, { .i = +8 } },
    { MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_underscore, incigaps, { .i = -8 } },
    { MODKEY, XKB_KEY_g, togglegaps, { 0 } },
    { MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_G, defaultgaps, { 0 } },

    /* VANITYGAPS PATCH KEYS END */

    /* CLIENT OPACITY KEYS */
    { MODKEY | WLR_MODIFIER_CTRL,
      XKB_KEY_i,
      setopacityunfocus,
      { .f = +0.1f } },
    { MODKEY | WLR_MODIFIER_CTRL,
      XKB_KEY_o,
      setopacityunfocus,
      { .f = -0.1f } },
    { MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT,
      XKB_KEY_I,
      setopacityfocus,
      { .f = +0.1f } },
    { MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT,
      XKB_KEY_O,
      setopacityfocus,
      { .f = -0.1f } },
    /* CLIENT OPACITY KEYS END */

    // media control
    { 0, XKB_KEY_XF86AudioMute, spawn, { .v = mute } },
    { 0, XKB_KEY_XF86AudioLowerVolume, spawn, { .v = voldown } },
    { 0, XKB_KEY_XF86AudioRaiseVolume, spawn, { .v = volup } },
    { 0, XKB_KEY_XF86MonBrightnessUp, spawn, { .v = lightup } },
    { 0, XKB_KEY_XF86MonBrightnessDown, spawn, { .v = lightdown } },
    { MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_S, tag, { .ui = ~0 } },
    { MODKEY, XKB_KEY_comma, focusmon, { .i = WLR_DIRECTION_LEFT } },
    { MODKEY, XKB_KEY_period, focusmon, { .i = WLR_DIRECTION_RIGHT } },
    { MODKEY | WLR_MODIFIER_SHIFT,
      XKB_KEY_less,
      tagmon,
      { .i = WLR_DIRECTION_LEFT } },

    { MODKEY | WLR_MODIFIER_SHIFT,
      XKB_KEY_greater,
      tagmon,
      { .i = WLR_DIRECTION_RIGHT } },
    TAGKEYS (XKB_KEY_1, XKB_KEY_exclam, 0),
    TAGKEYS (XKB_KEY_2, XKB_KEY_at, 1),
    TAGKEYS (XKB_KEY_3, XKB_KEY_numbersign, 2),
    TAGKEYS (XKB_KEY_4, XKB_KEY_dollar, 3),
    TAGKEYS (XKB_KEY_5, XKB_KEY_percent, 4),
    TAGKEYS (XKB_KEY_6, XKB_KEY_asciicircum, 5),
    TAGKEYS (XKB_KEY_7, XKB_KEY_ampersand, 6),
    TAGKEYS (XKB_KEY_8, XKB_KEY_asterisk, 7),
    TAGKEYS (XKB_KEY_9, XKB_KEY_parenleft, 8),
};
static Key keys_always[] = {
    { MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_q, quit, { 0 } },
    { WLR_MODIFIER_CTRL | WLR_MODIFIER_ALT,
      XKB_KEY_Terminate_Server,
      quit,
      { 0 } },
#define CHVT(n)                                                               \
    {                                                                         \
        WLR_MODIFIER_CTRL | WLR_MODIFIER_ALT, XKB_KEY_XF86Switch_VT_##n,      \
            chvt,                                                             \
        {                                                                     \
            .ui = (n)                                                         \
        }                                                                     \
    }
    CHVT (1),
    CHVT (2),
    CHVT (3),
    CHVT (4),
    CHVT (5),
    CHVT (6),
    CHVT (7),
    CHVT (8),
    CHVT (9),
    CHVT (10),
    CHVT (11),
    CHVT (12),
};

static const Button buttons[] = {
    { MODKEY, BTN_LEFT, moveresize, { .ui = CurMove } },
    { MODKEY, BTN_MIDDLE, togglefloating, { 0 } },
    { MODKEY, BTN_RIGHT, moveresize, { .ui = CurResize } },
};
