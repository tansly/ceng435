#include "globals.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


/* TODO: Command line options.
 */
int main(void)
{
    int status = 0;
    status = start_server("0.0.0.0", "26598", 5);
    if (status != 0) {
        fprintf(stderr, "Failed to start server!\n");
        return 1;
    }
    /* The server has been started.
     * Time to start the main loop of the program.
     * This will accept() clients and start recv()'ing and playing.
     */
    main_loop();
    // THOU SHALT NOT PASS
    assert(0);
    return 1;
}
