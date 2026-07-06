#include "fwd.h"
#include "server.h"

#include <wlr/util/log.h>

int main(void)
{
    wlr_log_init(WLR_DEBUG, NULL);

    IvyServer server = {0};

    Ivy_Server_Init(&server);

    Ivy_Server_Run(&server, NULL);

    Ivy_Server_Destroy(&server);

    return 0;
}
