#include <stdio.h>
#include <math.h>
#include <memory.h>
#include "portaudio.h"

#ifdef WIN32
#include <windows.h>
#endif

#include "params.h"

/*******************************************************************/
int list_devices()
{
    int     i, numDevices, defaultDisplayed;
    const   PaDeviceInfo *deviceInfo;
    PaStreamParameters outputParameters;
    PaStream*          stream;
    PaError err = paNoError;

    memset( &outputParameters, 0, sizeof( outputParameters ) );
    outputParameters.channelCount = NUM_CHANNELS;
    outputParameters.sampleFormat = SAMPLE_FORMAT;
    
    err = Pa_Initialize();
    if( err != paNoError )
    {
        printf( "ERROR: Pa_Initialize returned 0x%x\n", err );
        goto error;
    }

    printf( "\nPortAudio: %d\nPortAudio: '%s'\n",
            Pa_GetVersion(), Pa_GetVersionText() );


    numDevices = Pa_GetDeviceCount();
    if( numDevices < 0 )
    {
        printf( "ERROR: Pa_GetDeviceCount returned 0x%x\n", numDevices );
        err = numDevices;
        goto error;
    }
    
    printf( "\nTotal number of devices = %d\n", numDevices );
    printf( "\nList of compatible devices:\n" );
    for( i=0; i<numDevices; i++ )
    {
        deviceInfo = Pa_GetDeviceInfo( i );
        /* Print only compatible output devices */
        if(deviceInfo->maxOutputChannels <= 0)
            continue;
        outputParameters.device = i;
        if(Pa_IsFormatSupported( NULL, &outputParameters, SAMPLE_RATE ) != 0)
            continue;
        err = Pa_OpenStream( &stream,
                             0,
                             &outputParameters,
                             SAMPLE_RATE,
                             paFramesPerBufferUnspecified,
                             paNoFlag,
                             NULL,
                             (void *)NULL );
        Pa_CloseStream(&stream);
        if(err != paNoError)
            continue;

        printf( "\nDevice\t%d ------------------------------------\n", i );

        /* Mark global and API specific default devices */
        defaultDisplayed = 0;
        if( i == Pa_GetDefaultOutputDevice() )
        {
            printf( (defaultDisplayed ? "," : "[") );
            printf( " Default Output" );
            defaultDisplayed = 1;
        }
        else if( i == Pa_GetHostApiInfo( deviceInfo->hostApi )->defaultOutputDevice )
        {
            const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo( deviceInfo->hostApi );
            printf( (defaultDisplayed ? "," : "[") );
            printf( " Default %s Output", hostInfo->name );
            defaultDisplayed = 1;
        }

        if( defaultDisplayed )
            printf( " ]\n" );

        /* print device info fields */
#ifdef WIN32
        {   /* Use wide char on windows, so we can show UTF-8 encoded device names */
            wchar_t wideName[MAX_PATH];
            MultiByteToWideChar(CP_UTF8, 0, deviceInfo->name, -1, wideName, MAX_PATH-1);
            wprintf( L"Name       = %s\n", wideName );
        }
#else
        printf( "Name       = %s\n", deviceInfo->name );
#endif
        printf( "Host API   = %s\n",  Pa_GetHostApiInfo( deviceInfo->hostApi )->name );
    }

    Pa_Terminate();

    printf("\n");
    return 0;

error:
    Pa_Terminate();
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}

int check_device(int id)
{
    int     numDevices;
    const   PaDeviceInfo *deviceInfo;
    PaStreamParameters outputParameters;
    PaStream* stream;
    PaError err = paNoError;

    memset( &outputParameters, 0, sizeof( outputParameters ) );
    outputParameters.channelCount = NUM_CHANNELS;
    outputParameters.sampleFormat = SAMPLE_FORMAT;

    err = Pa_Initialize();
    if( err != paNoError ) {
        printf( "ERROR: Pa_Initialize returned 0x%x\n", err );
        return 0;
    }

    numDevices = Pa_GetDeviceCount();
    if( numDevices < 0 ) {
        printf( "ERROR: Pa_GetDeviceCount returned 0x%x\n", numDevices );
        Pa_Terminate();
        return 0;
    }
    if( id >= numDevices ) {
        printf( "ERROR: Device ID is out of devices range: 0...%d\n", numDevices );
        Pa_Terminate();
        return 0;
    }

    deviceInfo = Pa_GetDeviceInfo( id );

    if(deviceInfo->maxOutputChannels <= 0) {
        printf( "ERROR: Device ID has no outputs\n" );
        Pa_Terminate();
        return 0;
    }

    outputParameters.device = id;
    if(Pa_IsFormatSupported( NULL, &outputParameters, SAMPLE_RATE ) != 0) {
        printf( "ERROR: Device ID does not support required audio format.\n" );
        Pa_Terminate();
        return 0;
    }
    /* Try to Open stream with required parameters */
    err = Pa_OpenStream( &stream,
                         0,
                         &outputParameters,
                         SAMPLE_RATE,
                         paFramesPerBufferUnspecified,
                         paNoFlag,
                         NULL,
                         (void *)NULL );
    Pa_CloseStream(&stream);
    if(err != paNoError) {
        printf( "ERROR: Device ID does not support required stream format.\n" );
        Pa_Terminate();
        return 0;
    }

    Pa_Terminate();
    return 1;
}
