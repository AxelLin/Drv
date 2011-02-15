#ifndef __TW2865_LINUX__
#define __TW2865_LINUX__

#define AUTOMATICALLY 0
#define TW2865_NTSC	1
#define TW2865_PAL	2

#define SET_8_BITWIDTH 0
#define SET_16_BITWIDTH 1


typedef struct hitw2865_set_analogsetting{/*analog set*/
	unsigned char ch;
	unsigned char hue;
	unsigned char contrast;
	unsigned char brightness;
	unsigned char saturation;
	unsigned long SettingBitmap;
}tw2865_set_analogsetting;


typedef struct _audiooutput_{
	unsigned char PlaybackOrLoop;/*0:playback; 1:loop*/
	unsigned char channel;/*[0, 15]*/
}tw2865_set_audiooutput;

typedef struct _videooutput_{
	unsigned char PlaybackOrLoop;/*0:playback; 1:loop*/
	unsigned char channel;/*[0, 15]*/
}tw2865_set_videooutput;

typedef enum _audiosamplerate_{
	ASR_8K,
	ASR_16K,
	ASR_32K,
	ASR_44DOT1K,
	ASR_48K
}tw2865_set_audiosamplarate;


typedef struct hitw2865_w_reg
{
    unsigned char addr;
    unsigned char value;
}tw2865_w_reg;

typedef struct hitw2865_set_2d1
{
	unsigned char vdn;
    unsigned char ch1;
    unsigned char ch2;
}tw2865_set_2d1;

typedef struct hitw2865_set_1d1
{
	unsigned char vdn;
    unsigned char ch;
}tw2865_set_1d1;

typedef struct hitw2865_set_videomode
{
   unsigned char ch;
   unsigned char mode;
}tw2865_set_videomode;
typedef struct hitw2865_set_controlvalue
{
   unsigned char ch;
   unsigned char controlvalue;
}tw2865_set_controlvalue;


#define TW2865CMD_READ_REG		0x00
#define TW2865CMD_WRITE_REG	0x01
#define TW2865CMD_GET_VL		0x02
#define TW2865CMD_SET_ADA_SAMPLERATE		0x03//Audio output sample rate
#define TW2865CMD_SET_ANALOG	0x04
#define TW2865CMD_GET_ANALOG	0x05
#define TW2865CMD_READ_REGT	0x06
#define TW2865CMD_AUDIO_OUTPUT	0x07
#define TW2865CMD_AUDIO_MUTE	0x08
#define TW2865CMD_AUDIO_DEMUTE	0x09
#define TW2865CMD_AUDIO_VOLUME	0X0a//…Ë÷√“Ù¡ø
#define TW2865CMD_SET_1_D1		0x0b
#define TW2865CMD_SET_2_D1	0x0c
#define TW2865CMD_SET_4_CIF	0x0d
#define TW2865CMD_SET_4_D1		0x0e
#define TW2865CMD_GET_VIDEO_MODE		0x0f
#define TW2865CMD_SET_AUDIO_OUTPUT 0x10
#define TW2865CMD_SET_ADA_BITWIDTH 0x11
#define TW2865CMD_SET_CHANNEL_SEQUENCE 0x12
#define TW2865CMD_SET_VIDEO_MODE      0x13
#define TW2865CMD_SET_HUE 0x14
#define TW2865CMD_SET_CONTRAST 0x15
#define TW2865CMD_SET_BRIGHTNESS 0x16
#define TW2865CMD_SET_SATURATION 0x17
#define TW2865CMD_GET_HUE 0x18
#define TW2865CMD_GET_CONTRAST 0x19
#define TW2865CMD_GET_BRIGHTNESS 0x1a
#define TW2865CMD_GET_SATURATION 0x1b
#define TW2865CMD_REG_DUMP  0x1c
#define TW2865CMD_VIDEO_IN_ON  0x1d
#define TW2865CMD_VIDEO_IN_OFF 0x1e
#define TW2865CMD_VIDEO_OUT_ON 0x1f
#define TW2865CMD_VIDEO_OUT_OFF 0x20
#define TW2865CMD_AUDIO_IN_ON   0x21
#define TW2865CMD_AUDIO_IN_OFF  0x22
#define TW2865CMD_AUDIO_OUT_ON  0x23
#define TW2865CMD_AUDIO_OUT_OFF 0x24
#define TW2865CMD_VIDEO_OUT     0x25
#define TW2865CMD_SET_AIGAIN    0x26
#endif


