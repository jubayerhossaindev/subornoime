/* c_api_example.c — smallest possible SubornoIME embedding.
 *
 * Build & run (from a configured build directory):
 *   cmake --build . --target suborno_c_api_example
 *   ./examples/suborno_c_api_example
 *
 * The example links the portable core (suborno_core) through the C API in
 * <suborno/suborno.h> only: it works unchanged on every platform the core
 * supports, without touching any TSF/COM headers.
 */
#include <stdio.h>
#include <string.h>
#include <suborno/suborno.h>

int main(void) {
    SUBORNO_Engine* engine = suborno_engine_create();
    if (engine == NULL) {
        fprintf(stderr, "engine creation failed\n");
        return 1;
    }

    SUBORNO_Session* session = suborno_session_create(engine);
    if (session == NULL) {
        fprintf(stderr, "session creation failed\n");
        suborno_engine_destroy(engine);
        return 1;
    }

    /* Append romanized input; the engine keeps it pending for candidates. */
    const int status = suborno_session_append_utf8(session, "ami");
    if (status < 0) {
        fprintf(stderr, "append failed: %d\n", status);
    } else {
        char composition[256];
        const int snapshot = suborno_session_snapshot(session, composition, sizeof(composition));
        if (snapshot >= 0) {
            printf("composition: %s\n", composition);
        }
    }

    suborno_session_destroy(session);
    suborno_engine_destroy(engine);
    return 0;
}
