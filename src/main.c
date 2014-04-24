#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>

#include "audio.h"
#include "net.h"

void INThandler(int);
static int *stopSignal = NULL;

/*******************************************************************/

int main(int argc, char **argv)
{
    int err = 0;

    /* Parsing command line switches */
    gydra_param_t params;
    memset(&params, 0, sizeof(gydra_param_t));

    get_params(argc, argv, &params);

    /* If stop signal is requested send it and exit */
    if(params.stop) {
        if(!send_stop(&params)) {
            fprintf(stderr, "Error - send_stop()\n");
            exit(-1);
        }
        printf("Successfully sent stop signal to remote audio server.\n");
        exit(0);
    }

    /* Catch Ctrl-C signal for graceful shutdown */
    stopSignal = &params.state;
    signal(SIGINT, INThandler);

    /* Init audio engine */
    if(init_audio(&params) != 0) {
        printf("Error during audio initialization. Exiting!\n");
        exit(-1);
    }

    /* Create net reading thread */
    err = getdata_thread(&params);
    if(err) {
        printf("Error in getting audio data code: %d\n", err);
    }

    /* Shutdown code */
    if(close_audio(&params) != 0) {
        printf("Error during audio initialization. Exiting!\n");
        exit(-1);
    }

    return err;
}

void  INThandler(int sig)
{
     signal(sig, SIG_IGN);
     *stopSignal = STATE_BREAK;
}
