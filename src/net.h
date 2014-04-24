#ifndef NET_H
#define NET_H

#include "params.h"

#ifdef WIN32
   #include <winsock.h>
#else
   #include <sys/socket.h>
   #include <sys/un.h>
#endif

int send_stop(gydra_param_t* params);
int getdata_thread(gydra_param_t* params);

#endif
