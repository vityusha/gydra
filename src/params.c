#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>

#ifdef _WIN32
  #include <winsock2.h>
#else
  #include <sys/socket.h>
  #include <arpa/inet.h>
#endif

#include <portaudio.h>

#include "params.h"
#include "devs.h"

const char *test_mode = "test";
const char *real_mode = "real";
#define MODE_LENGTH 4

static void print_version()
{
    fprintf(stderr,
"\nGydra v.%s - utility to play network audio stream from special net server.\n"
"\nGydra is licensed under Modified BSD License.\n"
"(c) 2014 \"N Project\" author: Vityusha V. Vinokurov\n", VERSION);
}

static void print_usage()
{
    fprintf(stderr, 
"\nGydra - utility to play network audio stream from ...\n"
"\nUsage: gydra [options]\n\n"
"-l, --list-devices\t- list of compatible audio devices\n"
"-i, --ip <x.x.x.x>\t- IP address of net audio server (default 192.168.4.3)\n"
"-p, --port <n>\t\t- port of net audio server (default 257)\n"
"-d, --device <n>\t- audio device id (see --list-devices) for audio output\n"
"\t\t\t  if not specified the system default audio output\n"
"\t\t\t  will be used\n"
"-t, --time <n>\t\t- time in seconds for audio output before program exit\n"
"-n, --num-packets <n>\t- number of packets to be output before program exit\n"
"-m, --mode <test|real>\t- operation mode, real mode is default\n"
"-s, --stop\t\t- stop audio stream from remote engine\n"
"--out-file [filename]\t- write received data to local file\n"
"\t\t\t  allowed only when --num--packets or --time is defined\n"
"\t\t\t  if no filename specified then default \"gydra.raw\""
"\t\t\t  will be used\n"
"\n"
"-h, --help\t\t- print this help message\n"
"-v, --version\t\t- print program version and copyright and exit\n");
}

static bool isValidIpAddress(char *ipAddress)
{
    unsigned long result = inet_addr(ipAddress);
    return result != 0 && strlen(ipAddress) != 0;
}

void default_params(gydra_param_t* params)
{
    params->state             = STATE_NORMAL;
    params->ringBufferData    = NULL;
    params->starttime         = time(NULL);
    /* Program switches */
    params->verbose   = 0;
    params->id        = -1;
    params->nsec      = -1;
    params->npack     = -1;
    params->mode      = REAL_MODE;
    strcpy(params->ip, "192.168.4.3");
    params->port      = 257;
    params->stop      = 0;
    strcpy(params->file, "gydra.raw");
    params->writefile = 0;
}

int get_params(int argc, char** argv, gydra_param_t* params)
{
    bool printusage   = false;
    bool listdevices  = false;
    bool printversion = false;

    int tmpint = 0;

    static int verbose_flag = 0;

    /* Assign default params */
    default_params(params);

    /* Parsing command line switches */
    int c;
    while (1)
    {
        static struct option long_options[] =
        {
            /* These options set a flag. */
            {"verbose", no_argument,       &verbose_flag, 1},
            {"brief",   no_argument,       &verbose_flag, 0},
            /* These options don't set a flag.
                   We distinguish them by their indices. */
            {"help",          no_argument,       0, 'h'},
            {"list-devices",  no_argument,       0, 'l'},
            {"device",        required_argument, 0, 'd'},
            {"time",          required_argument, 0, 't'},
            {"ip",            required_argument, 0, 'i'},
            {"port",          required_argument, 0, 'p'},
            {"packets",       required_argument, 0, 'n'},
            {"mode",          required_argument, 0, 'm'},
            {"stop",          required_argument, 0, 's'},
            {"out-file",      optional_argument, 0, 0},
            {"version",       no_argument,       0, 'v'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "hld:i:p:t:n:m:sv",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
                break;
            /* File name option */
            if (strncmp(long_options[option_index].name, "out-file", 8) == 0) {
                params->writefile = 1;
                if(optarg != NULL && strlen(optarg) != 0) 
                    strncpy(params->file, optarg, MAX_PATH);
                printf("The output will be written to \"%s\" file!\n", params->file);
            }
            break;

        case 'h':
            printusage = true;
            break;

        case 'l':
            listdevices = true;
            break;

        case 'v':
            printversion = true;
            break;

        case 'p':
            tmpint = atoi(optarg);
            if(tmpint > 0 && tmpint < USHRT_MAX) {
                params->port = tmpint;
            } else {
                printf ("ERROR: Net server port must be in range %d...%d!\n", 1, USHRT_MAX);
                printusage = true;
            }
            break;

        case 'i':
            if(isValidIpAddress(optarg)) {
                strncpy(params->ip, optarg, sizeof(params->ip));
            } else {
                printf ("ERROR: Not valid IP address specified!\n");
                printusage = true;
            }
            break;

        case 't':
            tmpint = atoi(optarg);
            if(tmpint > 0 && tmpint < MAX_TIME) {
                params->nsec = tmpint;
            } else {
                printf ("ERROR: Time in seconds must be in range 1...%d!\n", MAX_TIME);
                printusage = true;
            }
            break;

        case 'n':
            tmpint = atoi(optarg);
            if(tmpint > 0 && tmpint < MAX_PACKETS) {
                params->npack = tmpint;
            } else {
                printf ("ERROR: Number of packets must be in range 1...%d!\n", MAX_PACKETS);
                printusage = true;
            }
            break;

        case 'm':
            if(!strncmp(optarg, test_mode, MODE_LENGTH))
                params->mode = TEST_MODE;
            else if(!strncmp(optarg, real_mode, MODE_LENGTH))
                params->mode = REAL_MODE;
            else {
                printf ("ERROR: Not allowed operation mode specified!\n");
                printusage = true;
            }
            break;

        case 'd':
            tmpint = atoi(optarg);
            if(check_device(tmpint)) {
                params->id = tmpint;
            } else {
                printf ("Invalid Device ID!\n");
                listdevices = true;
            }
            break;

        case 's':
            params->stop = 1;
            break;
        }
    }
    params->verbose = verbose_flag;

    /* Some specific switch logic */
    if(printusage) {
        print_usage();
        exit(0);
    }
    if(printversion) {
        print_version();
        exit(0);
    }
    if(listdevices) {
        list_devices();
        exit(0);
    }

    /* Check for required params */
    if(!(params->port > 0 && params->port < USHRT_MAX)) {
        printf("Net audio server port must be specified!\n");
        exit(0);
    }
    if(!isValidIpAddress(params->ip)) {
        printf("Net audio server IP address must be specified!\n");
        exit(0);
    }
    /* Write file allowed only when time restricted */
    if(params->writefile && params->nsec == -1 && params->npack == -1) {
        printf("Writing audio file is allowed only when --num-packets or --time is defined!\n");
        params->writefile = 0;
    }

    return 0;
}
