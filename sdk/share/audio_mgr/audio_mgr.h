﻿/*
 * Copyright (c) 2004 ITE Technology Corp. All Rights Reserved.
 */
/** @file
 * Audio manager.
 *
 * @author
 * @version 1.0
 */
#ifndef AUDIO_H
#define AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
//#include "audioRecord.h"

#include "ite/audio.h"
#include "ite/ith.h"
#include "ite/itp.h"
#if defined(__OPENRTOS__)
//#include "mmio.h"
#endif
//#include "pal/file.h"
#include <pthread.h>

#include "sxa_dmx/sxa_dmx.h"

#include "i2s/i2s.h"
//#include "mediastreamer2/mediastream.h"

#ifdef CFG_BUILD_FFMPEG
#include "castor3player.h"
#endif

// support gb2312
//#include "gb2312.h"

#include "parsing.h"


#define SMTK_AUDIO_MP3       1
#define SMTK_AUDIO_AAC       2
#define SMTK_AUDIO_WMA       5
#define SMTK_AUDIO_AMR       8
#define SMTK_AUDIO_WAV       12
#define SMTK_AUDIO_AC3       13

#define SMTK_AUDIO_DTS_SPDIF 14
#define SMTK_AUDIO_AC3_SPDIF 15

#define SMTK_AUDIO_FLAC      16

#define SMTK_AUDIO_SPECIAL_CASE_BUFFER_SIZE   (CFG_AUDIO_SPECIAL_CASE_BUFFER_SIZE*1024)

#define SMTK_AUDIO_READ_BUFFER_THREAD
#ifdef SMTK_AUDIO_READ_BUFFER_THREAD
#define SMTK_AUDIO_MGR_READ_BUFFER_STACK_SIZE (CFG_AUDIO_MGR_THREAD_STACK_SIZE * 1024)
#define SMTK_AUDIO_READ_BUFFER_SIZE           (CFG_AUDIO_MGR_READ_BUFFER_SIZE * 1024)
#define SMTK_AUDIO_READ_BUFFER_FRAMES         4  // 400   //  4*2

typedef struct AUDIO_READ_BUFFER_TAG {               // read audio data
    int         ready;
    int         size;
    char *data;
} AUDIO_READ_BUFFER;

#endif

#ifdef ParsingMP3TotolTime
    #define SMTK_AUDIO_SET_ACCURATE_MP3_TIME
    #define SMTK_AUDIO_ACCURATE_WAIT_TIME 2000
//#define SMTK_AUDIO_SET_TIME_BY_MILLISECOND
#endif
#define SMTK_AUDIO_READ_DATA_SLOW_COUNT   100

#define SMTK_AUDIO_RECORD_PARAM_FLAG_LIMIT_TIME      (0x01 << 0)
#define SMTK_AUDIO_RECORD_PARAM_FLAG_LIMIT_SIZE      (0x01 << 1)

///////////////////////////////////////////////////
// file info
#define SMTK_AUDIO_FLAG_INFO_TITLE                   (0x01 << 0)
#define SMTK_AUDIO_FLAG_INFO_ARTIST                  (0x01 << 1)
#define SMTK_AUDIO_FLAG_INFO_ALBUM                   (0x01 << 2)
#define SMTK_AUDIO_FLAG_INFO_YEAR                    (0x01 << 3)
#define SMTK_AUDIO_FLAG_INFO_TIME                    (0x01 << 4)
#define SMTK_AUDIO_FLAG_INFO_GENRE                   (0x01 << 5)
#define SMTK_AUDIO_FLAG_INFO_ATTATCHED_PICTURE       (0x01 << 6)
#define SMTK_AUDIO_FLAG_INFO_COMMENT                 (0x01 << 7)
#define SMTK_AUDIO_FLAG_INFO_TOOLS                   (0x01 << 8)
////////////////////////////////////////////////////
#define MMP_BOOL                                     unsigned char
#define MMP_CHAR                                     char
//#define MMP_WCHAR                                    wchar_t
#define MMP_UINT8                                    unsigned char
#define MMP_UINT16                                   unsigned short
#define MMP_UINT32                                   unsigned int
#define   MMP_UINT                                   unsigned int
#define   MMP_INT                                    int
#define   MMP_INT16                                  short
#define   MMP_INT32                                  int
// for build pass
#define MMP_UINT64                                   unsigned int
#define MMP_INT64                                    signed long long

#define   MMP_ULONG                                  unsigned long
#define   MMP_LONG                                   long
typedef void PAL_FILE;                  /**< File handle */

//#define   PAL_FILE FILE
#define   MMP_NULL                                   0

#define PAL_T(x)     L ## x /**< Unicode string type */
#define PalAssert(e) ((void) 0)

