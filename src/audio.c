/*********************************************************************************************
*
*   raylib.audio
*
*   Basic functions to manage Audio: InitAudioDevice, LoadAudioFiles, PlayAudioFiles
*    
*   Uses external lib:    
*       OpenAL - Audio device management lib
*       TODO: stb_vorbis - Ogg audio files loading
*       
*   Copyright (c) 2013 Ramon Santamaria (Ray San - raysan@raysanweb.com)
*    
*   This software is provided "as-is", without any express or implied warranty. In no event 
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial 
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you 
*     wrote the original software. If you use this software in a product, an acknowledgment 
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#include "raylib.h"

#include <AL/al.h>           // OpenAL basic header
#include <AL/alc.h>          // OpenAL context header (like OpenGL, OpenAL requires a context to work)

#include <stdlib.h>          // To use exit() function
#include <stdio.h>           // Used for .WAV loading

//#include "stb_vorbis.h"    // TODO: OGG loading functions

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// Nop...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// Wave file data
typedef struct Wave {
    unsigned char *data;      // Buffer data pointer
    unsigned int sampleRate;
    unsigned int dataSize;
    short channels;
    short format;    
} Wave;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
// Nop...

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
static Wave LoadWAV(char *fileName);
static void UnloadWAV(Wave wave);
//static Ogg LoadOGG(char *fileName);

//----------------------------------------------------------------------------------
// Module Functions Definition - Window and OpenGL Context Functions
//----------------------------------------------------------------------------------

// Initialize audio device and context
void InitAudioDevice()
{
    // Open and initialize a device with default settings
    ALCdevice *device = alcOpenDevice(NULL);
    
    if(!device)
    {
        fprintf(stderr, "Could not open a device!\n");
        exit(1);
    }

    ALCcontext *context = alcCreateContext(device, NULL);
    
    if(context == NULL || alcMakeContextCurrent(context) == ALC_FALSE)
    {
        if(context != NULL)    alcDestroyContext(context);
        
        alcCloseDevice(device);
        
        fprintf(stderr, "Could not set a context!\n");
        exit(1);
    }

    printf("Opened \"%s\"\n", alcGetString(device, ALC_DEVICE_SPECIFIER));
    
    // Listener definition (just for 2D)
    alListener3f(AL_POSITION, 0, 0, 0);
    alListener3f(AL_VELOCITY, 0, 0, 0);
    alListener3f(AL_ORIENTATION, 0, 0, -1);
}

// Close the audio device for the current context, and destroys the context
void CloseAudioDevice()
{
    ALCdevice *device;
    ALCcontext *context = alcGetCurrentContext();
    
    if (context == NULL) return;

    device = alcGetContextsDevice(context);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

// Load sound to memory
Sound LoadSound(char *fileName)
{
    Sound sound;
    
    // NOTE: The entire file is loaded to memory to play it all at once (no-streaming)
    
    // WAV file loading
    // NOTE: Buffer space is allocated inside LoadWAV, Wave must be freed
    Wave wave = LoadWAV(fileName);
    
    ALenum format;
    // The OpenAL format is worked out by looking at the number of channels and the bits per sample
    if (wave.channels == 1) 
    {
        if (wave.sampleRate == 8 ) format = AL_FORMAT_MONO8;
        else if (wave.sampleRate == 16) format = AL_FORMAT_MONO16;
    } 
    else if (wave.channels == 2) 
    {
        if (wave.sampleRate == 8 ) format = AL_FORMAT_STEREO8;
        else if (wave.sampleRate == 16) format = AL_FORMAT_STEREO16;
    }
    
    // Create an audio source
    ALuint source;
    alGenSources(1, &source);            // Generate pointer to audio source

    alSourcef(source, AL_PITCH, 1);    
    alSourcef(source, AL_GAIN, 1);
    alSource3f(source, AL_POSITION, 0, 0, 0);
    alSource3f(source, AL_VELOCITY, 0, 0, 0);
    alSourcei(source, AL_LOOPING, AL_FALSE);
    
    // Convert loaded data to OpenAL buffer
    //----------------------------------------
    ALuint buffer;
    alGenBuffers(1, &buffer);            // Generate pointer to buffer

    // Upload sound data to buffer
    alBufferData(buffer, format, wave.data, wave.dataSize, wave.sampleRate);

    // Attach sound buffer to source
    alSourcei(source, AL_BUFFER, buffer);
    
    // Unallocate WAV data
    UnloadWAV(wave);
    
    printf("Sample rate: %i\n", wave.sampleRate);
    printf("Channels: %i\n", wave.channels);
    printf("Format: %i\n", wave.format);
    
    printf("Audio file loaded...!\n");
    
    sound.source = source;
    sound.buffer = buffer;
    
    return sound;
}

// Unload sound
void UnloadSound(Sound sound)
{
    alDeleteSources(1, &sound.source);
    alDeleteBuffers(1, &sound.buffer);
}

// Play a sound
void PlaySound(Sound sound)
{
    alSourcePlay(sound.source);        // Play the sound
    
    printf("Playing sound!\n");

    // Find the current position of the sound being played
    // NOTE: Only work when the entire file is in a single buffer
    //int byteOffset;
    //alGetSourcei(sound.source, AL_BYTE_OFFSET, &byteOffset);
    //float seconds = (float)byteOffset / sampleRate;      // Number of seconds since the beginning of the sound
}

// Play a sound with extended options
void PlaySoundEx(Sound sound, float timePosition, bool loop)
{
    // TODO: Review
    
    // Change the current position (e.g. skip some part of the sound)
    // NOTE: Only work when the entire file is in a single buffer
    //alSourcei(sound.source, AL_BYTE_OFFSET, int(position * sampleRate));

    alSourcePlay(sound.source);        // Play the sound
    
    if (loop) alSourcei(sound.source, AL_LOOPING, AL_TRUE);
    else alSourcei(sound.source, AL_LOOPING, AL_FALSE);
}

// Pause a sound
void PauseSound(Sound sound)
{
    alSourcePause(sound.source);
}

// Stop reproducing a sound
void StopSound(Sound sound)
{
    alSourceStop(sound.source);
}

// Load WAV file into Wave structure
static Wave LoadWAV(char *fileName) 
{
    Wave wave;
    FILE *wavFile; 

    wavFile = fopen(fileName, "rb");
    
    if (!wavFile)
    {
        printf("Could not load WAV file.\n");
        exit(1);
    }
    
    unsigned char id[4];         // Four bytes to hold 'RIFF' and 'WAVE' (and other ids)
    
    unsigned int size = 0;       // File size (useless)
    
    short format;
    short channels;
    short blockAlign;
    short bitsPerSample;
    
    unsigned int formatLength;
    unsigned int sampleRate;
    unsigned int avgBytesSec;
    unsigned int dataSize;

    fread(id, sizeof(unsigned char), 4, wavFile);           // Read the first four bytes 
    
    if ((id[0] != 'R') || (id[1] != 'I') || (id[2] != 'F') || (id[3] != 'F'))
    {
        printf("Invalid RIFF file.\n");                     // If not "RIFF" id, exit
        exit(1);
    }                 

    fread(&size, sizeof(unsigned int), 1, wavFile);         // Read file size
    fread(id, sizeof(unsigned char), 4, wavFile);           // Read the next id
    
    if ((id[0] != 'W') || (id[1] != 'A') || (id[2] != 'V') || (id[3] != 'E'))
    {
        printf("Invalid WAVE file.\n");                     // If not "WAVE" id, exit
        exit(1);
    } 
    
    fread(id, sizeof(unsigned char), 4, wavFile);           // Read 4 bytes id "fmt "
    fread(&formatLength, sizeof(unsigned int),1,wavFile);   // Read format lenght
    fread(&format, sizeof(short), 1, wavFile);              // Read format tag
    fread(&channels, sizeof(short), 1, wavFile);            // Read num channels (1 mono, 2 stereo) 
    fread(&sampleRate, sizeof(unsigned int), 1, wavFile);   // Read sample rate (44100, 22050, etc...)
    fread(&avgBytesSec, sizeof(short), 1, wavFile);         // Read average bytes per second (probably won't need this)
    fread(&blockAlign, sizeof(short), 1, wavFile);          // Read block alignment (probably won't need this)
    fread(&bitsPerSample, sizeof(short), 1, wavFile);       // Read bits per sample (8 bit or 16 bit) 
    
    fread(id, sizeof(unsigned char), 4, wavFile);           // Read 4 bytes id "data" 
    fread(&dataSize, sizeof(unsigned int), 1, wavFile);     // Read data size (in bytes)
    
    wave.sampleRate = sampleRate;
    wave.dataSize = dataSize;
    wave.channels = channels;
    wave.format = format;
    
    wave.data = (unsigned char *)malloc(sizeof(unsigned char) * dataSize);     // Allocate the required bytes to store data
    
    fread(wave.data, sizeof(unsigned char), dataSize, wavFile);         // Read the whole sound data chunk
    
    return wave;
} 

// Unload WAV file data
static void UnloadWAV(Wave wave)
{
    free(wave.data);
}

// TODO: Ogg data loading
//static Ogg LoadOGG(char *fileName) { }

