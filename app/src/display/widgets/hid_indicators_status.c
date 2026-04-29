/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/hid_indicators_status.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/hid_indicators.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include <dt-bindings/zmk/hid_usage.h>

#define HID_INDICATOR_CAPS_LOCK BIT(1)

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct hid_indicators_status_state {
    zmk_hid_indicators_t indicators;
};

static bool local_caps_on;

static void set_indicator_text(lv_obj_t *label, struct hid_indicators_status_state state) {
    char text[10] = "";
    if (state.indicators & HID_INDICATOR_CAPS_LOCK) {
        lv_snprintf(text, sizeof(text), "CAP");
    }
    lv_label_set_text(label, text);
}

static void hid_indicators_status_update_cb(struct hid_indicators_status_state state) {
    struct zmk_widget_hid_indicators_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_indicator_text(widget->obj, state); }
}

static struct hid_indicators_status_state hid_indicators_status_get_state(const zmk_event_t *eh) {
    const struct zmk_hid_indicators_changed *ev = as_zmk_hid_indicators_changed(eh);

    return (struct hid_indicators_status_state){
        .indicators = (ev != NULL)
                          ? ev->indicators
                          : (local_caps_on ? HID_INDICATOR_CAPS_LOCK :
                                             zmk_hid_indicators_get_current_profile()),
    };
}

static int hid_indicators_caps_fallback_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

    if (!ev || !ev->state) {
        return 0;
    }

    if (ev->usage_page == HID_USAGE_KEY && ev->keycode == HID_USAGE_KEY_KEYBOARD_CAPS_LOCK) {
        local_caps_on = !local_caps_on;
        hid_indicators_status_update_cb((struct hid_indicators_status_state){
            .indicators = local_caps_on ? HID_INDICATOR_CAPS_LOCK : 0,
        });
    }

    return 0;
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_hid_indicators_status, struct hid_indicators_status_state,
                            hid_indicators_status_update_cb, hid_indicators_status_get_state)

ZMK_SUBSCRIPTION(widget_hid_indicators_status, zmk_hid_indicators_changed);
ZMK_SUBSCRIPTION(widget_hid_indicators_status, zmk_endpoint_changed);
ZMK_LISTENER(hid_indicators_caps_fallback, hid_indicators_caps_fallback_listener);
ZMK_SUBSCRIPTION(hid_indicators_caps_fallback, zmk_keycode_state_changed);

int zmk_widget_hid_indicators_status_init(struct zmk_widget_hid_indicators_status *widget,
                                          lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);

    sys_slist_append(&widgets, &widget->node);

    widget_hid_indicators_status_init();
    return 0;
}

lv_obj_t *zmk_widget_hid_indicators_status_obj(struct zmk_widget_hid_indicators_status *widget) {
    return widget->obj;
}