#define PAL_HEAP_DEFAULT                             0
#define MMP_FALSE                                    0
#define MMP_TRUE                                     1
//////////////////////////////////////////////
//Mp3
//id3 v1
#define SMTK_AUDIO_MP3_ID3_V1_HEADER_LENGTH          3
#define SMTK_AUDIO_MP3_ID3_V1_TITLE_LENGTH           30
#define SMTK_AUDIO_MP3_ID3_V1_ARTIST_LENGTH          30
#define SMTK_AUDIO_MP3_ID3_V1_ALBUM_LENGTH           30
#define SMTK_AUDIO_MP3_ID3_V1_YEAR_LENGTH            4
#define SMTK_AUDIO_MP3_ID3_V1_COMMENT_LENGTH         30
#define SMTK_AUDIO_MP3_ID3_V1_GENRE_LENGTH           1
// id3 v2
#define SMTK_AUDIO_MP3_ID3_V2_3_HEADER_SIZE          10
#define SMTK_AUDIO_MP3_ID3_V2_3_HEADER_LENGTH        3
#define SMTK_AUDIO_MP3_ID3_V2_3_VERSION_LENGTH       2
#define SMTK_AUDIO_MP3_ID3_V2_3_SIZE_LENGTH          4

#define SMTK_AUDIO_MP3_ID3_V2_3_UNSYNC_BIT           0x80
#define SMTK_AUDIO_MP3_ID3_V2_3_EXTEND_HEADER_BIT    0x40
#define SMTK_AUDIO_MP3_ID3_V2_3_EXPERTIMENTAL_BIT    0x20

#define SMTK_AUDIO_MP3_ID3_V2_3_FRAME_SIZE           10
#define SMTK_AUDIO_MP3_ID3_V2_3_FRAME_ID_LENGTH      4
#define SMTK_AUDIO_MP3_ID3_V2_3_FRAME_SIZE_LENGTH    4
#define SMTK_AUDIO_MP3_ID3_V2_3_FRAME_FLAG_LENGTH    2

#define SMTK_AUDIO_MP3_ID3_V2_3_FRAME_TAG_ALTER_BIT  0x80
#define SMTK_AUDIO_MP3_ID3_V2_3_FRAME_FILE_ALTER_BIT 0x40
#define SMTK_AUDIO_MP3_ID3_V2_3_FRAME_READONLY_BIT   0x20
#define SMTK_AUDIO_MP3_ID3_V2_3_FRAME_COMPRESS_BIT   0x80
#define SMTK_AUDIO_MP3_ID3_V2_3_FRAME_ENCRYPTION_BIT 0x40
#define SMTK_AUDIO_MP3_ID3_V2_3_FRAME_GROUP_BIT      0x20

#define SMTK_AUDIO_FILE_INFO_DEFAULT_SIZE            256
#define SMTK_AUDIO_LIMIT_ATTATCHED_PICTURE_SIZE      500 * 1024

#define WMA_CODEC_ID_STANDARD1                       0x160
#define WMA_CODEC_ID_STANDARD2                       0x161

#define SMTK_AUDIO_MP3_PASS_MIME_TYPE 14
#define SMTK_AUDIO_M4A_PASS_MIME_TYPE 16
#define SMTK_AUDIO_WMA_PASS_MIME_TYPE 29

#define SMTK_AUDIO_LRC_MAX_SIZE       256

#define SMTK_AUDIO_WAV_HEADER 44
//=============================================================================
//                              Structure Definition
//=============================================================================
typedef enum SMTK_AUDIO_TYPE_TAG
{
    SMTK_AUDIO_TYPE_MP3 = 0,
    SMTK_AUDIO_TYPE_AMR,
    SMTK_AUDIO_TYPE_AAC,
    SMTK_AUDIO_TYPE_WAV,
    SMTK_AUDIO_TYPE_WMA
} SMTK_AUDIO_TYPE;

typedef enum SMTK_AUDIO_STATE_TAG
{
    SMTK_AUDIO_NONE = 0,
    SMTK_AUDIO_PLAY,
    SMTK_AUDIO_PAUSE,
    SMTK_AUDIO_STOP
} SMTK_AUDIO_STATE;

typedef enum SMTK_AUDIO_MODE_TAG
{
    SMTK_AUDIO_NORMAL = 0,
    SMTK_AUDIO_REPEAT
} SMTK_AUDIO_MODE;

typedef enum SMTK_AUDIO_SEEK_MODE_TAG
{
    SMTK_AUDIO_SEEK_OK = 0,
    SMTK_AUDIO_SEEK_NEXT,
    SMTK_AUDIO_SEEK_FAIL
} SMTK_AUDIO_SEEK_MODE;

typedef enum tagAUDIO_MGR_STATE_CALLBACK_E
{
    AUDIOMGR_STATE_CALLBACK_PLAYING_FINISH = 0,
    AUDIOMGR_STATE_CALLBACK_GET_METADATA,
    AUDIOMGR_STATE_CALLBACK_GET_LRC,
    AUDIOMGR_STATE_CALLBACK_GET_FINISH_NAME,
    AUDIOMGR_STATE_CALLBACK_PLAYING_INTERRUPT_SOUND_FINISH,
    AUDIOMGR_STATE_CALLBACK_PLAYING_SPECIAL_CASE_FINISH,
    AUDIOMGR_STATE_CALLBACK_MAX,
} AUDIO_MGR_STATE_CALLBACK_E;

