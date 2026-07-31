#include "core/fwd.h"
#include "core/server.h"

int main(void)
{
    IvyServer server = {0};
    Ivy_Server_Init(&server);
    // Ivy_Server_Run(&server, NULL);
    // Ivy_Server_Destroy(&server);

    return 0;
}