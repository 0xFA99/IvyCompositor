#include "core/server.h"
#include "core/output.h"

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>

static void IvyOutputManager_HandleNewOutput(struct wl_listener *listener, void *data);

void Ivy_OutputManager_Init(IvyOutputManager *output_manager)
{
    IvyServer *server = wl_container_of(output_manager, server, output_manager);

    wl_list_init(&output_manager->list);

    output_manager->new_output.notify = IvyOutputManager_HandleNewOutput;
    wl_signal_add(&server->core.wlr_backend->events.new_output, &output_manager->new_output);
}

static void IvyOutputManager_HandleNewOutput(struct wl_listener *listener, void *data)
{
    // TODO: ....
}
