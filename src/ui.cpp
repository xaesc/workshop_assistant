#include <lvgl.h>
#include "ui.h"
#include "config.h"

static RelaisCallback relaisCb = nullptr;
static ScenarioCallback scenarioCb = nullptr;

static lv_obj_t *screen;
static lv_obj_t *tabview;
static lv_obj_t *currentLabel;
static lv_obj_t *currentBigLabel;
static lv_obj_t *relaisButtons[NB_RELAIS];
static lv_obj_t *relaisLabels[NB_RELAIS];
static lv_obj_t *chart;
static lv_chart_series_t *chartSeries;

static lv_style_t styleBtnOn;
static lv_style_t styleBtnOff;
static lv_style_t styleTab;

static void relaisEventHandler(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    if (relaisCb) relaisCb(idx, !lv_obj_has_state(relaisButtons[idx], LV_STATE_CHECKED));
}

static void scenarioEventHandler(lv_event_t *e) {
    const char *name = (const char *)lv_obj_get_user_data(lv_event_get_target(e));
    if (scenarioCb) scenarioCb(name);
}

static void createHeader(lv_obj_t *parent) {
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, SCREEN_WIDTH, 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x1E3A5F), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "GESTION ATELIER");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_center(title);
}

static lv_obj_t *createTab(const char *name) {
    return lv_tabview_add_tab(tabview, name);
}

static void buildHomeTab(lv_obj_t *tab) {
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x0D1B2A), 0);

    lv_obj_t *card = lv_obj_create(tab);
    lv_obj_set_size(card, 340, 180);
    lv_obj_align(card, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1B263B), 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 0, 0);

    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, "Consommation actuelle");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x8D99AE), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 20);

    currentBigLabel = lv_label_create(card);
    lv_label_set_text(currentBigLabel, "0.00 A");
    lv_obj_set_style_text_color(currentBigLabel, lv_color_hex(0x00D4AA), 0);
    lv_obj_set_style_text_font(currentBigLabel, &lv_font_montserrat_40, 0);
    lv_obj_align(currentBigLabel, LV_ALIGN_CENTER, 0, 15);

    lv_obj_t *card2 = lv_obj_create(tab);
    lv_obj_set_size(card2, 340, 180);
    lv_obj_align(card2, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_set_style_bg_color(card2, lv_color_hex(0x1B263B), 0);
    lv_obj_set_style_radius(card2, 12, 0);
    lv_obj_set_style_border_width(card2, 0, 0);

    lv_obj_t *lbl2 = lv_label_create(card2);
    lv_label_set_text(lbl2, "Relais actifs");
    lv_obj_set_style_text_color(lbl2, lv_color_hex(0x8D99AE), 0);
    lv_obj_align(lbl2, LV_ALIGN_TOP_MID, 0, 20);

    currentLabel = lv_label_create(card2);
    lv_label_set_text(currentLabel, "0 / 8");
    lv_obj_set_style_text_color(currentLabel, lv_color_hex(0xF4A261), 0);
    lv_obj_set_style_text_font(currentLabel, &lv_font_montserrat_40, 0);
    lv_obj_align(currentLabel, LV_ALIGN_CENTER, 0, 15);
}