typedef enum SMTK_AUDIO_ERROR_TAG
{
    SMTK_AUDIO_ERROR_NO_ERROR = 0,
    SMTK_AUDIO_ERROR_NOT_SUPPORT,
    SMTK_AUDIO_ERROR_NOT_ALLOCATE_MEMORY_FAIL,
    SMTK_AUDIO_ERROR_FILE_NOT_OPEN,
    SMTK_AUDIO_ERROR_FILE_LENGTH_FAIL,
    SMTK_AUDIO_ERROR_FILE_READ_FAIL,
    SMTK_AUDIO_ERROR_CHANNEL_FAIL,
    SMTK_AUDIO_ERROR_WMA_SUBOBJECT_FAIL,
    SMTK_AUDIO_ERROR_WMA_CODEC_ID_NOT_SUPPORTED,
    SMTK_AUDIO_ERROR_ID3_V2_FAIL,
    SMTK_AUDIO_ERROR_ID3_V2_FRAME_FAIL,
    SMTK_AUDIO_ERROR_ID3_V2_UNREASONABLE_FRAME_SIZE_FAIL,
    SMTK_AUDIO_ERROR_ENGINE_TYPE_NOT_SUPPORT,
    SMTK_AUDIO_ERROR_ENGINE_UNKNOW_ENGINE_TYPE,
    SMTK_AUDIO_ERROR_INIT_FAIL,
    SMTK_AUDIO_ERROR_INIT_CALLBACK_FAIL,
    SMTK_AUDIO_ERROR_UNKNOW_FAIL
} SMTK_AUDIO_ERROR;

typedef enum SMTK_AUDIO_SPECTRUM_MODE_TAG
{
    SMTK_AUDIO_SPECTRUM_OFF = 0,
    SMTK_AUDIO_SPECTRUM_ON
} SMTK_AUDIO_SPECTRUM_MODE;

typedef enum SMTK_AUDIO_UPSAMPLING_MODE_TAG
{
    SMTK_AUDIO_UPSAMPLING_OFF = 0,
    SMTK_AUDIO_UPSAMPLING_ON
} SMTK_AUDIO_UPSAMPLING_MODE;

typedef enum SMTK_AUDIO_UPSAMPLING_ONLY2X_MODE_TAG
{
    SMTK_AUDIO_UPSAMPLING_ONLY2X_OFF = 0,
    SMTK_AUDIO_UPSAMPLING_ONLY2X_ON
} SMTK_AUDIO_UPSAMPLING_ONLY2X_MODE;

typedef enum SMTK_AUDIO_ID3_UTF16_BYTEORDER_TAG
{
    SMTK_AUDIO_ID3_UTF16_BYTEORDER_LE = 0,
    SMTK_AUDIO_ID3_UTF16_BYTEORDER_BE,
    SMTK_AUDIO_ID3_UTF16_BYTEORDER_ERROR
} SMTK_AUDIO_ID3_UTF16_BYTEORDER;

typedef enum SMTK_AUDIO_EQTYPE_TAG
{
    SMTK_AUDIO_EQ_CLASSICAL = 0,
    SMTK_AUDIO_EQ_CLUB,
    SMTK_AUDIO_EQ_LIVE,
    SMTK_AUDIO_EQ_POP,
    SMTK_AUDIO_EQ_ROCK,
    SMTK_AUDIO_EQ_NONE
} SMTK_AUDIO_EQTYPE;

typedef enum SMTK_AUIDO_RECORD_WAVE_FORMAT_TAG
{
    SMTK_AUIDO_RECORD_WAVE_FORMAT_UNKNOWN    = 0x0000, /* Microsoft Unknown Wave Format */
    SMTK_AUIDO_RECORD_WAVE_FORMAT_PCM        = 0x0001, /* Microsoft PCM Format */
    SMTK_AUIDO_RECORD_WAVE_FORMAT_ADPCM      = 0x0002, /* Microsoft ADPCM Format */
    SMTK_AUIDO_RECORD_WAVE_FORMAT_ALAW       = 0x0006, /* Microsoft ALAW */
    SMTK_AUIDO_RECORD_WAVE_FORMAT_MULAW      = 0x0007, /* Microsoft MULAW */
    SMTK_AUIDO_RECORD_WAVE_FORMAT_DVI_ADPCM  = 0x0011, /* Intel's DVI ADPCM */
    SMTK_AUIDO_RECORD_WAVE_FORMAT_G723_ADPCM = 0x0014, /* G.723 ADPCM */
    SMTK_AUIDO_RECORD_WAVE_FORMAT_G726_ADPCM = 0x0064, /* G.726 ADPCM */
    SMTK_AUIDO_RECORD_WAVE_FORMAT_G722_ADPCM = 0x0065  /* G.722 ADPCM */
} SMTK_AUIDO_RECORD_WAVE_FORMAT;

