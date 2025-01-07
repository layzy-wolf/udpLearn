#include <stdio.h>
#include <signal.h>
#include "libserver/server.h"

int serverForward;

void main(void)
{
    signal(SIGINT, shutDownServerHandler);

    printf("server starts on port %d \r\n", PORT);

    addressList* addressList = create_address_list(10);

    serveServer(addressList, &serverForward);
}