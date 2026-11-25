/* engine_demo.c — full C-API workflow: append romanized input, list the
 * produced candidates, select one and commit it.
 *
 * Build & run (from a configured build directory):
 *   cmake --build . --target suborno_engine_demo
 *   ./examples/suborno_engine_demo ami bhai
 *
 * Only <suborno/suborno.h> is used, so the demo compiles against the
 * portable core on every supported platform.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <suborno/suborno.h>

static void print_candidates(SUBORNO_Session* session) {
    size_t count = 0;
    if (suborno_session_candidate_count(session, &count) < 0) {
        printf("no candidates\n");
        return;
    }
    printf("%zu candidate(s):\n", count);
    for (size_t index = 0; index < count; ++index) {
        char buffer[512];
        if (suborno_session_candidate_snapshot(session, index, buffer, sizeof(buffer)) >= 0) {
            printf("  %zu. %s\n", index + 1, buffer);
        }
    }
}

int main(int argc, char** argv) {
    const char* roman_input = argc > 1 ? argv[1] : "ami";

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

    if (suborno_session_append_utf8(session, roman_input) < 0) {
        fprintf(stderr, "append failed for \"%s\"\n", roman_input);
    } else {
        char composition[256];
        if (suborno_session_snapshot(session, composition, sizeof(composition)) >= 0) {
            printf("composition: %s\n", composition);
        }
        print_candidates(session);

        /* Committing consumes the pending composition and returns the
         * selected Bangla text — free it with suborno_string_free(). */
        char* committed = NULL;
        size_t committed_size = 0;
        if (suborno_session_commit(session, &committed, &committed_size) >= 0
            && committed != NULL) {
            printf("committed: %s\n", committed);
        } else {
            printf("nothing to commit\n");
        }
        suborno_string_free(committed);
    }

    suborno_session_destroy(session);
    suborno_engine_destroy(engine);
    return 0;
}