static void buildRelaisTab(lv_obj_t *tab) {
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(tab, 15, 0);

    const char *names[NB_RELAIS] = {"Éclairage", "Prises", "Compresseur", "Chauffage",
                                    "Ventilation", "Établi", "Chargeur", "Secours"};

    for (int i = 0; i < NB_RELAIS; i++) {
        lv_obj_t *btn = lv_btn_create(tab);
        lv_obj_set_size(btn, 160, 140);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_event_cb(btn, relaisEventHandler, LV_EVENT_CLICKED, NULL);
        lv_obj_set_user_data(btn, (void *)(intptr_t)i);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, names[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_center(lbl);

        relaisButtons[i] = btn;
        relaisLabels[i] = lbl;
    }
}

static void buildMesuresTab(lv_obj_t *tab) {
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x0D1B2A), 0);

    chart = lv_chart_create(tab);
    lv_obj_set_size(chart, 740, 320);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x1B263B), 0);
    lv_obj_set_style_radius(chart, 12, 0);
    lv_obj_set_style_border_width(chart, 0, 0);

    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 30);
    lv_chart_set_point_count(chart, 60);
    lv_chart_set_div_line_count(chart, 5, 8);
    chartSeries = lv_chart_add_series(chart, lv_color_hex(0x00D4AA), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_next_value(chart, chartSeries, 0);

    lv_obj_t *title = lv_label_create(tab);
    lv_label_set_text(title, "Historique courant (A) - 60 dernières mesures");
    lv_obj_set_style_text_color(title, lv_color_hex(0x8D99AE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
}

static void buildScenariosTab(lv_obj_t *tab) {
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    const char *scenarios[] = {"Mode Travail", "Mode Nettoyage", "Tout OFF", "Mode Nuit", "Urgence", "Présence"};
    int colors[] = {0x00D4AA, 0x4CC9F0, 0xE63946, 0xF4A261, 0x9B5DE5, 0xF1FA8C};

    for (int i = 0; i < 6; i++) {
        lv_obj_t *btn = lv_btn_create(tab);
        lv_obj_set_size(btn, 220, 100);
        lv_obj_set_style_bg_color(btn, lv_color_hex(colors[i]), 0);
        lv_obj_set_style_radius(btn, 12, 0);
        lv_obj_add_event_cb(btn, scenarioEventHandler, LV_EVENT_CLICKED, NULL);
        lv_obj_set_user_data(btn, (void *)scenarios[i]);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, scenarios[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x0D1B2A), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_center(lbl);
    }
}

void uiInit(RelaisCallback rCb, ScenarioCallback sCb) {
    relaisCb = rCb;
    scenarioCb = sCb;

    lv_style_init(&styleBtnOn);
    lv_style_set_bg_color(&styleBtnOn, lv_color_hex(0x00D4AA));
    lv_style_set_text_color(&styleBtnOn, lv_color_hex(0x0D1B2A));

    lv_style_init(&styleBtnOff);
    lv_style_set_bg_color(&styleBtnOff, lv_color_hex(0x415A77));
    lv_style_set_text_color(&styleBtnOff, lv_color_white());

    screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0D1B2A), 0);

    createHeader(screen);

    tabview = lv_tabview_create(screen, LV_DIR_TOP, 45);
    lv_obj_set_size(tabview, SCREEN_WIDTH, SCREEN_HEIGHT - 50);
    lv_obj_align(tabview, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *tabBar = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_style_bg_color(tabBar, lv_color_hex(0x1E3A5F), 0);
    lv_obj_set_style_text_color(tabBar, lv_color_white(), 0);

    buildHomeTab(createTab("Accueil"));
    buildRelaisTab(createTab("Relais"));
    buildMesuresTab(createTab("Mesures"));
    buildScenariosTab(createTab("Scénarios"));

    for (int i = 0; i < NB_RELAIS; i++) {
        lv_obj_add_style(relaisButtons[i], &styleBtnOff, 0);
        lv_obj_add_style(relaisButtons[i], &styleBtnOff, LV_STATE_CHECKED);
    }
}

void uiUpdateCurrent(float amps) {
    if (!currentBigLabel) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f A", amps);
    lv_label_set_text(currentBigLabel, buf);
    lv_chart_set_next_value(chart, chartSeries, (lv_coord_t)(amps * 10));
}

void uiUpdateRelais(int index, bool state) {
    if (index < 0 || index >= NB_RELAIS || !relaisButtons[index]) return;
    if (state) {
        lv_obj_add_state(relaisButtons[index], LV_STATE_CHECKED);
        lv_obj_add_style(relaisButtons[index], &styleBtnOn, LV_STATE_CHECKED);
        lv_obj_remove_style(relaisButtons[index], &styleBtnOff, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(relaisButtons[index], LV_STATE_CHECKED);
        lv_obj_add_style(relaisButtons[index], &styleBtnOff, LV_STATE_CHECKED);
        lv_obj_remove_style(relaisButtons[index], &styleBtnOn, LV_STATE_CHECKED);
    }

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
