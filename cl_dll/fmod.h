/* ========================================================================================== */
/* FMOD 3.6 main header (Firelight Technologies), reduced to the declarations CMP3 uses.      */
/* The client loads fmod.dll at run time (mp3.cpp) and links no FMOD import library.          */
/* ========================================================================================== */

#ifndef _FMOD_H_
#define _FMOD_H_

/* 
    FMOD defined types 
*/
typedef struct FSOUND_STREAM    FSOUND_STREAM;

/*
[ENUM]
[
    [DESCRIPTION]   
    These output types are used with FSOUND_SetOutput, to choose which output driver to use.
]
*/
enum FSOUND_OUTPUTTYPES
{
    FSOUND_OUTPUT_NOSOUND,    /* NoSound driver, all calls to this succeed but do nothing. */
    FSOUND_OUTPUT_WINMM,      /* Windows Multimedia driver. */
    FSOUND_OUTPUT_DSOUND      /* DirectSound driver. */
};

/*
[DEFINE_START] 
[
    [NAME] 
    FSOUND_MODES
    
    [DESCRIPTION]   
    Sample description bitfields, OR them together for loading and describing samples.
]
*/
#define FSOUND_LOOP_NORMAL   0x00000002  /* For forward looping samples. */
#define FSOUND_16BITS        0x00000010  /* For 16 bit samples. */
#define FSOUND_MONO          0x00000020  /* For mono samples. */
#define FSOUND_SIGNED        0x00000100  /* For user created source data containing signed data. */

#define FSOUND_NORMAL       (FSOUND_16BITS | FSOUND_SIGNED | FSOUND_MONO)      
/* [DEFINE_END] */

#endif