typedef enum SMTK_AUDIO_RECORD_STATUS_TAG
{
    SMTK_AUDIO_RECORD_STATUS_TERMINATE = 0,
    SMTK_AUDIO_RECORD_STATUS_INITIALIZE,
    SMTK_AUDIO_RECORD_STATUS_FILEOPEN,
    SMTK_AUDIO_RECORD_STATUS_PAUSE,
    SMTK_AUDIO_RECORD_STATUS_RECORD
} SMTK_AUDIO_RECORD_STATUS;

typedef enum SMTK_AUDIO_RECORD_TYPE_TAG {
    SMTK_AUDIO_RECORD_TYPE_WAVE_FORMAT_PCM16 = 0x0000,    /* Microsoft PCM Format */
    SMTK_AUDIO_RECORD_TYPE_WAVE_FORMAT_PCM8  = 0x0001,    /* Microsoft PCM Format */
    SMTK_AUDIO_RECORD_TYPE_WAVE_FORMAT_ADPCM = 0x0002,    /* DVI ADPCM Format */
    SMTK_AUDIO_RECORD_TYPE_WAVE_FORMAT_ALAW  = 0x0003,    /* Microsoft ALAW */
    SMTK_AUDIO_RECORD_TYPE_WAVE_FORMAT_MULAW = 0x0004,    /* Microsoft MULAW */
} SMTK_AUDIO_RECORD_TYPE;

typedef enum SMTK_AUDIO_RECORD_SAMPLERATE_TAG {
    SMTK_AUDIO_RECORD_SAMPLERATE_6000  = 0,
    SMTK_AUDIO_RECORD_SAMPLERATE_8000  = 1,
    SMTK_AUDIO_RECORD_SAMPLERATE_11025 = 2,
    SMTK_AUDIO_RECORD_SAMPLERATE_12000 = 3,
    SMTK_AUDIO_RECORD_SAMPLERATE_16000 = 4,
    SMTK_AUDIO_RECORD_SAMPLERATE_22050 = 5,
    SMTK_AUDIO_RECORD_SAMPLERATE_24000 = 6,
    SMTK_AUDIO_RECORD_SAMPLERATE_32000 = 7,
    SMTK_AUDIO_RECORD_SAMPLERATE_44100 = 8,
    SMTK_AUDIO_RECORD_SAMPLERATE_48000 = 9
} SMTK_AUDIO_RECORD_SAMPLERATE;

typedef enum SMTK_AUDIO_RECORD_ERROR_TAG
{
    SMTK_AUDIO_RECORD_ERROR_NO_ERROR = 0,
    SMTK_AUDIO_RECORD_ERROR_NOT_SUPPORT,
    SMTK_AUDIO_RECORD_ERROR_MEMORY_NOT_CREATE,
    SMTK_AUDIO_RECORD_ERROR_FILE_NOT_OPEN,
    SMTK_AUDIO_RECORD_ERROR_FILE_LENGTH_FAIL,
    SMTK_AUDIO_RECORD_ERROR_SAMPLERATE_FAIL,
    SMTK_AUDIO_RECORD_ERROR_TYPE_FAIL,
    SMTK_AUDIO_RECORD_ERROR_LIMIT_TIME_FAIL,
    SMTK_AUDIO_RECORD_ERROR_LIMIT_SIZE_FAIL,
    SMTK_AUDIO_RECORD_ERROR_STATUS_FAIL,
    SMTK_AUDIO_RECORD_ERROR_CHANNEL_FAIL,
    SMTK_AUDIO_RECORD_ERROR_UNKNOW_FAIL
} SMTK_AUDIO_RECORD_ERROR;

typedef enum SMTK_AUDIO_RECORD_LIMIT_TAG
{
    SMTK_AUDIO_RECORD_LIMIT_NOT_CARE = 0,
    SMTK_AUDIO_RECORD_LIMIT_ONLY_TIME,
    SMTK_AUDIO_RECORD_LIMIT_ONLY_SIZE,
    SMTK_AUDIO_RECORD_LIMIT_BOTH_TIME_SIZE
} SMTK_AUDIO_RECORD_LIMIT;

typedef enum SMTK_AUDIO_NETWORK_STATE_TAG
{
    SMTK_AUDIO_NETWORK_STATE_NOT_EOF_ERROR = 0,
    SMTK_AUDIO_NETWORK_STATE_EOF,  // End of file
    SMTK_AUDIO_NETWORK_STATE_EOP,  // End of playing
    SMTK_AUDIO_NETWORK_STATE_ERROR
} SMTK_AUDIO_NETWORK_STATE;

