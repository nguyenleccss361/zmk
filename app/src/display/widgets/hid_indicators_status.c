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
#include <zmk/event_manager.h>
#include <zmk/hid_indicators.h>

#define HID_INDICATOR_CAPS_LOCK BIT(1)

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct hid_indicators_status_state {
    zmk_hid_indicators_t indicators;
};

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
        .indicators = (ev != NULL) ? ev->indicators : zmk_hid_indicators_get_current_profile(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_hid_indicators_status, struct hid_indicators_status_state,
                            hid_indicators_status_update_cb, hid_indicators_status_get_state)

ZMK_SUBSCRIPTION(widget_hid_indicators_status, zmk_hid_indicators_changed);
ZMK_SUBSCRIPTION(widget_hid_indicators_status, zmk_endpoint_changed);

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
