/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static int sync_layer_to_profile(uint8_t profile_index) {
    const zmk_keymap_layer_id_t layer =
        (profile_index == 0) ? CONFIG_ZMK_BLE_PROFILE_0_LAYER : CONFIG_ZMK_BLE_PROFILE_OTHER_LAYER;

    LOG_DBG("Applying layer %d for BLE profile %d", layer, profile_index);
    return zmk_keymap_layer_to(layer);
}

static int ble_profile_layer_sync_listener(const zmk_event_t *eh) {
    const struct zmk_ble_active_profile_changed *ev = as_zmk_ble_active_profile_changed(eh);

    if (!ev) {
        return 0;
    }

    return sync_layer_to_profile(ev->index);
}

ZMK_LISTENER(ble_profile_layer_sync, ble_profile_layer_sync_listener);
ZMK_SUBSCRIPTION(ble_profile_layer_sync, zmk_ble_active_profile_changed);

static int ble_profile_layer_sync_init(void) {
    return sync_layer_to_profile(zmk_ble_active_profile_index());
}

SYS_INIT(ble_profile_layer_sync_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