// 1: open, 2:get data 3:put data
typedef enum SMTK_AUDIO_SXADMX_STATE_TAG
{
    SMTK_AUDIO_SXADMX_STATE_INIT = 0,
    SMTK_AUDIO_SXADMX_STATE_OPEN,
    SMTK_AUDIO_SXADMX_STATE_GET_DATA,
    SMTK_AUDIO_SXADMX_STATE_PUT_DATA,
    SMTK_AUDIO_SXADMX_STATE_STOP,
    SMTK_AUDIO_SXADMX_STATE_SEEK
} SMTK_AUDIO_SXADMX_STATE;

typedef struct SMTK_WMAInfo_TAG
{
    MMP_UINT8  header_size;
    MMP_UINT8  channels;
    MMP_INT    sampling_rate;
    MMP_ULONG  frames;
    MMP_INT    bitrate;
    MMP_ULONG  framesize;
    MMP_INT    samplesperframe;
    MMP_INT32  subObjects;
    MMP_UINT32 duration;
    MMP_UINT32 nPacket;
    MMP_INT16  nCodecId;
} SMTK_WMAInfo;

typedef struct SMTK_AUDIO_PARAM_NETWORK_TAG
{
    void *pHandle;                     // handle of network
//#ifdef READ_HTTP
    MMP_INT (*NetworkRead)(void *Handle, char *buf, size_t *size, int timeout);
    size_t (*LocalRead)(void *ptr, size_t size, size_t count, FILE *fp);
    MMP_INT      nReadLength;          // the read length from network
    MMP_INT      nType;                // audio type
    MMP_BOOL     bEOF;
    MMP_BOOL     bEOP;                 // end of play
    MMP_INT      nEOFWaitCount;
    MMP_INT      nError;
    MMP_BOOL     bSeek;          // seek music 1, play from begining :0
    MMP_BOOL     bLocalPlay;
    MMP_BOOL     bSpectrum;          // show spectrum 1
    int          nARMDecode;     // not RISC decode
    int          nM4A;
    int          nSpecialCase;    // for 907X can not reset I2S when DA & AD enable , speical case for small size wav
    int          nLocalFileSize;
    char         *pFilename;      // file name to find lrc
    char         *pSpectrum;
    int          pcmidx;
    int (*audioMgrCallback)(int nCondition);
} SMTK_AUDIO_PARAM_NETWORK;

#ifdef SMTK_AUDIO_SUPPORT_PLAY_EXFILE
//
// Parameter for SmtkAudioMgrPlayEx
//
typedef struct SMTK_AUDIO_PARAM_EX_TAG
{
    PAL_TCHAR *ptPathName;                  // filename with full path
    MMP_INT   nOffset;                      // the offset that audio starts in the media package file
    MMP_INT   nLength;                      // the length of the audio
    MMP_INT   nType;                        // audio type
    MMP_BOOL  bDecrypt;                     // need to decrypt or not
} SMTK_AUDIO_PARAM_EX;
#endif

typedef struct SMTK_AUDIO_TAG_INFO_TAG
{
    MMP_UINT8 pBuffer[4];    // id3 v1
    MMP_UINT8 pTitle[30];
    MMP_UINT8 pWmaBuf[512];
} SMTK_AUDIO_TAG_INFO;

typedef struct SMTK_AUDIO_MP3_ID3_V2_3_HEADER_TAG
{
    char identifier[SMTK_AUDIO_MP3_ID3_V2_3_HEADER_LENGTH];
    char version[SMTK_AUDIO_MP3_ID3_V2_3_VERSION_LENGTH];
    char flag;
    char size[SMTK_AUDIO_MP3_ID3_V2_3_SIZE_LENGTH];
} SMTK_AUDIO_MP3_ID3_V2_3_HEADER;

typedef struct SMTK_AUDIO_MP3_ID3_V2_3_FRAME_TAG
{
    char id[SMTK_AUDIO_MP3_ID3_V2_3_FRAME_ID_LENGTH];
    char size[SMTK_AUDIO_MP3_ID3_V2_3_FRAME_SIZE_LENGTH];
    char flag[SMTK_AUDIO_MP3_ID3_V2_3_FRAME_FLAG_LENGTH];
} SMTK_AUDIO_MP3_ID3_V2_3_FRAME;

