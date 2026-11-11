#include <lvgl.h>
#include <stdio.h>
#include "ui.h"
#include "config.h"

#define ALERT_THRESHOLD_A 20.0f
#define LOCK_TIMEOUT_MS 60000

static RelaisCallback relaisCb = nullptr;
static ScenarioCallback scenarioCb = nullptr;

static lv_obj_t *screen;
static lv_obj_t *tabview;
static lv_obj_t *currentLabel;
static lv_obj_t *currentBigLabel;
static lv_obj_t *powerLabel;
static lv_obj_t *clockLabel;
static lv_obj_t *alertBanner;
static lv_obj_t *arc;
static lv_obj_t *arcLabel;
static lv_obj_t *lockScreen;
static lv_obj_t *relaisButtons[NB_RELAIS];
static lv_obj_t *chart;
static lv_chart_series_t *chartSeries;

static lv_style_t styleBtnOn;
static lv_style_t styleBtnOff;
static lv_style_t styleCard;

static unsigned long lastActivity = 0;
static bool locked = false;

static void touchActivity() { lastActivity = lv_tick_get(); }

static void relaisEventHandler(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    if (relaisCb) relaisCb(idx, !lv_obj_has_state(relaisButtons[idx], LV_STATE_CHECKED));
}

static void scenarioEventHandler(lv_event_t *e) {
    const char *name = (const char *)lv_obj_get_user_data(lv_event_get_target(e));
    if (scenarioCb) scenarioCb(name);
}

static void unlockEventHandler(lv_event_t *e) {
    uiSetLocked(false);
}

static void createHeader(lv_obj_t *parent) {
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, SCREEN_WIDTH, 56);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x14264A), 0);
    lv_obj_set_style_bg_grad_color(header, lv_color_hex(0x0B1526), 0);
    lv_obj_set_style_bg_grad_dir(header, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_shadow_width(header, 12, 0);
    lv_obj_set_style_shadow_color(header, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(header, LV_OPA_40, 0);
    lv_obj_set_style_shadow_ofs_y(header, 4, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "GESTION ATELIER");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 24, 0);

    clockLabel = lv_label_create(header);
    lv_label_set_text(clockLabel, "--:--");
    lv_obj_set_style_text_color(clockLabel, lv_color_hex(0x4CC9F0), 0);
    lv_obj_set_style_text_font(clockLabel, &lv_font_montserrat_18, 0);
    lv_obj_align(clockLabel, LV_ALIGN_RIGHT_MID, -24, 0);
}

static lv_obj_t *createAlertBanner(lv_obj_t *parent) {
    lv_obj_t *banner = lv_obj_create(parent);
    lv_obj_set_size(banner, SCREEN_WIDTH, 0);
    lv_obj_align(banner, LV_ALIGN_TOP_MID, 0, 56);
    lv_obj_set_style_bg_color(banner, lv_color_hex(0xE63946), 0);
    lv_obj_set_style_border_width(banner, 0, 0);
    lv_obj_clear_flag(banner, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(banner, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *lbl = lv_label_create(banner);
    lv_label_set_text(lbl, "SURCHARGE DETECTEE");
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl);
    return banner;
}

static lv_obj_t *createCard(lv_obj_t *parent, int w, int h, lv_align_t align, int x_ofs) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_align(card, align, x_ofs, 0);
    lv_obj_add_style(card, &styleCard, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

static lv_obj_t *createTab(const char *name) {
    lv_obj_t *tab = lv_tabview_add_tab(tabview, name);
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x0A1420), 0);
    return tab;
}

