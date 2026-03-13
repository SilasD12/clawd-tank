// simulator/sim_socket.h
#ifndef SIM_SOCKET_H
#define SIM_SOCKET_H

#include <stdbool.h>

#define SIM_SOCKET_DEFAULT_PORT 19872

// Start TCP listener on given port. Spawns a background thread.
// Returns 0 on success, -1 on error.
int sim_socket_init(int port);

// Drain any queued events from the socket thread.
// Call from the main loop before ui_manager_tick().
// Returns true if any event was processed.
bool sim_socket_process(void);

// Stop listener, close sockets, join thread.
void sim_socket_shutdown(void);

#endif