//
// Audio file info
//
typedef struct SMTK_AUDIO_FILE_INFO_TAG
{
    MMP_WCHAR     *ptPathName;           // filename with full path
    MMP_WCHAR     *ptTitle;
    MMP_WCHAR     *ptArtist;
    MMP_WCHAR     *ptAlbum;
    MMP_WCHAR     *ptGenre;
    MMP_WCHAR     *ptTools;
    MMP_WCHAR     *ptYear;
    MMP_WCHAR     *ptComment;
    unsigned char *ptTitleUtf8;
    unsigned char *ptArtistUtf8;
    unsigned char *ptAlbumUtf8;
    MMP_UINT8     *ptAttatchedPicture;
    MMP_UINT      nTotlaTime;
    MMP_BOOL      bTitle;
    MMP_BOOL      bArtist;
    MMP_BOOL      bAlbum;
    MMP_BOOL      bGenre;
    MMP_BOOL      bTools;
    MMP_BOOL      bYear;
    MMP_BOOL      bComment;
    MMP_BOOL      bAttatchedPicture;
    MMP_UINT32    nAttatchedPictureSize;
    MMP_BOOL      bSupported;
    MMP_BOOL      bSkipId3V2;                   // if it has id3 v1 ,then skip id3 v2
    MMP_INT       nFlag;
    MMP_BOOL      bUnLimitAttatchedPictureSize; // default limit picture size
} SMTK_AUDIO_FILE_INFO;

typedef struct SMTK_AUDIO_LRC_INFO_TAG {
    int  time; //duration
    int  line; //�Ҧb��
    char pLrc[SMTK_AUDIO_LRC_MAX_SIZE];
} SMTK_AUDIO_LRC_INFO;

typedef struct SMTK_AUDIO_MGR_TAG
{
    MMP_CHAR                 *filename;
    MMP_BOOL                 playing;
    MMP_BOOL                 pause;
    MMP_BOOL                 filePlay;
    MMP_BOOL                 bFilePlayEx;
    MMP_BOOL                 bNetworkPlay;
    MMP_BOOL                 bLocalPlay;
    MMP_BOOL                 destroying;
    MMP_UINT                 volume;
    MMP_INT                  nInit;
    MMP_BOOL                 mute;
    MMP_UINT8                *sampleBuf;
    MMP_UINT8                *streamBuf; // for buffer simple play mode
    MMP_UINT                 streamSize; // for buffer simple play mode
    MMP_BOOL                 stop;
    MMP_BOOL                 parsing;
    MMP_INT                  jumpSecond; // unit : second
    MMP_INT                  nAudioType;
    MMP_INT                  nOffset;
    MMP_INT                  nExFileLength;
    MMP_BOOL                 bAudioEos;
    MMP_INT                  nShowSpectrum;
    MMP_INT                  nKeepTime; // keep eos time
    MMP_INT                  nAudioSeek;
    SMTK_AUDIO_MODE          mode;
    MMP_BOOL                 bDecrypt;  // need to decrypt or not
    MMP_BOOL                 bMp4Parsing;
    MMP_INT                  nMp4Index; // index start from 1
    MMP_INT                  nMp4Format;
    SMTK_AUDIO_FILE_INFO     tFileInfo;
    MMP_UINT                 nDriverSampleRate;
    MMP_INT                  nReadBufferStop;
    MMP_INT                  nCurrentReadBuffer;
    MMP_INT                  nCurrentWriteBuffer;
    SMTK_AUDIO_PARAM_NETWORK ptNetwork;
    unsigned int             nNetwrokTagSize;
    MMP_BOOL                 bCheckMusicComplete;
    int                      nDuration;
    MMP_INT                  nReading;
    int                      nEnableSxaDmx;
    int                      Nfilequeque;
    MMP_BOOL                 bInsertSound;
} SMTK_AUDIO_MGR;

typedef struct SMTK_AUDIO_RECORD_WAVE_WAVEINFO_TAG {
    SMTK_AUIDO_RECORD_WAVE_FORMAT format;
    MMP_UINT32                    nChans;
    MMP_UINT32                    sampRate;
    MMP_UINT32                    avgBytesPerSec;
    MMP_UINT32                    blockAlign;
    MMP_UINT32                    bitsPerSample;
    MMP_UINT32                    samplesPerBlock;
} SMTK_AUDIO_RECORD_WAVE_WAVEINFO;

//
// Parameter for SmtkAudioRecordMgrInFile
//
typedef struct SMTK_AUDIO_RECORD_PARAM_TAG
{
    char             *ptPathName; // filename with full path
    int              nType;       // audio record type
    int              nSampleRate;
    int              nChannel;    // record channels
    unsigned int     nFlag;       // flag
    int              nLimitTime;  // unit : ms
    int              nLineIn;     //0 MIC ,1 Line In
    signed long long nLimitSize;
} SMTK_AUDIO_RECORD_PARAM;

typedef struct SMTK_AUDIO_RECORD_INFO_TAG
{
    int nError;
    int nEncodeTime;
    int nEncodeStatus;
} SMTK_AUDIO_RECORD_INFO;
//=============================================================================
//                              Global Data Definition
//=============================================================================

//=============================================================================
//                              Public Function Definition
//=============================================================================

void AudioThreadFunc();

/**
 * Audio initial function, should be called in AP initialize
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 *
 * @see smtkAudioMgrTerminate().
 */
MMP_INT
smtkAudioMgrInitialize(
    void);

/**
 * called in AP terminated
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 *
 * @see smtkAudioMgrInitialize().
 */
