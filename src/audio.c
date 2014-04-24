#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <stdint.h>

#include <portaudio.h>

#include "params.h"

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int paCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    /* Cast data passed through stream to our structure. */
    gydra_param_t *params = (gydra_param_t *)userData;

    (void) framesPerBuffer; /* Prevent unused variable warning. */
    (void) inputBuffer;     /* Prevent unused variable warning. */
    (void) timeInfo;        /* Prevent unused variable warning. */
    (void) statusFlags;     /* Prevent unused variable warning. */

    /* Delete data that the callback is finished with */
    if (PaUtil_GetRingBufferReadAvailable(&params->ringBuffer) > 0) {
        PaUtil_ReadRingBuffer(&params->ringBuffer, outputBuffer, 1);
    }
    return 0;
}

/*******************************************************************/

void handle_audio_error(int err)
{
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    // Print more information about the error.
    if( err == paUnanticipatedHostError ) {
        const PaHostErrorInfo *hostErrorInfo = Pa_GetLastHostErrorInfo();
        fprintf( stderr, "Host API error = #%ld, hostApiType = %d\n", hostErrorInfo->errorCode, hostErrorInfo->hostApiType );
        fprintf( stderr, "Host API error = %s\n", hostErrorInfo->errorText );
    }
}

/*******************************************************************/

int init_audio(gydra_param_t *gdata)
{
    PaStreamParameters outputParameters;

    //PaStream *stream;
    PaError err = paNoError;

    printf("Initalizing audio engine...\n");

    /* Initialize library before making any other calls. */
    err = Pa_Initialize();
    if( err != paNoError ) {
        handle_audio_error(err);
        return err;
    }

    /* Initialize communication buffers (queues) */
    gdata->ringBufferData = malloc(BUFFER_SIZE * BUFFERS_COUNT);
    if (gdata->ringBufferData == NULL) {
        printf("ERROR: Cannot allocate mamory for audio buffers!\n");
        return -1;
    }
    PaUtil_InitializeRingBuffer(&gdata->ringBuffer, BUFFER_SIZE, BUFFERS_COUNT, gdata->ringBufferData);

    memset( &outputParameters, 0, sizeof( outputParameters ) ); //not necessary if you are filling in all the fields
    outputParameters.channelCount = NUM_CHANNELS;
    outputParameters.sampleFormat = SAMPLE_FORMAT;

    /* Set audio stream parameters */
    if(gdata->id == -1) {
        outputParameters.device = Pa_GetDefaultOutputDevice();
        printf("Use system default audio output device ID = %d\n", outputParameters.device);
    }
    else {
        printf("Use defice id %d for audio output...\n", gdata->id);
        outputParameters.device = gdata->id;
    }
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowInputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    /* Open stream for writing */
    err = Pa_OpenStream( &gdata->stream,
                         0,                                 /* no input channels */
                         &outputParameters,
                         SAMPLE_RATE,
                         FRAMES_PER_BUFFER,
                         paClipOff,
                         paCallback,
                         (void*) gdata);
    if( err != paNoError ) {
        handle_audio_error(err);
        return err;
    }

    err = Pa_StartStream( gdata->stream );
    if( err != paNoError ) {
        handle_audio_error(err);
        return err;
    }

    return 0;
}

/*******************************************************************/

int close_audio(gydra_param_t *gdata)
{
    //PaStream *stream;
    PaError err = paNoError;

    /* Gracefull shutdown */
    printf("Close audio engine.\n");
    err = Pa_StopStream( gdata->stream );
    if( err != paNoError ) {
        handle_audio_error(err);
        return err;
    }

    err = Pa_CloseStream( gdata->stream );
    if( err != paNoError ) {
        handle_audio_error(err);
        return err;
    }

    Pa_Terminate();

    /* Cleanup memory */
    PaUtil_FlushRingBuffer(&gdata->ringBuffer);
    free(gdata->ringBufferData);

    return 0;
}

