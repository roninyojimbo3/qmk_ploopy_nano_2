/* Copyright 2021 Colin Lam (Ploopy Corporation)
 * Copyright 2020 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
 * Copyright 2019 Sunjun Kim
 * Copyright 2019 Hiroyuki Okada
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include QMK_KEYBOARD_H

enum custom_keycodes {
    NANO_BTN = SAFE_RANGE
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(NANO_BTN)
};

#define HOLD_TERM 220
#define TAP_GAP   260
#define SCROLL_THRESHOLD 150
#define MEDIA_THRESHOLD 220

static bool btn_down = false;
static bool hold_active = false;
static uint16_t press_timer = 0;
static uint16_t last_tap_timer = 0;
static uint8_t tap_count = 0;

static bool scroll_mode = false;
static bool precision_mode = false;
static bool media_mode = false;

static int16_t scroll_x = 0;
static int16_t scroll_y = 0;
static int16_t media_x = 0;
static int16_t media_y = 0;

static void finish_taps(void) {
    if (tap_count == 1) {
        tap_code16(QK_MOUSE_BUTTON_1);
    } else if (tap_count == 2) {
        tap_code16(QK_MOUSE_BUTTON_2);
    } else if (tap_count == 3) {
        tap_code16(QK_MOUSE_BUTTON_3);
    }

    tap_count = 0;
}

static void start_hold_action(void) {
    hold_active = true;

    if (tap_count == 0) {
        register_code16(QK_MOUSE_BUTTON_1);   // Hold = drag
    } else if (tap_count == 1) {
        scroll_mode = true;                   // Tap-hold = scroll
    } else if (tap_count == 2) {
        precision_mode = true;                // Tap-tap-hold = precision
    } else if (tap_count == 3) {
        media_mode = true;                    // Tap-tap-tap-hold = media
    }

    tap_count = 0;
}

static void stop_hold_action(void) {
    unregister_code16(QK_MOUSE_BUTTON_1);

    scroll_mode = false;
    precision_mode = false;
    media_mode = false;
    hold_active = false;

    scroll_x = 0;
    scroll_y = 0;
    media_x = 0;
    media_y = 0;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (keycode != NANO_BTN) {
        return true;
    }

    if (record->event.pressed) {
        btn_down = true;
        press_timer = timer_read();
    } else {
        btn_down = false;

        if (hold_active) {
            stop_hold_action();
        } else {
            tap_count++;
            last_tap_timer = timer_read();
        }
    }

    return false;
}

void matrix_scan_user(void) {
    if (btn_down && !hold_active && timer_elapsed(press_timer) > HOLD_TERM) {
        start_hold_action();
    }

    if (!btn_down && tap_count > 0 && timer_elapsed(last_tap_timer) > TAP_GAP) {
        finish_taps();
    }
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    int16_t raw_x = mouse_report.x;
    int16_t raw_y = mouse_report.y;

    if (scroll_mode) {
        scroll_x += raw_x;
        scroll_y += raw_y;

        mouse_report.x = 0;
        mouse_report.y = 0;
        mouse_report.h = 0;
        mouse_report.v = 0;

        if (scroll_x > SCROLL_THRESHOLD) {
            mouse_report.h = 1;
            scroll_x = 0;
        } else if (scroll_x < -SCROLL_THRESHOLD) {
            mouse_report.h = -1;
            scroll_x = 0;
        }

        if (scroll_y > SCROLL_THRESHOLD) {
            mouse_report.v = -1;
            scroll_y = 0;
        } else if (scroll_y < -SCROLL_THRESHOLD) {
            mouse_report.v = 1;
            scroll_y = 0;
        }
    }

    else if (precision_mode) {
        mouse_report.x = raw_x / 3;
        mouse_report.y = raw_y / 3;
        mouse_report.h = 0;
        mouse_report.v = 0;
    }

    else if (media_mode) {
        media_x += raw_x;
        media_y += raw_y;

        mouse_report.x = 0;
        mouse_report.y = 0;
        mouse_report.h = 0;
        mouse_report.v = 0;

        if (media_y < -MEDIA_THRESHOLD) {
            tap_code16(KC_VOLU);
            media_y = 0;
        } else if (media_y > MEDIA_THRESHOLD) {
            tap_code16(KC_VOLD);
            media_y = 0;
        }

        if (media_x > MEDIA_THRESHOLD) {
            tap_code16(KC_MNXT);
            media_x = 0;
        } else if (media_x < -MEDIA_THRESHOLD) {
            tap_code16(KC_MPRV);
            media_x = 0;
        }
    }

    else {
        mouse_report.h = 0;
        mouse_report.v = 0;
    }

    return mouse_report;
}
