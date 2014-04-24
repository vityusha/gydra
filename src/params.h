#ifndef PARAMS_H
#define PARAMS_H

#include <time.h>
#include <limits.h>
#include <portaudio.h>

#include "protocol.h"
#include <pa_ringbuffer.h>

#define SAMPLE_RATE       (double)30050.0
#define SAMPLE_FORMAT     paInt24
#define BYTES_PER_SAMPLE  3
#define NUM_CHANNELS      (int)1
#define FRAMES_PER_BUFFER (AUDIO_LENGTH / BYTES_PER_SAMPLE)

/* Set beffer large enouth and more then packet size */
#define BUFFER_SIZE        AUDIO_LENGTH
#define BUFFERS_COUNT      16

#define MAX_TIME    1000
#define MAX_PACKETS USHRT_MAX

#ifndef MAX_PATH
#define MAX_PATH 255
#endif

/*@T
 * \section{Gydra parameters}
 * 
 * The [[solve_param_t]] structure holds the parameters that
 * describe the simulation.  These parameters are filled in
 * by the [[get_params]] function.  Details of the parameters
 * are described elsewhere in the code.
 *@c*/
enum {                /* Types of programs mode available: */
    TEST_MODE = 1,    /* 1. Test mode                      */
    REAL_MODE = 2     /* 2. Normal working mode            */
};

enum {                   /* Types of programs mode available: */
    STATE_NORMAL = 0,    /* 1. Normal working state           */
    STATE_BREAK  = 1     /* 2. Signal to break thread         */
};

typedef struct gydra_param_t {
    /* Signal for other threads */
    int        state;                   /* IPC signal */
    /* Ring buffer (FIFO) for "communicating" towards audio callback */
    PaStream*        stream;            /* PortAudio stream */
    PaUtilRingBuffer ringBuffer;        /* Ring buffer for audio data */
    void*            ringBufferData;    /* Audiop data */
    time_t     starttime;  /* Program starting time */
    /* Program swithes */
    int    verbose;        /* Verbose output */
    int    id;             /* Portaudio Device ID. -1 means default audio device*/
    int    nsec;           /* Time in seconds before program exit */
    int    npack;          /* Number of recieved packets before program exit */
    int    mode;           /* Program mode test/real */
    char   ip[16];         /* IP address of the nry audio server */
    int    port;           /* Port of the net audio server */
    int    stop;           /* Send stop signal flag */
    int    writefile;      /* Flag to create file */
    char   file[MAX_PATH]; /* File path for save */
} gydra_param_t;

int get_params(int argc, char** argv, gydra_param_t* params);

/*@q*/
#endif /* PARAMS_H */
