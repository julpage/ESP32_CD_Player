#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "cdPlayer.h"
#include "gui_cdPlayer.h"

#define AREA_STATUS_BAR_HEIGHT 25
#define AREA_OSCILLOSCOPE_HEIGHT 65
lv_style_t style_preEmphasis;

lv_obj_t *area_topBar;
lv_obj_t *area_bottomBar;
lv_obj_t *area_player;
lv_obj_t *area_left;
lv_obj_t *area_right;

lv_obj_t *lb_driveModel;
lv_obj_t *lb_driveState;
lv_obj_t *lb_albumTitle;
lv_obj_t *lb_trackNumber;
lv_obj_t *lb_trackTitle;
lv_obj_t *lb_trackPerformer;
lv_obj_t *lb_preEmphasized;
lv_obj_t *lb_time;
lv_obj_t *lb_duration;

lv_obj_t *bar_playProgress;
lv_obj_t *bar_meterLeft;
lv_obj_t *bar_meterRight;

lv_obj_t *chart_left;
lv_chart_series_t *ser_left;
lv_obj_t *chart_right;
lv_chart_series_t *ser_right;

lv_obj_t *lb_playState;
lv_obj_t *lb_volume;

void gui_player_init()
{
    lv_color_t color_background = lv_color_make(0x18, 0x18, 0x18);
    lv_color_t color_foreground = lv_color_make(0x1f, 0x1f, 0x1f);

    // 界面
    // main screen
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, color_background, 0);

    /***********************
     * 背景 - 状态栏
     * Background - Status Bar
     */

    int size_barWidth = 240;
    int size_barHeight = AREA_STATUS_BAR_HEIGHT;

    // 顶部状态栏
    // top
    static lv_style_t style_topBar;
    lv_style_init(&style_topBar);
    lv_style_set_radius(&style_topBar, 0);
    lv_style_set_bg_opa(&style_topBar, LV_OPA_COVER);
    lv_style_set_bg_color(&style_topBar, color_background);
    lv_style_set_border_width(&style_topBar, 1);
    lv_style_set_border_opa(&style_topBar, LV_OPA_COVER);
    lv_style_set_border_color(&style_topBar, lv_color_make(0x2e, 0x2e, 0x2e));
    lv_style_set_pad_all(&style_topBar, 0);

    area_topBar = lv_obj_create(screen);
    lv_obj_add_style(area_topBar, &style_topBar, 0);
    lv_obj_set_size(area_topBar, size_barWidth, size_barHeight);
    lv_obj_set_pos(area_topBar, 0, 0);
    lv_obj_set_scrollbar_mode(area_topBar, LV_SCROLLBAR_MODE_OFF);

    // 底部状态栏
    // bottom
    static lv_style_t style_bottomBar;
    lv_style_init(&style_bottomBar);
    lv_style_set_radius(&style_bottomBar, 0);
    lv_style_set_bg_opa(&style_bottomBar, LV_OPA_COVER);
    lv_style_set_bg_color(&style_bottomBar, lv_color_make(0x86, 0x1b, 0x2d));
    lv_style_set_border_width(&style_bottomBar, 1);
    lv_style_set_border_opa(&style_bottomBar, LV_OPA_COVER);
    // lv_style_set_border_color(&style_bottomBar, lv_color_make(0x2e, 0x2e, 0x2e));
    lv_style_set_border_color(&style_bottomBar, lv_color_make(0x86, 0x1b, 0x2d));
    lv_style_set_pad_all(&style_bottomBar, 0);

    area_bottomBar = lv_obj_create(screen);
    lv_obj_add_style(area_bottomBar, &style_bottomBar, 0);
    lv_obj_set_size(area_bottomBar, size_barWidth, size_barHeight);
    lv_obj_set_pos(area_bottomBar, 0, 240 - size_barHeight);
    lv_obj_set_scrollbar_mode(area_bottomBar, LV_SCROLLBAR_MODE_OFF);

    /***********************
     * 背景 - 播放器
     * Background - Player
     */
    int size_playerWidth = 240;
    int size_playerHeight = 240 - 2 * AREA_STATUS_BAR_HEIGHT - AREA_OSCILLOSCOPE_HEIGHT;

    static lv_style_t style_player;
    lv_style_init(&style_player);
    lv_style_set_radius(&style_player, 0);
    lv_style_set_bg_opa(&style_player, LV_OPA_COVER);
    lv_style_set_bg_color(&style_player, color_foreground);
    lv_style_set_border_width(&style_player, 1);
    lv_style_set_border_opa(&style_player, LV_OPA_COVER);
    lv_style_set_border_color(&style_player, lv_color_make(0x00, 0x78, 0xd4));
    lv_style_set_pad_all(&style_player, 0);

    area_player = lv_obj_create(screen);
    lv_obj_add_style(area_player, &style_player, 0);
    lv_obj_set_size(area_player, size_playerWidth, size_playerHeight);
    lv_obj_set_pos(area_player, 0, AREA_STATUS_BAR_HEIGHT);

    /***********************
     * 背景 - 示波器
     * Background - Oscilloscope
     */
    int size_oscilloscopeWidth = 240 / 2;
    int size_oscilloscopeHeight = AREA_OSCILLOSCOPE_HEIGHT;

    area_left = lv_obj_create(screen);
    lv_obj_set_scrollbar_mode(area_left, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(area_left, &style_topBar, 0);
    lv_obj_set_size(area_left, size_oscilloscopeWidth, size_oscilloscopeHeight);
    lv_obj_align_to(area_left, area_player, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

    area_right = lv_obj_create(screen);
    lv_obj_set_scrollbar_mode(area_right, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(area_right, &style_topBar, 0);
    lv_obj_set_size(area_right, size_oscilloscopeWidth, size_oscilloscopeHeight);
    lv_obj_align_to(area_right, area_player, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0);

    /***********************
     * 光驱型号
     * CD drive model
     */
    static lv_style_t style1;
    lv_style_init(&style1);
    lv_style_set_text_color(&style1, lv_color_white());
    lv_style_set_text_font(&style1, &lv_font_montserrat_14);

    lb_driveModel = lv_label_create(area_topBar);
    lv_obj_add_style(lb_driveModel, &style1, 0);
    gui_setDriveModel("XXX");

    /***********************
     * 光驱状态
     * CD drive state
     */
    lb_driveState = lv_label_create(area_topBar);
    lv_obj_add_style(lb_driveState, &style1, 0);
    gui_setDriveState("Start up");

    /***********************
     * 专辑名
     * album title
     */
    lb_albumTitle = lv_label_create(area_player);
    lv_obj_set_style_text_color(lb_albumTitle, lv_color_make(0xc5, 0x86, 0xc0), 0);
    lv_obj_set_style_text_font(lb_albumTitle, &lv_font_montserrat_16, 0);
    lv_obj_set_width(lb_albumTitle, 178);
    lv_obj_set_height(lb_albumTitle, 20);
    lv_label_set_long_mode(lb_albumTitle, LV_LABEL_LONG_DOT);
    gui_setAlbumTitle("Album-Performer longlonglong");

    /***********************
     * 曲名
     * track title and performer
     */
    lb_trackTitle = lv_label_create(area_player);
    lv_obj_set_style_text_color(lb_trackTitle, lv_color_make(0xff, 0xff, 0xff), 0);
    lv_obj_set_style_text_font(lb_trackTitle, &lv_font_montserrat_26, 0);
    lv_obj_set_width(lb_trackTitle, 230);
    lv_obj_set_height(lb_trackTitle, 30);
    lv_obj_set_style_text_align(lb_trackTitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(lb_trackTitle, LV_LABEL_LONG_DOT);

    lb_trackPerformer = lv_label_create(area_player);
    lv_obj_set_style_text_color(lb_trackPerformer, lv_color_make(0xdd, 0xdd, 0xdd), 0);
    lv_obj_set_style_text_font(lb_trackPerformer, &lv_font_montserrat_16, 0);
    lv_obj_set_width(lb_trackPerformer, 230);
    lv_obj_set_height(lb_trackPerformer, 20);
    lv_obj_set_style_text_align(lb_trackPerformer, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(lb_trackPerformer, LV_LABEL_LONG_DOT);

    gui_setTrackTitle("What title long long long long long", "Who know long long long long long");

    /***********************
     * 预加重
     * pre-Emphasis indicator
     */
    lv_style_init(&style_preEmphasis);
    lv_style_set_border_width(&style_preEmphasis, 0);
    lv_style_set_bg_opa(&style_preEmphasis, LV_OPA_COVER);
    lv_style_set_bg_color(&style_preEmphasis, lv_color_make(0x00, 0x78, 0xd4));
    lv_style_set_radius(&style_preEmphasis, 2);
    lv_style_set_pad_all(&style_preEmphasis, 2);
    lv_style_set_pad_left(&style_preEmphasis, 4);
    lv_style_set_pad_right(&style_preEmphasis, 4);
    lv_style_set_text_color(&style_preEmphasis, lv_color_white());
    lv_style_set_text_font(&style_preEmphasis, &lv_font_montserrat_14);

    lb_preEmphasized = lv_label_create(area_player);
    lv_obj_add_style(lb_preEmphasized, &style_preEmphasis, 0);

    gui_setEmphasis(true);

    /***********************
     * 播放时间
     * Play time
     */
    lb_time = lv_label_create(area_player);
    lv_obj_set_style_text_color(lb_time, lv_color_make(0xdc, 0xdc, 0xaa), LV_PART_MAIN);
    lv_obj_set_style_text_font(lb_time, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_align(lb_time, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    lb_duration = lv_label_create(area_player);
    lv_obj_set_style_text_color(lb_duration, lv_color_make(0xdc, 0xdc, 0xaa), LV_PART_MAIN);
    lv_obj_set_style_text_font(lb_duration, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_align(lb_duration, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    hmsf_t a = {0, 1, 2, 3};
    hmsf_t b = {4, 5, 6, 7};
    gui_setTime(a, b);

    /***********************
     * 进度条
     * progress bar
     */
    bar_playProgress = lv_bar_create(area_player);
    lv_obj_set_size(bar_playProgress, 230, 11);
    lv_obj_align_to(bar_playProgress, area_player, LV_ALIGN_BOTTOM_MID, 0, -5);

    gui_setProgress(111, 999);

    /***********************
     * 播放状态
     * play state
     */
    lb_playState = lv_label_create(area_bottomBar);
    // lv_obj_set_style_text_color(lb_playState, lv_color_make(0xf1, 0xc4, 0x0f), LV_PART_MAIN);
    // lv_obj_set_style_text_font(lb_playState, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_add_style(lb_playState, &style1, 0);

    gui_setPlayState(LV_SYMBOL_STOP);

    /***********************
     * 音量
     * volume
     */
    lb_volume = lv_label_create(area_bottomBar);
    // lv_obj_set_style_text_color(lb_volume, lv_color_make(0xf1, 0xc4, 0x0f), LV_PART_MAIN);
    // lv_obj_set_style_text_font(lb_volume, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_add_style(lb_volume, &style1, 0);

    gui_setVolume(99);

    /***********************
     * 曲目数
     * number of tracks
     */
    lb_trackNumber = lv_label_create(area_bottomBar);
    lv_obj_add_style(lb_trackNumber, &style1, 0);
    gui_setTrackNum(11, 99);

    /***********************
     * 电平表
     * level meter
     */
    int size_meterWidth = 5;

    static lv_style_t style_indic;
    lv_style_init(&style_indic);
    lv_style_set_radius(&style_indic, 0);
    lv_style_set_bg_grad_dir(&style_indic, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indic, lv_color_make(0xf5, 0x4d, 0x46));
    lv_style_set_bg_grad_color(&style_indic, lv_color_make(0x37, 0x8c, 0x46));

    static lv_style_t style_bg;
    lv_style_init(&style_bg);
    lv_style_set_radius(&style_bg, 0);
    lv_style_set_border_width(&style_bg, 0);
    lv_style_set_pad_all(&style_bg, 0);

    bar_meterLeft = lv_bar_create(area_left);
    lv_obj_add_style(bar_meterLeft, &style_bg, 0);
    lv_obj_add_style(bar_meterLeft, &style_indic, LV_PART_INDICATOR);
    lv_bar_set_range(bar_meterLeft, 35, 90); // 32768:1 = 90.3dB
    lv_obj_set_size(bar_meterLeft, size_meterWidth, AREA_OSCILLOSCOPE_HEIGHT);
    lv_obj_align_to(bar_meterLeft, area_left, LV_ALIGN_RIGHT_MID, 0, 0);

    bar_meterRight = lv_bar_create(area_right);
    lv_obj_add_style(bar_meterRight, &style_bg, 0);
    lv_obj_add_style(bar_meterRight, &style_indic, LV_PART_INDICATOR);
    lv_bar_set_range(bar_meterRight, 35, 90);
    lv_obj_set_size(bar_meterRight, size_meterWidth, AREA_OSCILLOSCOPE_HEIGHT);
    lv_obj_align_to(bar_meterRight, area_right, LV_ALIGN_LEFT_MID, 0, 0);

    gui_setMeter(56, 78);

    /***********************
     * 示波器
     * oscilloscope
     */
    size_oscilloscopeWidth = 240 / 2 - size_meterWidth - 1;
    size_oscilloscopeHeight = AREA_OSCILLOSCOPE_HEIGHT;

    // 背景样式
    static lv_style_t style_oscilloscope;
    lv_style_init(&style_oscilloscope);
    lv_style_set_radius(&style_oscilloscope, 0);
    lv_style_set_pad_all(&style_oscilloscope, 0);
    lv_style_set_bg_color(&style_oscilloscope, color_background);
    lv_style_set_border_color(&style_oscilloscope, lv_color_make(0x55, 0x55, 0x55));
    lv_style_set_border_width(&style_oscilloscope, 1);
    lv_style_set_line_color(&style_oscilloscope, lv_color_make(0x33, 0x33, 0x33));

    // 折线样式
    static lv_style_t style_oscilloscopeLine;
    lv_style_init(&style_oscilloscopeLine);
    lv_style_set_line_width(&style_oscilloscopeLine, 1);

    // lv_color_t lineColor = lv_color_make(0x00, 0x8c, 0x3a);
    lv_color_t lineColor = lv_color_make(0x43, 0xd9, 0x96);

    int32_t maxPoint = 29000;

    chart_left = lv_chart_create(area_left);
    lv_obj_add_style(chart_left, &style_oscilloscope, LV_PART_MAIN);
    lv_obj_add_style(chart_left, &style_oscilloscopeLine, LV_PART_ITEMS);
    lv_obj_set_style_size(chart_left, 0, LV_PART_INDICATOR); // 不显示点
    lv_obj_set_size(chart_left, size_oscilloscopeWidth, size_oscilloscopeHeight);
    lv_obj_align_to(chart_left, area_left, LV_ALIGN_LEFT_MID, -1, 0);
    lv_chart_set_div_line_count(chart_left, 3, 6);
    lv_chart_set_update_mode(chart_left, LV_CHART_UPDATE_MODE_SHIFT);             // 滚动模式
    lv_chart_set_point_count(chart_left, size_oscilloscopeWidth);                 // 点数
    lv_chart_set_range(chart_left, LV_CHART_AXIS_PRIMARY_Y, -maxPoint, maxPoint); // 范围

    chart_right = lv_chart_create(area_right);
    lv_obj_add_style(chart_right, &style_oscilloscope, LV_PART_MAIN);
    lv_obj_add_style(chart_right, &style_oscilloscopeLine, LV_PART_ITEMS);
    lv_obj_set_style_size(chart_right, 0, LV_PART_INDICATOR);
    lv_obj_set_size(chart_right, size_oscilloscopeWidth, size_oscilloscopeHeight);
    lv_obj_align_to(chart_right, area_right, LV_ALIGN_RIGHT_MID, 1, 0);
    lv_chart_set_div_line_count(chart_right, 3, 6);
    lv_chart_set_update_mode(chart_right, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_point_count(chart_right, size_oscilloscopeWidth);
    lv_chart_set_range(chart_right, LV_CHART_AXIS_PRIMARY_Y, -maxPoint, maxPoint);

    ser_left = lv_chart_add_series(chart_left, lineColor, LV_CHART_AXIS_PRIMARY_Y);
    ser_right = lv_chart_add_series(chart_right, lineColor, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart_left, ser_left, 0);
    lv_chart_set_all_value(chart_right, ser_right, 0);
}

void gui_setDriveModel(const char *str)
{
    char *oldStr = lv_label_get_text(lb_driveModel);

    if (strcmp(oldStr, str) == 0)
        return;

    lv_label_set_text(lb_driveModel, str);
    lv_obj_align_to(lb_driveModel, area_topBar, LV_ALIGN_LEFT_MID, 5, 0);
}

void gui_setDriveState(const char *str)
{
    char *oldStr = lv_label_get_text(lb_driveState);

    if (strcmp(oldStr, str) == 0)
        return;

    lv_label_set_text(lb_driveState, str);
    lv_obj_align_to(lb_driveState, area_topBar, LV_ALIGN_RIGHT_MID, -5, 0);
}

void gui_setAlbumTitle(const char *str)
{
    char *oldStr = lv_label_get_text(lb_albumTitle);

    if (strcmp(oldStr, str) == 0)
        return;

    lv_label_set_text(lb_albumTitle, str);
    lv_obj_align_to(lb_albumTitle, area_player, LV_ALIGN_TOP_LEFT, 5, 5);
}

void gui_setTrackTitle(const char *title, const char *performer)
{
    char *oldTitle = lv_label_get_text(lb_trackTitle);
    char *oldPerformer = lv_label_get_text(lb_trackPerformer);

    if ((title != NULL) && (strcmp(oldTitle, title) != 0))
    {
        lv_label_set_text(lb_trackTitle, title);

        bool noPerformer = true;
        for (int i = 0; i < strlen(performer); i++)
        {
            if (performer[i] != ' ')
            {
                noPerformer = false;
                break;
            }
        }

        if (noPerformer)
        {
            lv_obj_set_height(lb_trackTitle, 60);
            lv_obj_align_to(lb_trackTitle, area_player, LV_ALIGN_TOP_MID, 0, 25);
        }
        else
        {
            lv_obj_set_height(lb_trackTitle, 30);
            lv_obj_align_to(lb_trackTitle, area_player, LV_ALIGN_TOP_MID, 0, 33);
        }
    }

    if ((performer != NULL) && (strcmp(oldPerformer, performer) != 0))
    {
        lv_label_set_text(lb_trackPerformer, performer);
        lv_obj_align_to(lb_trackPerformer, lb_trackTitle, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    }
}

void gui_setEmphasis(bool en)
{
    static bool oldEn = false;
    if (oldEn == en)
        return;
    oldEn = en;

    if (en)
    {
        lv_style_set_bg_opa(&style_preEmphasis, LV_OPA_COVER);
        lv_label_set_text(lb_preEmphasized, "EMPH");
    }
    else
    {
        lv_style_set_bg_opa(&style_preEmphasis, LV_OPA_TRANSP);
        lv_label_set_text(lb_preEmphasized, "");
    }
    lv_obj_align_to(lb_preEmphasized, area_player, LV_ALIGN_TOP_RIGHT, -5, 5);
}

void gui_setTime(hmsf_t current, hmsf_t total)
{
    static hmsf_t oldCurrent = {255, 255, 255, 255};
    static hmsf_t oldTotal = {255, 255, 255, 255};

    if (memcmp(&oldCurrent, &current, sizeof(hmsf_t)) != 0)
    {
        memcpy(&oldCurrent, &current, sizeof(hmsf_t));
        lv_label_set_text_fmt(lb_time, "%02d:%02d.%02d", current.minute, current.second, current.frame);
        lv_obj_align_to(lb_time, area_player, LV_ALIGN_BOTTOM_LEFT, 5, -20);
    }

    if (memcmp(&oldTotal, &total, sizeof(hmsf_t)) != 0)
    {
        memcpy(&oldTotal, &total, sizeof(hmsf_t));
        lv_label_set_text_fmt(lb_duration, "%02d:%02d.%02d", total.minute, total.second, total.frame);
        lv_obj_align_to(lb_duration, area_player, LV_ALIGN_BOTTOM_RIGHT, -5, -20);
    }
}

void gui_setProgress(uint32_t current, uint32_t total)
{
    static uint32_t oldCurrent = 0;
    static uint32_t oldToal = 0;

    if (oldCurrent == current && oldToal == total)
        return;
    oldCurrent = current;
    oldToal = total;

    if (total == 0) // prevent IntegerDivideByZero exception
        total = 1234;

    lv_bar_set_range(bar_playProgress, 0, total);
    lv_bar_set_value(bar_playProgress, current, LV_ANIM_OFF);
}

void gui_setPlayState(const char *str)
{
    char *oldStr = lv_label_get_text(lb_playState);
    if (strcmp(oldStr, str) == 0)
        return;

    lv_label_set_text(lb_playState, str);
    lv_obj_align_to(lb_playState, area_bottomBar, LV_ALIGN_LEFT_MID, 7, 0);
}

void gui_setVolume(int vol)
{
    static int oldVol = -1;
    if (oldVol == vol)
        return;

    oldVol = vol;
    lv_label_set_text_fmt(lb_volume, LV_SYMBOL_VOLUME_MAX " %02d", vol);
    lv_obj_align_to(lb_volume, area_bottomBar, LV_ALIGN_RIGHT_MID, -5, 0);
}

void gui_setTrackNum(int current, int total)
{
    static int oldCurrent = -1;
    static int oldTotal = -1;

    if (oldCurrent == current && oldTotal == total)
        return;
    oldCurrent = current;
    oldTotal = total;

    if (total == 0)
    {
        lv_label_set_text(lb_trackNumber, "");
    }
    else
    {
        lv_label_set_text_fmt(lb_trackNumber, "%d / %d", current, total);
    }
    lv_obj_align_to(lb_trackNumber, area_bottomBar, LV_ALIGN_CENTER, 0, 0);
}

void gui_setMeter(int l, int r)
{
    static int oldL = -1;
    static int oldR = -1;

    if (oldL != l)
    {
        oldL = l;
        lv_bar_set_value(bar_meterLeft, l, LV_ANIM_OFF);
    }
    if (oldR != r)
    {
        oldR = r;
        lv_bar_set_value(bar_meterRight, r, LV_ANIM_OFF);
    }
}