MMP_INT
smtkAudioMgrTerminate(
    void);

/**
 * play the audio file with the given name
 *
 * @param filename  send mp3 file name to audio player
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 */
// MMP_INT
// smtkAudioMgrPlay(
    // MMP_CHAR *filename);

// #ifdef SMTK_AUDIO_SUPPORT_PLAY_EXFILE
// /*
 // * play the ex-audio file with the given name
 // *
 // */
// MMP_INT
// smtkAudioMgrPlayEx(
    // SMTK_AUDIO_PARAM_EX *pParam);
// #endif

// /**
 // * Simplily play the audio file with the given stream buffer and size.
 // * The size should not exceed 64KB
 // *
 // * @param stream  point to playing stream
 // *
 // * @param size  stream size
 // *
 // * @return zero if succeed, otherwise return non-zero error codes.
 // */
// MMP_INT
// smtkAudioMgrSimplePlay(
    // MMP_UINT8 *stream,
    // MMP_LONG  size);

// check if playing,then stop
MMP_INT
smtkAudioMgrQuickStop();

/**
 * Stop audio player
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 *
 */
MMP_INT
smtkAudioMgrStop(
    void);

/**
 * Pause audio player
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 * @see smtkAudioMgrContinue().
 */
MMP_INT
smtkAudioMgrPause(
    void);

/**
 * When audio player paused,this api can continue audio player.
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 * @see smtkAudioMgrPause().
 */
MMP_INT
smtkAudioMgrContinue(
    void);

/**
 * Set volume of audio plyaer.
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 */
MMP_INT
smtkAudioMgrSetVolume(
    MMP_INT nVolume);

/**
 * Get volume of audio plyaer.
 *
 * @return volume
 */
MMP_INT
smtkAudioMgrGetVolume(
    void);

/**
 * Increase volume of audio plyaer.
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 * @see smtkAudioMgrDecreaseVolume().
 */
MMP_INT
smtkAudioMgrIncreaseVolume(
    void);

/**
 * Decrease volume of audio plyaer.
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 * @see smtkAudioMgrIncreaseVolume().
 */
MMP_INT
smtkAudioMgrDecreaseVolume(
    void);

/**
 * Mute on
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 * @see smtkAudioMgrMuteOff().
 */
MMP_INT
smtkAudioMgrMuteOn(
    void);

/**
 * Mute off
 *
 * @return zero if succeed, otherwise return non-zero error codes.
 * @see smtkAudioMgrMuteOn().
 */
MMP_INT
smtkAudioMgrMuteOff(
    void);

/**
 * Check mute param is on or not
 *
 * @return mute param
 * @see smtkAudioMgrIsMuteOn().
 */
MMP_BOOL
smtkAudioMgrIsMuteOn(
    void);

// return upnp (1) /local player (2)
//int
//smtkAudioMgrGetPlayer(
//    void);

int
smtkAudioMgrGetTransitionState(
    void);

/**
 * Get state from audio player.
 * The state is play or pause or stop
 *
 * @return SMTK_AUDIO_STATE
 * @see smtkAudioMgrSetState().
 */
SMTK_AUDIO_STATE
smtkAudioMgrGetState(
    void);

/**
 * get the spectrum information with UINT8[20*5], get 5 frame's spectrum
 *
 * @param buffer
 * @return zero if succeed, otherwise return non-zero error codes.
 */
MMP_UINT8*
smtkAudioMgrGetSpectrum(
    );

/**
 * set the spectrum pointer 
 *
 * @param buffer
 * @return zero if succeed, otherwise return non-zero error codes.
 */

// set spectrum pointer
MMP_INT
smtkAudioMgrSetSpectrum(
    MMP_UINT8* buffer);


// 0:re-start spectrum, 1:pause/stop spectrum
MMP_INT
smtkAudioMgrPauseSpectrum(   
    MMP_INT nPause);


/**
 * Get audio decoding time.
 *
 * @return decoding time
 * @see smtkAudioMgrGetTime().
 */
MMP_UINT
smtkAudioMgrGetTime(
    void);

/**
 * Set audio decoding time.
 *
 * @return decoding time
 * @see smtkAudioMgrGetTime().
 */
MMP_INT
smtkAudioMgrSetTime(
    MMP_INT nTime);

/**
 * Get audio Network Music Tag Size
 *
 * @return decoding time
 * @see smtkAudioMgrGetTime().
 */
/*unsigned int
smtkAudioMgrGetNetworkMusicTagSize(
    void);

/**
 * Set audio Network Music Tag Size
 *
 * @return decoding time
 * @see smtkAudioMgrGetTime().
 */
/*void
smtkAudioMgrSetNetworkMusicTagSize(
    unsigned int nSize);

/**
 * Reset network read handle
 *
 * @return
 */
int smtkAudiomgrPlayNetworkResetHandle(void *handle);

