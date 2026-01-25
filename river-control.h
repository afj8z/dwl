#include "river-control-unstable-v1-private-protocol.c"
#include "river-control-unstable-v1-protocol.h"
#ifdef KEYS_USED
void default_binds(struct wl_list*);
#endif
void enter_mode(const Arg*);
void create_mode_user(const Arg*);
void oneshot_mode(const Arg*);
void clear_rules(const Arg*);
void clear_binds(const Arg*);
void setborderpx(const Arg*);
void setfocuscolor(const Arg*);
void setbordercolor(const Arg*);
void seturgentcolor(const Arg*);
struct wl_list arg_str_store;
struct wl_list rule_str_store;
struct wl_list rules_list;
struct wl_list modes_list;
typedef struct {
    Key *key;
    struct wl_list link;
    bool no_free_key;
    bool no_remove;
} Key_linked;
typedef struct {
    Rule *rule;
    struct wl_list link;
    bool no_free_rule;
    bool no_remove;
    struct Str_link *str_link;
} Rule_linked;
typedef struct Mode Mode;
struct Mode {
    struct wl_list link;
    struct wl_list linked_keys;
    struct Mode *oneshot_mode;
    char* name;
};
Mode *active_mode;
Mode *normal_mode;
struct Keysym_str_pair {
    xkb_keysym_t keysym;
    const char * keysym_str;
};
struct Mod_str_pair {
	uint32_t mod;
    const char * mod_str;
};
typedef enum {
    FUNC_STR_ARG_TYPE_NONE,
    FUNC_STR_ARG_TYPE_INT,
    FUNC_STR_ARG_TYPE_UINT,
    FUNC_STR_ARG_TYPE_FLOAT,
    FUNC_STR_ARG_TYPE_COLOR,
    FUNC_STR_ARG_TYPE_STRING_ARRAY,
    FUNC_STR_ARG_TYPE_WLR_DIRECTION,
    FUNC_STR_ARG_TYPE_LAYOUT,
} Func_str_arg_type;

