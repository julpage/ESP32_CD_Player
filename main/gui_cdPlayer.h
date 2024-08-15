#ifndef __GUI_CD_PLAYER_H_
#define __GUI_CD_PLAYER_H_

extern lv_obj_t *chart_left;
extern lv_chart_series_t *ser_left;
extern lv_obj_t *chart_right;
extern lv_chart_series_t *ser_right;

extern lv_obj_t *bar_meterLeft;
extern lv_obj_t *bar_meterRight;

void gui_player_init();

void gui_setDriveModel(const char *str);
void gui_setDriveState(const char *str);
void gui_setAlbumTitle(const char *str);
void gui_setTrackTitle(const char *title, const char *performer);
void gui_setEmphasis(bool en);
void gui_setTime(hmsf_t current, hmsf_t total);
void gui_setProgress(uint32_t current, uint32_t total);
void gui_setPlayState(const char *str);
void gui_setVolume(int vol);
void gui_setTrackNum(int current, int total);
void gui_setMeter(int l, int r);

#endif