/**
 * Set state from audio player.
 * The state is play or pause or stop
 *
 * @return SMTK_AUDIO_STATE
 * @see smtkAudioMgrGetState().
 */
void
smtkAudioMgrSetMode(
    SMTK_AUDIO_MODE mode);

MMP_UINT
smtkAudioMgrSetTotalTime(
    int nTime);

MMP_UINT
smtkAudioMgrGetTotalTime(
    MMP_UINT *time);

/**
 * Set parsing status
 *
 */
void
smtkAudioMgrSetParsing(
    MMP_BOOL status);

// show spectrum
MMP_INT
smtkAudioMgrShowSpectrum(
    MMP_INT nMode);

void smtkAudioMgrGetNetworkState(
    int *pType, int *pErr);

void smtkAudioMgrSetNetworkError(int nErr);

// Get file Info
MMP_INT
smtkAudioMgrGetFileInfo(
    SMTK_AUDIO_FILE_INFO *ptFileInfo);

// Close file Info
MMP_INT
smtkAudioMgrGetFileInfoClose(
    SMTK_AUDIO_FILE_INFO *ptFileInfo);

int
smtkAudioMgrGetMetadata(SMTK_AUDIO_FILE_INFO *tgt_metadata);

SMTK_AUDIO_LRC_INFO *
smtkAudioMgrGetLrc(char *filename);

/* copy lrc's pointer */
/*SMTK_AUDIO_LRC_INFO *
smtkAudioMgrCopyLrc();
*/
/*for quick play sound*/
MMP_INT smtkSetFileQueue(SMTK_AUDIO_PARAM_NETWORK tmpNetwork);

void smtkAudioMgrSetInitPlay(SMTK_AUDIO_PARAM_NETWORK *audiomgr_local,char *filename);
MMP_INT smtkAudioMgrPlayNetwork(SMTK_AUDIO_PARAM_NETWORK *pNetwork);
/*for inturrupt sound (video inturrupt audio)*/
void FFmpeg_pause(int pause);
MMP_BOOL smtkAudioGetInsertSoundStatus(void);
void smtkAudioSetInsertSoundStatus(MMP_BOOL pause);

int (*smtkAudioMgrGetCallbackFunction())(int);
void smtkAudioMgrSetCallbackFunction(int (*pApi)(int));

/*record*/
MMP_INT smtkAudioMgrRecordSetLevel(MMP_INT nReclevel);
void smtkAudioMgrSetInitRecord(SMTK_AUDIO_RECORD_PARAM *audiomgr_record,char *filename);

/*for inturrupt sound (audio inturrupt audio(.wav))*/
int smtkAudioMgrSetInterruptStatus(int nStatus);// return 1: interrupt sound playing ,return 0: interrupt sound not playing
int smtkAudioMgrGetInterruptStatus();           // return 1: interrupt sound playing ,return 0: interrupt sound not playing
int smtkAudioMgrInterruptSoundGetRemoveCard();  // nRemove :1 remove ,nRemove:0 finished
void smtkAudioMgrInterruptSoundSetRemoveCard(int nRemove);// nRemove :1 remove ,nRemove:0 finished
void smtkAudioMgrSetInterruptOverwriteStatus(int nStatus);

// 1: not overrite,only play wav
int smtkAudioMgrGetInterruptOverwriteStatus();

// input a wav file to play immediately
int smtkAudioMgrInterruptSound(char *filename, int overwrite, int *pCallback);

//set equalizer
void
smtkAudioSetEqualizer(
    SMTK_AUDIO_EQTYPE eqType);

// open audio engine
MMP_INT
smtkAudioMgrOpenEngine(
    MMP_INT nEngineType);

int
smtkAudioMgrGetI2sPostpone();

int
smtkAudioMgrSampleRate();

int
smtkAudioMgrSetI2sPostpone(int nValue);

int smtkAudioMgrGetType();

char *smtkAudioMgrGetFinishName();

// set callbakc function
void smtkAudioMgrInterruptSoundFinish();

// return 0:not wav hd, 1: wav hd
int smtkAudioMgrWavHD(unsigned int *pBitPerSample);

int smtkAudioMgrUnloadSBC();

// set func pointer to reload sbc
void smtkAudioMgrReloadSBC(void* func);

int
smtkAudioMgrRecordInitialize(
    void);

int
smtkAudioMgrRecordTerminate(
    void);

int
smtkAudioMgrRecordSetting(
    SMTK_AUDIO_RECORD_PARAM *pParam);

int
smtkAudioMgrRecordStart(
    SMTK_AUDIO_RECORD_PARAM *pParam);

int
smtkAudioMgrRecordPause(
    void);

int
smtkAudioMgrRecordResume(
    void);

int
smtkAudioMgrRecordStop(
    void);

int
smtkAudioMgrRecordGetStatus(
    SMTK_AUDIO_RECORD_INFO *tEncodeInfo);

bool mtal_pb_check_fileplayer_with_audio(void);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_H */