struct Func_str_type_pair {
	void (*func)(const Arg *);
    Func_str_arg_type arg_type;
    const char * func_str;
};
#define STR(a,b) \
    { a, b, #a }
struct Func_str_type_pair Func_str_type_pair_list[] = {
    { clear_binds, FUNC_STR_ARG_TYPE_NONE, "clear-binds" },
    { clear_rules, FUNC_STR_ARG_TYPE_NONE, "clear-rules" },
    { enter_mode, FUNC_STR_ARG_TYPE_STRING_ARRAY, "enter-mode"},
    { oneshot_mode, FUNC_STR_ARG_TYPE_STRING_ARRAY, "oneshot-mode"},
    { create_mode_user, FUNC_STR_ARG_TYPE_STRING_ARRAY, "create-mode"},
    STR(setfocuscolor,FUNC_STR_ARG_TYPE_COLOR),
    STR(setbordercolor,FUNC_STR_ARG_TYPE_COLOR),
    STR(seturgentcolor,FUNC_STR_ARG_TYPE_COLOR),
    STR(setborderpx,FUNC_STR_ARG_TYPE_UINT),
    STR(setlayout,FUNC_STR_ARG_TYPE_LAYOUT),
    STR(spawn,FUNC_STR_ARG_TYPE_STRING_ARRAY),
    STR(focusstack,FUNC_STR_ARG_TYPE_INT),
    STR(setmfact,FUNC_STR_ARG_TYPE_FLOAT),
    STR(zoom,FUNC_STR_ARG_TYPE_NONE),
    STR(killclient,FUNC_STR_ARG_TYPE_NONE),
    STR(incnmaster,FUNC_STR_ARG_TYPE_INT),
    STR(togglefloating,FUNC_STR_ARG_TYPE_NONE),
    STR(togglefullscreen,FUNC_STR_ARG_TYPE_NONE),
    STR(view,FUNC_STR_ARG_TYPE_UINT),
    STR(toggleview,FUNC_STR_ARG_TYPE_UINT),
    STR(tagmon,FUNC_STR_ARG_TYPE_WLR_DIRECTION),
    STR(focusmon,FUNC_STR_ARG_TYPE_WLR_DIRECTION),
    STR(tag,FUNC_STR_ARG_TYPE_UINT),
    STR(toggletag,FUNC_STR_ARG_TYPE_UINT),
    STR(togglefullscreen,FUNC_STR_ARG_TYPE_NONE),
    STR(quit,FUNC_STR_ARG_TYPE_NONE),
};
#undef STR
struct Mod_str_pair Mod_str_pair_list[] = {
    {0,"none"},
    {WLR_MODIFIER_LOGO,"super"},
    {WLR_MODIFIER_LOGO,"logo"},
    {WLR_MODIFIER_CTRL,"ctrl"},
    {WLR_MODIFIER_ALT,"alt"},
    {WLR_MODIFIER_SHIFT,"shift"},
    {WLR_MODIFIER_CAPS,"caps"},
    {WLR_MODIFIER_MOD3,"mod3"},
    {WLR_MODIFIER_MOD2,"mod2"},
    {WLR_MODIFIER_MOD5,"mod5"},
};

typedef enum {
    ZRIVER_ARG_TYPE_NONE=0,
    ZRIVER_ARG_TYPE_KEY,
    ZRIVER_ARG_TYPE_RULE,
    ZRIVER_ARG_TYPE_FUNC,
} ZRIVER_ARG_TYPE;
const char *zriver_error_generic = "catchall error";
const char *zriver_error_alloc = "alloc error";
const char *zriver_error_too_few_args = "too few args";
const char *zriver_error_out_of_range = "out of arg range";
const char *zriver_error_no_matching_argument = "no matching argument";
const char *zriver_error_double_appid = "set appid more then once!";
const char *zriver_error_double_title = "set title more then once!";
const char *zriver_error_under_zero = "argument can't be less then zero!";
const char *zriver_error_invalid_keysym = "invalid keysym!";
#define STR_LINK_ARRAY_SIZE 10
struct Str_link {
    struct wl_list link;
    char* string[STR_LINK_ARRAY_SIZE];
};
struct zriver_func_arg_pair {
	void (*func)(const Arg *);
	Arg arg;
};
union zriver_arg_ptr {
    Rule_linked *rl;
    Key_linked *kl;
    struct zriver_func_arg_pair *fa;
};
typedef enum {
    ZRIVER_RULE_MATCH_TYPE_NONE=0,
    ZRIVER_RULE_MATCH_TYPE_APPID,
    ZRIVER_RULE_MATCH_TYPE_TITLE,
    ZRIVER_RULE_MATCH_TYPE_APPLYING,
} Rule_match_type_next;
typedef enum {
    ZRIVER_RULE_TYPE_NONE=0,
    ZRIVER_RULE_TYPE_TAGS,
    ZRIVER_RULE_TYPE_MONITOR,
} Rule_type;
struct zriver_arg_list_resource {
    int argc;
    ZRIVER_ARG_TYPE type;
    union zriver_arg_ptr p;
    struct Str_link *str_link;
    Func_str_arg_type key_arg_type;
    Rule_match_type_next rule_match_type;
    Rule_type rule_type;
    Mode *key_mode;
    bool rule_valid;
    bool error;
    const char* error_msg;
};

void setborderpx(const Arg *arg) {
	Client *c;
    Monitor *m;
    borderpx = arg->ui;
	wl_list_for_each(c, &clients, link) {
	    c->bw = borderpx;
    }
    wl_list_for_each(m, &mons,link) {
        arrange(m);
    }
}

void setfocuscolor(const Arg *arg) {
    const float color[4] = COLOR(arg->i);
    int i;
	for (i = 0; i < 4; i++) {
        focuscolor[i] = color[i];
    }
}
void setbordercolor(const Arg *arg) {
    const float color[4] = COLOR(arg->i);
    int i;
	for (i = 0; i < 4; i++) {
        bordercolor[i] = color[i];
    }
}
void seturgentcolor(const Arg *arg) {
    const float color[4] = COLOR(arg->i);
    int i;
	for (i = 0; i < 4; i++) {
        urgentcolor[i] = color[i];
    }
}

void zriver_control_destroy(struct wl_client *client,
			struct wl_resource *resource) {
    printf("destroy!\n");
}
void clear_str_store(struct wl_list *str_store) {
    struct Str_link *str_link,*str_link_tmp;
    int i;
    wl_list_for_each_safe(str_link,str_link_tmp,str_store,link) {
        wl_list_remove(&str_link->link);
        for (i = 0; i < STR_LINK_ARRAY_SIZE; i++) {
            if (str_link->string[i] != NULL) {
                free(str_link->string[i]);
            }
        }
        free(str_link);
    }
}
void free_str_store(struct Str_link *str_link) {
    int i;
    char** string = str_link->string;
    for (i = 0; i < STR_LINK_ARRAY_SIZE; i++) {
        if (string != NULL) {
            free(*string);
        }
        string++;
    }
    wl_list_remove(&str_link->link);
}
char* append_str_store(char** str_store_array,const char * string,int index) {
    char** append_str = str_store_array+index;
    int string_len = strlen(string) + 1;
    *append_str = malloc(sizeof(char) * string_len);
    if (*append_str != NULL) {
        memcpy(*append_str,string,string_len);
    }
    return *append_str;
}
struct Str_link* add_rule_str_store(void) {
    struct Str_link *str_link = calloc(1,sizeof(struct Str_link));
    int i;
    if (str_link == NULL) {
        return NULL;
    }
    for (i = 1; i < STR_LINK_ARRAY_SIZE ;i++) {
        str_link->string[i] = NULL;
    }
    wl_list_insert(&rule_str_store,&str_link->link);
    return str_link;
}
struct Str_link* add_arg_str_store(const char* string) { 
    struct Str_link *str_link = calloc(1,sizeof(struct Str_link));
    int i;
    int string_len = strlen(string) + 1;
    if (str_link == NULL) {
        return NULL;
    }
    str_link->string[0] = malloc(sizeof(char) * string_len);
    if (str_link->string[0] == NULL) {
        free(str_link); 
        return NULL;
    }
    memcpy(str_link->string[0],string,string_len);
    for (i = 1; i < STR_LINK_ARRAY_SIZE ;i++) {
        str_link->string[i] = NULL;
    }
    wl_list_insert(&arg_str_store,&str_link->link);
    return str_link;
}
void setup_rules(void) {
#ifdef USE_RULES
    Rule *r;
    Rule_linked *rl;
#endif
    if (rules_list.next == NULL) {
        wl_list_init(&rules_list);
    }
#ifdef USE_RULES
	for (r = rules; r < END(rules); r++) {
        rl = calloc(1,sizeof(Rule_linked));
        if (rl != NULL) {
            rl->rule = r;
            rl->no_free_rule = true;
            rl->no_remove = true; /* remove this line to make rules[] removed by clear-rules */
            wl_list_insert(&rules_list,&rl->link);
        }
    }
#endif
    if (rule_str_store.next == NULL) {
        wl_list_init(&rule_str_store);
    }
}
void clear_rules(const Arg* arg) {
    Rule_linked *rl,*tmp_rl;

    wl_list_for_each_safe(rl,tmp_rl,&rules_list,link) {
        if (rl->no_remove == false) {
            wl_list_remove(&rl->link);
            if (rl->no_free_rule == false) {
                free(rl->rule);
            }
            free(rl);
        }
    }
    clear_str_store(&rule_str_store);
}
Mode* create_mode(const char *name) {
    Mode *mode = calloc(1,sizeof(Mode));
    if (mode == NULL) {return NULL;}
    wl_list_init(&mode->linked_keys);
    wl_list_insert(&modes_list, &mode->link);
    if (name != NULL) {
        int string_len = strlen(name) + 1;
        mode->name = malloc(sizeof(char) * string_len);
        if (mode->name == NULL) {
            free(mode);
            return NULL;
        }
        memcpy(mode->name,name,string_len);
    }
    return mode;
}
Mode* get_mode(char* mode_name) {
    Mode *mode;
    wl_list_for_each(mode,&modes_list,link) {
        if (strcmp(mode_name,mode->name) == 0) {
            return mode;
        }
    }
    return NULL;
}
void oneshot_mode(const Arg *arg) {
    char * oneshot_mode_name = *(char **)arg->v;
    char * return_mode_name = *((char **)arg->v+1);
    if (oneshot_mode_name != NULL && return_mode_name != NULL) {
        Mode *oneshot_mode = get_mode(oneshot_mode_name);
        if (oneshot_mode != NULL) {
            oneshot_mode->oneshot_mode = get_mode(return_mode_name);
        }
    }
}
void create_mode_user(const Arg *arg) {
    char * mode_name = *(char **)arg->v;
    if (mode_name != NULL) {
        Mode *mode_exists = get_mode(mode_name);
        if (mode_exists == NULL) {
            create_mode(mode_name);
        }
    }
}
void enter_mode(const Arg *arg) {
    char * mode_name = *(char **)arg->v;
    Mode *mode = get_mode(mode_name);
    if (mode != NULL) {
        active_mode = mode;
    }
}
char * zriver_default_mode_name = "normal";
void setup_binds(void) {
    if (modes_list.next == NULL) {
        Mode *normal;
        wl_list_init(&modes_list);
        normal = create_mode(NULL);
        if (normal == NULL) { die("out of memory!"); }
        normal->name = zriver_default_mode_name;
        normal_mode = normal;
        active_mode = normal;
        default_binds(&normal->linked_keys);
    }
    if (arg_str_store.next == NULL) {
        wl_list_init(&arg_str_store);
    }
}
#ifdef KEYS_USED
void default_binds(struct wl_list *keys_list) {
    Key *k;
    Key_linked *kl;
	for (k = keys; k < END(keys); k++) {
        kl = calloc(1,sizeof(Key_linked));
        if (kl != NULL) {
            kl->key = k;
            kl->no_free_key = true;
            wl_list_insert(keys_list,&kl->link);
        }
	}
}
#endif
void clear_binds(const Arg* arg) {
    Key_linked *kl,*tmp_kl;
    Mode *mode,*tmp_mode;
    active_mode = normal_mode;

    wl_list_for_each_safe(mode,tmp_mode,&modes_list,link) {
        wl_list_for_each_safe(kl,tmp_kl,&mode->linked_keys,link) {
            if (kl->no_remove == false) {
                wl_list_remove(&kl->link);
                if (kl->no_free_key == false) {
                    free(kl->key);
                }
                free(kl);
            }
        }
        if (normal_mode != mode) {
            wl_list_remove(&mode->link);
            free(mode->name);
            free(mode);
        }
    }
    clear_str_store(&arg_str_store);
}

void zriver_control_add_argument(struct wl_client *client,
			     struct wl_resource *resource,
			     const char *argument) {
    struct zriver_arg_list_resource *args = wl_resource_get_user_data(resource);
    const struct Mod_str_pair *ms;
    const struct Func_str_type_pair *fst;
    bool arg_filter = false;
    Arg *arg = NULL;

    if (args->error == true) {return;}
    if (args->argc == 0) {
        if (strcmp("rule-add",argument) == 0) {
            args->type = ZRIVER_ARG_TYPE_RULE;
            args->p.rl = calloc(1,sizeof(Rule_linked));
            if (args->p.rl != NULL) {
                args->p.rl->rule = calloc(1,sizeof(Rule));
                if (args->p.rl->rule != NULL) {
                    args->str_link = add_rule_str_store();
                    args->p.rl->rule->monitor = -1;
                } else {
                    args->error = true;
                    args->error_msg = zriver_error_alloc;
                }
            } else {
                args->error = true;
                args->error_msg = zriver_error_alloc;
            }
        } else if (strcmp("map",argument) == 0 || strcmp("bind",argument) == 0) {
            args->type = ZRIVER_ARG_TYPE_KEY;
            args->p.kl = calloc(1,sizeof(Key_linked));
            if (args->p.kl != NULL) {
                args->p.kl->key = calloc(1,sizeof(Key));
                if (args->p.kl->key == NULL) {
                    args->error = true;
                    args->error_msg = zriver_error_alloc;
                } else {
                    args->str_link = add_arg_str_store(argument);
                }
            } else {
                args->error = true;
                args->error_msg = zriver_error_alloc;
            }
        } else {
            for (fst = Func_str_type_pair_list; fst < END(Func_str_type_pair_list); fst++) {
                if (strcmp(argument,fst->func_str) == 0) {
                    args->type = ZRIVER_ARG_TYPE_FUNC;
                    args->p.fa = calloc(1,sizeof(struct zriver_func_arg_pair));
                    if (args->p.fa == NULL) {
                        args->error = true;
                        args->error_msg = zriver_error_alloc;
                    } else {
                        args->p.fa->func = fst->func;
                        args->key_arg_type = fst->arg_type;
                    }
                    break;
                }
            }
            if (args->error != true && args->type != ZRIVER_ARG_TYPE_FUNC) {
                args->error = true;
                args->error_msg = zriver_error_no_matching_argument;
            }
        }
    } else if (args->type == ZRIVER_ARG_TYPE_RULE && args->str_link != NULL) {
        switch (args->rule_match_type) {
            case(ZRIVER_RULE_MATCH_TYPE_NONE):
                if (strcmp(argument,"-appid") == 0) {
                    args->rule_match_type = ZRIVER_RULE_MATCH_TYPE_APPID;
                } else if (strcmp(argument,"-title") == 0) {
                    args->rule_match_type = ZRIVER_RULE_MATCH_TYPE_TITLE;
                } else {
                    args->rule_match_type = ZRIVER_RULE_MATCH_TYPE_APPLYING;
                }
                break;
            case(ZRIVER_RULE_MATCH_TYPE_APPID):
                if (args->p.rl->rule->id == NULL) {
                    args->p.rl->rule->id = append_str_store(args->str_link->string,argument,args->argc-1);
                    args->rule_match_type = ZRIVER_RULE_MATCH_TYPE_NONE;
                } else {
                    args->error = true;
                    args->error_msg = zriver_error_double_appid;
                }
                break;
            case(ZRIVER_RULE_MATCH_TYPE_TITLE):
                if (args->p.rl->rule->title == NULL) {
                    args->p.rl->rule->title = append_str_store(args->str_link->string,argument,args->argc-1);
                    args->rule_match_type = ZRIVER_RULE_MATCH_TYPE_NONE;
                } else {
                    args->error = true;
                    args->error_msg = zriver_error_double_title;
                }
                break;
            case(ZRIVER_RULE_MATCH_TYPE_APPLYING):
                break;
        }
        if (args->rule_match_type == ZRIVER_RULE_MATCH_TYPE_APPLYING) {
            switch (args->rule_type) {
                case(ZRIVER_RULE_TYPE_NONE):
                    if (strcmp(argument,"float") == 0) {
                        args->p.rl->rule->isfloating = true;
                        args->rule_valid = true;
                    } else if (strcmp(argument,"tags") == 0){
                        args->rule_type = ZRIVER_RULE_TYPE_TAGS;
                        args->rule_valid = true;
                    } else if (strcmp(argument,"monitor") == 0){
                        args->rule_type = ZRIVER_RULE_TYPE_MONITOR;
                        args->rule_valid = true;
                    } else {
                        args->error = true;
                    }
                    break;
                case(ZRIVER_RULE_TYPE_TAGS):
                    args->p.rl->rule->tags = strtol(argument,NULL,10);
                    args->rule_type = ZRIVER_RULE_TYPE_NONE;
                    args->rule_valid = true;
                    break;
                case(ZRIVER_RULE_TYPE_MONITOR):
                    args->p.rl->rule->monitor = strtol(argument,NULL,10);
                    args->rule_type = ZRIVER_RULE_TYPE_NONE;
                    args->rule_valid = true;
                    break;
            }

        }

    } else if (args->type == ZRIVER_ARG_TYPE_FUNC) {
        if (args->argc == 1) {
            arg_filter = true;
            arg = &args->p.fa->arg;
        } else if (args->argc > 1 && args->argc < STR_LINK_ARRAY_SIZE && args->key_arg_type == FUNC_STR_ARG_TYPE_STRING_ARRAY && args->p.kl->key->arg.v != NULL) {
            append_str_store((char**)args->p.fa->arg.v,argument,args->argc-1);
        }
    } else if (args->type == ZRIVER_ARG_TYPE_KEY) {
        if (args->argc == 2) {
	        for (ms = Mod_str_pair_list; ms < END(Mod_str_pair_list); ms++) {
                if (strstr(argument,ms->mod_str)) {
                    if (args->p.kl->key->mod != 0) {
                        args->p.kl->key->mod = args->p.kl->key->mod|ms->mod;
                    } else {
                        args->p.kl->key->mod = ms->mod;
                    }
                }
            }
        } else if (args->argc == 1) {
            int arg_len = strlen(argument) + 1;
            if (arg_len > 1) {
                bool found_mode = false;
                Mode *mode;
                wl_list_for_each(mode,&modes_list,link) {
                    printf("mode name: %s, argument %s \n",mode->name,argument);
                    if (strcmp(argument,mode->name) == 0) {
                        found_mode = true;
                        args->key_mode = mode;
                        break;
                    }
                }
                if (found_mode == false) {
                    args->key_mode = create_mode(argument);
                    if (args->key_mode == NULL) {
                        args->error = true;
                        args->error_msg = zriver_error_alloc;
                    }
                }
            } else {
                args->error = true;
                args->error_msg = zriver_error_too_few_args;
            }
        } else if (args->argc == 3) {
            if (strcmp(argument,"none") == 0 ) {
                args->p.kl->key->keysym = XKB_KEY_NoSymbol;
            } else {
                args->p.kl->key->keysym = xkb_keysym_from_name(argument,XKB_KEYSYM_NO_FLAGS);
                if (args->p.kl->key->keysym == XKB_KEY_NoSymbol) {
                    args->error = true;
                    args->error_msg = zriver_error_invalid_keysym;
                }
            }
        } else if (args->argc == 4) {
            for (fst = Func_str_type_pair_list; fst < END(Func_str_type_pair_list); fst++) {
                if (strcmp(argument,fst->func_str) == 0) {
                    args->p.kl->key->func = fst->func;
                    args->key_arg_type = fst->arg_type;
                    break;
                }
            }
        } else if (args->argc == 5) {
            arg_filter = true;
            arg = &args->p.kl->key->arg;
        } else if (args->argc > 5 && args->argc < STR_LINK_ARRAY_SIZE && args->key_arg_type == FUNC_STR_ARG_TYPE_STRING_ARRAY && args->p.kl->key->arg.v != NULL) {
            append_str_store((char**)args->p.kl->key->arg.v,argument,args->argc-5);
        }
    }
    if (arg_filter == true && arg != NULL) {
            switch (args->key_arg_type) {
                case(FUNC_STR_ARG_TYPE_NONE):
                    break;
                case(FUNC_STR_ARG_TYPE_UINT):
                    arg->i = strtol(argument,NULL,10);
                    if (arg->i >= 0) {
                        arg->ui = arg->i;
                    } else {
                        args->error = true;
                        args->error_msg = zriver_error_under_zero;
                    }
                    break;
                case(FUNC_STR_ARG_TYPE_INT):
                    arg->i = strtol(argument,NULL,10);
                    break;
                case(FUNC_STR_ARG_TYPE_FLOAT):
                    arg->f = strtof(argument,NULL);
                    break;
                case(FUNC_STR_ARG_TYPE_STRING_ARRAY):
                    args->str_link = add_arg_str_store(argument);
                    if (args->str_link == NULL) {
                        printf("string arg NULL \n");
                    } else {
                        arg->v = args->str_link->string;
                    }

                    break;
                case(FUNC_STR_ARG_TYPE_WLR_DIRECTION):
                    if (strcmp("up",argument)) {
                        arg->i = WLR_DIRECTION_UP;
                    } else if (strcmp("left",argument)) {
                        arg->i = WLR_DIRECTION_LEFT;
                    } else if (strcmp("right",argument)) {
                        arg->i = WLR_DIRECTION_RIGHT;
                    } else if (strcmp("down",argument)) {
                        arg->i = WLR_DIRECTION_DOWN;
                    } else {
                        args->error = true;
                        args->error_msg = zriver_error_out_of_range;
                    }
                    break;
                case(FUNC_STR_ARG_TYPE_LAYOUT):
                    arg->ui = strtol(argument,NULL,10);
                    if (arg->ui < (int)LENGTH(layouts)) {
                        arg->v = &layouts[arg->ui];
                    } else {
                        args->error = true;
                        args->error_msg = zriver_error_out_of_range;
                    }
                    break;
                case(FUNC_STR_ARG_TYPE_COLOR):
                    arg->i = strtol(argument,NULL,16);
                    break;
            }
    }
    args->argc++;
    printf("add arg '%s' !\n",argument);
}
void zriver_control_run_command(struct wl_client *client,
			    struct wl_resource *resource,
			    struct wl_resource *run_command_seat,
			    uint32_t callback) {
    struct zriver_arg_list_resource *args = wl_resource_get_user_data(resource);
    struct wl_resource *callback_interface = wl_resource_create(
        client, &zriver_command_callback_v1_interface, zriver_command_callback_v1_interface.version, callback);
    if (args->argc == 0) {
        zriver_command_callback_v1_send_failure(callback_interface,zriver_error_too_few_args);
    } else if (args->error == true) {
        switch (args->type) {
            case(ZRIVER_ARG_TYPE_KEY):
                if (args->p.kl != NULL) {
                    if (args->p.kl->key != NULL) {
                        free(args->p.kl->key);
                    }
                    free(args->p.kl);
                }
                break;
            case(ZRIVER_ARG_TYPE_FUNC):
                if (args->p.fa != NULL) {
                    free(args->p.fa);
                }
                break;
            case(ZRIVER_ARG_TYPE_RULE):
                if (args->p.rl != NULL) {
                    if (args->p.rl->rule != NULL) {
                        free(args->p.rl->rule);
                    }
                    free(args->p.rl);
                }
                break;
            case(ZRIVER_ARG_TYPE_NONE):
                break;
        }
        if (args->str_link != NULL) {
            free_str_store(args->str_link);
            free(args->str_link);
        }
        zriver_command_callback_v1_send_failure(callback_interface,args->error_msg);
    } else if (args->error == false) {
        if (args->type == ZRIVER_ARG_TYPE_KEY) {
            if (args->p.kl->key != NULL && args->p.kl->key->func != NULL) {
                wl_list_insert(&args->key_mode->linked_keys,&args->p.kl->link);
                zriver_command_callback_v1_send_success(callback_interface,"bind success!");
            } else {
                if (args->str_link != NULL) {
                    free_str_store(args->str_link);
                    free(args->str_link);
                }
                if (args->p.kl->key != NULL) {
                    free(args->p.kl->key);
                }
                zriver_command_callback_v1_send_failure(callback_interface,zriver_error_too_few_args);
            }
        } else if (args->type == ZRIVER_ARG_TYPE_FUNC) {
            if (args->p.fa->func != NULL) {
                args->p.fa->func(&args->p.fa->arg);
                zriver_command_callback_v1_send_success(callback_interface,"command success!");
            } else {
                zriver_command_callback_v1_send_failure(callback_interface,zriver_error_too_few_args);
            }
            if (args->str_link != NULL) {
                free_str_store(args->str_link);
                free(args->str_link);
            }
            free(args->p.fa);
        } else if (args->type == ZRIVER_ARG_TYPE_RULE) {
            if (args->rule_valid == true) {
                /* check for rule with same title and id */
                bool replaced_rule = false;
#ifdef NEW_RULES_OVERRIDE
                if (args->p.rl->rule->title != NULL || args->p.rl->rule->id != NULL) {
                    Rule_linked *rl;
                    Rule *r;
                    wl_list_for_each(rl,&rules_list,link) {
                        r = rl->rule;
#define CHECKNULL(a,b) ((a != NULL && b != NULL && strcmp(a,b) == 0) || (a == NULL && b == NULL))
                        if (CHECKNULL(args->p.rl->rule->title,r->title) && CHECKNULL(args->p.rl->rule->id,r->id)) {
                            wl_list_remove(&rl->link);
                            if (rl->no_free_rule == false) {
                                free(rl->rule);
                            }
                            if (rl->str_link != NULL) {
                                free_str_store(rl->str_link);
                                free(rl->str_link);
                            }
                            free(rl);
                            replaced_rule = true;
                            break;
                        }
                    }
                }
#endif
                wl_list_insert(rules_list.prev,&args->p.rl->link);
                if (replaced_rule == true) {
                    zriver_command_callback_v1_send_success(callback_interface,"rule replaced!");
                } else {
                    zriver_command_callback_v1_send_success(callback_interface,"rule success!");
                }
            } else {
                zriver_command_callback_v1_send_failure(callback_interface,zriver_error_too_few_args);
                free_str_store(args->str_link);
                free(args->str_link);
                free(args->p.rl->rule);
                free(args->p.rl);
            }
        } else {
            zriver_command_callback_v1_send_success(callback_interface,"");
        }
    }
}
struct zriver_control_v1_interface zriver_control_interface = {
    .run_command = zriver_control_run_command,
    .add_argument = zriver_control_add_argument,
    .destroy = zriver_control_destroy,
};
static void zriver_control_handle_destory(struct wl_resource *resource) {
    struct zriver_arg_list_resource *zriver_arg_list_resource = wl_resource_get_user_data(resource);
    free(zriver_arg_list_resource);
    printf("handle destroy\n");
}
static void zriver_control_handle_bind(struct wl_client *client, void *data,
        uint32_t version, uint32_t id) {
    struct zriver_arg_list_resource *zriver_arg_list_resource = calloc(1,sizeof(struct zriver_arg_list_resource) );
    struct wl_resource *resource = wl_resource_create(
        client, &zriver_control_v1_interface, zriver_control_v1_interface.version, id);
    zriver_arg_list_resource->error_msg = zriver_error_generic;


    wl_resource_set_implementation(resource, &zriver_control_interface,
        zriver_arg_list_resource, zriver_control_handle_destory);
}
