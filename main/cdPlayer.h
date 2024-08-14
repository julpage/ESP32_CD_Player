#ifndef __CD_PLAYER_H_
#define __CD_PLAYER_H_

typedef struct
{
    uint8_t trackNum;
    uint32_t lbaBegin;
    uint32_t trackDuration;
    uint8_t preEmphasis;
    char *title;
    char *performer;
} cdplayer_trackInfo_t;

typedef struct
{
    char vendor[9];
    char product[17];
    uint8_t discInserted;
    uint8_t trayClosed;
    uint8_t discIsCD;
    uint8_t trackCount;
    cdplayer_trackInfo_t trackList[99];
    uint8_t cdTextAvalibale;
    uint8_t readyToPlay;
    char *albumTitle;
    char *albumPerformer;
    char *strBuf_titles;     // need to free
    char *strBuf_performers; // need to free
} cdplayer_driveInfo_t;

typedef struct cdPlayer
{
    int8_t volume;
    uint8_t playing;
    uint8_t fastForwarding;
    uint8_t fastBackwarding;
    int8_t playingTrackIndex;
    int32_t readFrameCount;

} cdplayer_playerInfo_t;

typedef struct
{
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t frame;
} hmsf_t;

extern cdplayer_driveInfo_t cdplayer_driveInfo;
extern cdplayer_playerInfo_t cdplayer_playerInfo;

void cdplay_init();
hmsf_t cdplay_frameToHmsf(uint32_t frame);

#endif