static void buildHomeTab(lv_obj_t *tab) {
    lv_obj_t *card = createCard(tab, 350, 300, LV_ALIGN_LEFT_MID, 20);

    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, "CONSOMMATION");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x8D99AE), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 16);

    arc = lv_arc_create(card);
    lv_obj_set_size(arc, 200, 200);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 10);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, 0, 300);
    lv_arc_set_value(arc, 0);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x00D4AA), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 14, LV_PART_BG);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x1B263B), LV_PART_BG);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);

    arcLabel = lv_label_create(arc);
    lv_label_set_text(arcLabel, "0.0 A");
    lv_obj_set_style_text_color(arcLabel, lv_color_white(), 0);
    lv_obj_set_style_text_font(arcLabel, &lv_font_montserrat_28, 0);
    lv_obj_center(arcLabel);

    currentBigLabel = lv_label_create(card);
    lv_label_set_text(currentBigLabel, "0.00 A");
    lv_obj_set_style_text_color(currentBigLabel, lv_color_hex(0x00D4AA), 0);
    lv_obj_set_style_text_font(currentBigLabel, &lv_font_montserrat_24, 0);
    lv_obj_align(currentBigLabel, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_add_flag(currentBigLabel, LV_OBJ_FLAG_HIDDEN);

    powerLabel = lv_label_create(card);
    lv_label_set_text(powerLabel, "0 W");
    lv_obj_set_style_text_color(powerLabel, lv_color_hex(0xF4A261), 0);
    lv_obj_set_style_text_font(powerLabel, &lv_font_montserrat_18, 0);
    lv_obj_align(powerLabel, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_obj_t *card2 = createCard(tab, 350, 300, LV_ALIGN_RIGHT_MID, -20);
    lv_obj_t *lbl2 = lv_label_create(card2);
    lv_label_set_text(lbl2, "RELAIS ACTIFS");
    lv_obj_set_style_text_color(lbl2, lv_color_hex(0x8D99AE), 0);
    lv_obj_set_style_text_font(lbl2, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl2, LV_ALIGN_TOP_MID, 0, 16);

    currentLabel = lv_label_create(card2);
    lv_label_set_text(currentLabel, "0 / 8");
    lv_obj_set_style_text_color(currentLabel, lv_color_hex(0xF4A261), 0);
    lv_obj_set_style_text_font(currentLabel, &lv_font_montserrat_48, 0);
    lv_obj_align(currentLabel, LV_ALIGN_CENTER, 0, 10);
}

static void buildRelaisTab(lv_obj_t *tab) {
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(tab, 20, 0);
    lv_obj_set_style_pad_row(tab, 20, 0);

    const char *names[NB_RELAIS] = {"Eclairage", "Prises", "Compresseur", "Chauffage",
                                    "Ventilation", "Etabli", "Chargeur", "Secours"};
    const char *icons[NB_RELAIS] = {"*", "#", "@", "+", "~", "=", "%", "!"};

    for (int i = 0; i < NB_RELAIS; i++) {
        lv_obj_t *btn = lv_btn_create(tab);
        lv_obj_set_size(btn, 165, 150);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_event_cb(btn, relaisEventHandler, LV_EVENT_CLICKED, NULL);
        lv_obj_set_user_data(btn, (void *)(intptr_t)i);
        lv_obj_set_style_radius(btn, 16, 0);
        lv_obj_set_style_shadow_width(btn, 8, 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);

        lv_obj_t *icon = lv_label_create(btn);
        lv_label_set_text(icon, icons[i]);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_40, 0);
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 18);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, names[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -14);

        relaisButtons[i] = btn;
    }
}

static void buildMesuresTab(lv_obj_t *tab) {
    chart = lv_chart_create(tab);
    lv_obj_set_size(chart, 740, 340);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x14264A), 0);
    lv_obj_set_style_radius(chart, 16, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_shadow_width(chart, 10, 0);
    lv_obj_set_style_shadow_opa(chart, LV_OPA_30, 0);

    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 300);
    lv_chart_set_point_count(chart, 60);
    lv_chart_set_div_line_count(chart, 5, 10);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x2A3F5F), LV_PART_MAIN);
    chartSeries = lv_chart_add_series(chart, lv_color_hex(0x00D4AA), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_series_width(chart, 3, chartSeries);
    lv_chart_set_next_value(chart, chartSeries, 0);

    lv_obj_t *title = lv_label_create(tab);
    lv_label_set_text(title, "HISTORIQUE COURANT (A) - 60 DERNIERES MESURES");
    lv_obj_set_style_text_color(title, lv_color_hex(0x8D99AE), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);
}

static void buildScenariosTab(lv_obj_t *tab) {
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(tab, 20, 0);
    lv_obj_set_style_pad_row(tab, 20, 0);

    const char *scenarios[] = {"Mode Travail", "Mode Nettoyage", "Tout OFF", "Mode Nuit", "Urgence", "Presence"};
    int colors[] = {0x00D4AA, 0x4CC9F0, 0xE63946, 0xF4A261, 0x9B5DE5, 0x90E0A0};

    for (int i = 0; i < 6; i++) {
        lv_obj_t *btn = lv_btn_create(tab);
        lv_obj_set_size(btn, 225, 110);
        lv_obj_set_style_bg_color(btn, lv_color_hex(colors[i]), 0);
        lv_obj_set_style_radius(btn, 16, 0);
        lv_obj_set_style_shadow_width(btn, 8, 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_40, 0);
        lv_obj_add_event_cb(btn, scenarioEventHandler, LV_EVENT_CLICKED, NULL);
        lv_obj_set_user_data(btn, (void *)scenarios[i]);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, scenarios[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x0A1420), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_center(lbl);
    }
}

