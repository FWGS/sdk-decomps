// mp3 support added by Killar
//
// T2DEF: Killar's FMOD MP3 player (the tutorial's names; its strings and the
// eight decorated FSOUND names are retail's), playing music/<file> once or looped

#ifndef MP3_H
#define MP3_H

#include "fmod.h"
#include "windows.h"

class CMP3
{
private:
	signed char		(_stdcall * SCL)	(FSOUND_STREAM *stream);
	signed char		(_stdcall * SOP)	(int outputtype);
	signed char		(_stdcall * SBS)	(int len_ms);
	signed char		(_stdcall * SDRV)	(int driver);
	signed char		(_stdcall * INIT)	(int mixrate, int maxsoftwarechannels, unsigned int flags);
	FSOUND_STREAM*	(_stdcall * SOF)	(const char *filename, unsigned int mode, int memlength);
	int				(_stdcall * SPLAY)	(int channel, FSOUND_STREAM *stream);
	void			(_stdcall * CLOSE)	( void );

	FSOUND_STREAM	*m_Stream;
	int				m_iIsPlaying;
	HINSTANCE		m_hFMod;

public:
	int		Initialize();
	int		Shutdown();
	int		PlayMP3( char cLoop, const char *pszSong );	// cLoop: name ours
	int		StopMP3();
};

extern CMP3 gMP3;
#endif
