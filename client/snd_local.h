/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

//
// snd_local.h
// Private sound functions
//

#include "../client/cl_local.h"

#define MAX_SFX			(MAX_CS_SOUNDS*2)
#define MAX_CHANNELS	128
#define MAX_PLAYSOUNDS	128

// only begin attenuating sound volumes when outside the FULLVOLUME range
#define SOUND_FULLVOLUME	80.0f
#define SOUND_LOOPATTENUATE	(ATTN_STATIC * 0.001f)

struct sfxCache_t {
	int					alFormat;
	uint32				alBufferNum;

	int					length;
	int					loopStart;
	int					speed;				// not needed, because converted on load?
	int					width;
	int					stereo;

	byte				data[1];			// variable sized
};

struct sfx_t {
	char				name[MAX_QPATH];
	uint32				touchFrame;			// 0 = free
	sfxCache_t			*cache;
	char				*trueName;
};

// playsound types let the mixer(s) know to play locally, or to spatialize for a fixed/entity origin/velocity
typedef enum psndType_s {
	PSND_ENTITY,
	PSND_FIXED,
	PSND_LOCAL,
} psndType_t;

// a playSound_t will be generated by each call to Snd_StartSound, when the mixer
// reaches playsound->begin, the playsound will be assigned to a channel
struct playSound_t {
	playSound_t			*prev, *next;

	sfx_t				*sfx;

	psndType_t			type;

	int					volume;
	float				attenuation;

	int					entNum;
	EEntSndChannel		entChannel;

	vec3_t				origin;

	int					beginTime;			// begin on this sample
};

struct wavInfo_t {
	int					rate;
	int					width;
	int					channels;
	int					loopStart;
	int					samples;
	int					dataOfs;			// chunk starts this many bytes from file start
};

struct channel_t {
	sfx_t				*sfx;				// sfx number

	psndType_t			psType;

	int					entNum;				// to allow overriding a specific sound
	EEntSndChannel		entChannel;

	bool				alLooping;
	int					alLoopEntNum;
	int					alLoopFrame;
	bool				alRawPlaying;		// don't stop playing until all buffers are processed
	bool				alRawStream;		// raw stream channels are locked from being used until done
	uint32				alSourceNum;
	int					alStartTime;
	float				alVolume;

	vec3_t				origin;

	int					masterVol;			// 0-255 master volume
	int					leftVol;			// 0-255 volume
	int					rightVol;			// 0-255 volume

	int					endTime;			// end time in global paintsamples
	int					position;			// sample position in sfx

	float				distMult;			// distance multiplier (attenuation/clipK)

	bool				autoSound;			// from an entity->sound, cleared each frame
};

// ==========================================================================

//
// snd_main.c
//
extern bool					snd_isActive;
extern bool					snd_isDMA;
extern bool					snd_isAL;

extern uint32					snd_registrationFrame;

extern playSound_t				snd_playSounds[MAX_PLAYSOUNDS];
extern playSound_t				snd_freePlays;
extern playSound_t				snd_pendingPlays;

extern cVar_t	*s_show;
extern cVar_t	*s_loadas8bit;
extern cVar_t	*s_volume;

extern cVar_t	*s_khz;
extern cVar_t	*s_mixahead;
extern cVar_t	*s_testsound;
extern cVar_t	*s_primary;

extern cVar_t	*al_allowExtensions;
extern cVar_t	*al_device;
extern cVar_t	*al_dopplerFactor;
extern cVar_t	*al_dopplerVelocity;
extern cVar_t	*al_errorCheck;
extern cVar_t	*al_gain;
extern cVar_t	*al_minDistance;
extern cVar_t	*al_maxDistance;
extern cVar_t	*al_rollOffFactor;

sfxCache_t *Snd_LoadSound (sfx_t *s);

void	Snd_FreePlaysound (playSound_t *ps);

//
// snd_dma.c
//
struct audioDMA_t {
	int				channels;
	int				samples;			// mono samples in buffer
	int				submissionChunk;	// don't mix less than this #
	int				samplePos;			// in mono samples
	int				sampleBits;
	int				speed;
	byte			*buffer;
};

extern audioDMA_t	snd_audioDMA;
extern int			snd_dmaPaintedTime;

bool	DMASnd_Init ();
void	DMASnd_Shutdown ();

void	DMASnd_StopAllSounds ();
void	DMASnd_RawSamples (int samples, int rate, int width, int channels, byte *data);

void	DMASnd_Update (refDef_t *rd);

//
// snd_openal.c
//
struct audioAL_t {
	// Static information (never changes after initialization)
	const char	*extensionString;
	const char	*rendererString;
	const char	*vendorString;
	const char	*versionString;

	const char	*deviceName;

	int			numChannels;

	// Dynamic information
	int			frameCount;
};

extern audioAL_t	snd_audioAL;

void	ALSnd_Activate(const bool bActive);

void	ALSnd_CreateBuffer (sfxCache_t *sc, int width, int channels, byte *data, int size, int frequency);
void	ALSnd_DeleteBuffer (sfxCache_t *sc);

void	ALSnd_StopAllSounds ();

channel_t *ALSnd_RawStart ();
void	ALSnd_RawSamples (channel_t *rawChannel, int samples, int rate, int width, int channels, byte *data);
void	ALSnd_RawStop (channel_t *rawChannel);
void	ALSnd_RawShutdown ();

void	ALSnd_Update (refDef_t *rd);

bool	ALSnd_Init ();
void	ALSnd_Shutdown ();

/*
=============================================================================

	SYSTEM SPECIFIC FUNCTIONS

=============================================================================
*/

bool	SndImp_Init ();			// initializes cycling through a DMA buffer and returns information on it
int		SndImp_GetDMAPos ();	// gets the current DMA position
void	SndImp_BeginPainting ();
void	SndImp_Submit ();
void	SndImp_Shutdown ();