static void buildLockScreen(lv_obj_t *parent) {
    lockScreen = lv_obj_create(parent);
    lv_obj_set_size(lockScreen, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_align(lockScreen, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(lockScreen, lv_color_hex(0x050A12), 0);
    lv_obj_set_style_bg_opa(lockScreen, LV_OPA_90, 0);
    lv_obj_add_flag(lockScreen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lockScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(lockScreen, unlockEventHandler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(lockScreen);
    lv_label_set_text(lbl, "VERROUILLE\nTouchez pour deverrouiller");
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lbl);
}

void uiInit(RelaisCallback rCb, ScenarioCallback sCb) {
    relaisCb = rCb;
    scenarioCb = sCb;
    lastActivity = lv_tick_get();

    lv_style_init(&styleBtnOn);
    lv_style_set_bg_color(&styleBtnOn, lv_color_hex(0x00D4AA));
    lv_style_set_text_color(&styleBtnOn, lv_color_hex(0x0A1420));
    lv_style_set_shadow_color(&styleBtnOn, lv_color_hex(0x00D4AA));
    lv_style_set_shadow_opa(&styleBtnOn, LV_OPA_50);
    lv_style_set_shadow_spread(&styleBtnOn, 4);

    lv_style_init(&styleBtnOff);
    lv_style_set_bg_color(&styleBtnOff, lv_color_hex(0x2A3F5F));
    lv_style_set_text_color(&styleBtnOff, lv_color_hex(0xC0CBD8));

    lv_style_init(&styleCard);
    lv_style_set_bg_color(&styleCard, lv_color_hex(0x14264A));
    lv_style_set_radius(&styleCard, 16);
    lv_style_set_border_width(&styleCard, 0);
    lv_style_set_shadow_width(&styleCard, 14);
    lv_style_set_shadow_color(&styleCard, lv_color_black());
    lv_style_set_shadow_opa(&styleCard, LV_OPA_40);
    lv_style_set_shadow_ofs_y(&styleCard, 6);

    screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0A1420), 0);
    lv_obj_add_event_cb(screen, [](lv_event_t *e) { touchActivity(); }, LV_EVENT_PRESSED, NULL);

    createHeader(screen);
    alertBanner = createAlertBanner(screen);

    tabview = lv_tabview_create(screen, LV_DIR_TOP, 50);
    lv_obj_set_size(tabview, SCREEN_WIDTH, SCREEN_HEIGHT - 56);
    lv_obj_align(tabview, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *tabBar = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_style_bg_color(tabBar, lv_color_hex(0x14264A), 0);
    lv_obj_set_style_text_color(tabBar, lv_color_hex(0xC0CBD8), 0);
    lv_obj_set_style_text_color(tabBar, lv_color_hex(0x00D4AA), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_pad_all(tabBar, 8, 0);

    buildHomeTab(createTab("Accueil"));
    buildRelaisTab(createTab("Relais"));
    buildMesuresTab(createTab("Mesures"));
    buildScenariosTab(createTab("Scenarios"));

    for (int i = 0; i < NB_RELAIS; i++) {
        lv_obj_add_style(relaisButtons[i], &styleBtnOff, 0);
        lv_obj_add_style(relaisButtons[i], &styleBtnOn, LV_STATE_CHECKED);
    }

    buildLockScreen(screen);
}

void uiUpdateCurrent(float amps) {
    if (currentBigLabel) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f A", amps);
        lv_label_set_text(currentBigLabel, buf);
    }
    if (arc) {
        lv_arc_set_value(arc, (int)(amps * 10));
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f A", amps);
        lv_label_set_text(arcLabel, buf);
        lv_color_t c = amps > ALERT_THRESHOLD_A ? lv_color_hex(0xE63946) : lv_color_hex(0x00D4AA);
        lv_obj_set_style_arc_color(arc, c, LV_PART_INDICATOR);
        lv_obj_set_style_text_color(arcLabel, c, 0);
    }
    if (powerLabel) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d W", (int)(amps * 230));
        lv_label_set_text(powerLabel, buf);
    }
    if (alertBanner) {
        if (amps > ALERT_THRESHOLD_A) {
            lv_obj_remove_flag(alertBanner, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_height(alertBanner, 36);
        } else {
            lv_obj_add_flag(alertBanner, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_height(alertBanner, 0);
        }
    }
    if (chart && chartSeries) {
        lv_chart_set_next_value(chart, chartSeries, (lv_coord_t)(amps * 10));
    }
}

void uiUpdateRelais(int index, bool state) {
    if (index < 0 || index >= NB_RELAIS || !relaisButtons[index]) return;
    if (state) lv_obj_add_state(relaisButtons[index], LV_STATE_CHECKED);
    else lv_obj_clear_state(relaisButtons[index], LV_STATE_CHECKED);

    int active = 0;
    for (int i = 0; i < NB_RELAIS; i++) {
        if (lv_obj_has_state(relaisButtons[i], LV_STATE_CHECKED)) active++;
    }
    if (currentLabel) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d / %d", active, NB_RELAIS);
        lv_label_set_text(currentLabel, buf);
    }
}

void uiUpdateClock(const char *timeStr) {
    if (clockLabel) lv_label_set_text(clockLabel, timeStr);
}

void uiSetLocked(bool lock) {
    locked = lock;
    if (lockScreen) {
        if (lock) lv_obj_remove_flag(lockScreen, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(lockScreen, LV_OBJ_FLAG_HIDDEN);
    }
}

void uiTick() {
    if (!locked && lv_tick_get() - lastActivity > LOCK_TIMEOUT_MS) {
        uiSetLocked(true);
    }
}
