/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/activity.h>
#include <zmk/display/widgets/sleep_status.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/position_state_changed.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static struct k_work_delayable refresh_work;
static int64_t last_activity_ms;

static void update_all_widgets(void) {
    struct zmk_widget_sleep_status *widget;
    char text[16] = "";

    if (zmk_activity_get_state() == ZMK_ACTIVITY_SLEEP) {
        lv_snprintf(text, sizeof(text), "SLP");
    } else if (CONFIG_ZMK_IDLE_SLEEP_TIMEOUT <= 0) {
        lv_snprintf(text, sizeof(text), "AWK");
    } else {
        int64_t elapsed = k_uptime_get() - last_activity_ms;
        int64_t remaining = CONFIG_ZMK_IDLE_SLEEP_TIMEOUT - elapsed;
        if (remaining < 0) {
            remaining = 0;
        }
        int mins = (int)(remaining / 60000);
        int secs = (int)((remaining % 60000) / 1000);
        lv_snprintf(text, sizeof(text), "%02d:%02d", mins, secs);
    }

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { lv_label_set_text(widget->obj, text); }
}

static void refresh_work_cb(struct k_work *work) {
    ARG_UNUSED(work);
    update_all_widgets();
    k_work_reschedule(&refresh_work, K_SECONDS(1));
}

static int sleep_status_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *pos = as_zmk_position_state_changed(eh);
    if (pos != NULL && pos->state) {
        last_activity_ms = k_uptime_get();
        update_all_widgets();
        return ZMK_EV_EVENT_BUBBLE;
    }

    const struct zmk_keycode_state_changed *key = as_zmk_keycode_state_changed(eh);
    if (key != NULL && key->state) {
        last_activity_ms = k_uptime_get();
        update_all_widgets();
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (as_zmk_activity_state_changed(eh) != NULL) {
        if (zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE) {
            last_activity_ms = k_uptime_get();
        }
        update_all_widgets();
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(widget_sleep_status, sleep_status_listener);
ZMK_SUBSCRIPTION(widget_sleep_status, zmk_position_state_changed);
ZMK_SUBSCRIPTION(widget_sleep_status, zmk_keycode_state_changed);
ZMK_SUBSCRIPTION(widget_sleep_status, zmk_activity_state_changed);

int zmk_widget_sleep_status_init(struct zmk_widget_sleep_status *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);

    sys_slist_append(&widgets, &widget->node);

    if (!k_work_delayable_is_pending(&refresh_work)) {
        last_activity_ms = k_uptime_get();
        k_work_init_delayable(&refresh_work, refresh_work_cb);
        k_work_schedule(&refresh_work, K_SECONDS(1));
    }

    update_all_widgets();
    return 0;
}

lv_obj_t *zmk_widget_sleep_status_obj(struct zmk_widget_sleep_status *widget) { return widget->obj; }
