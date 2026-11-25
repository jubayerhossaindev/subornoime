// Suborno IME — public C API.
//
// This header is the stable ABI surface of the Suborno engine library.
// Identifiers and enumeration values here are external contracts and must
// not be renamed; internal C++ code follows the project naming standard.
//
// SPDX-License-Identifier: MIT
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#if defined(SUBORNO_API_EXPORTS)
#define SUBORNO_API __declspec(dllexport)
#elif !defined(SUBORNO_API_STATIC)
#define SUBORNO_API __declspec(dllimport)
#else
#define SUBORNO_API
#endif
#else
#define SUBORNO_API
#endif

/** Opaque engine handle: owns settings, layouts, dictionary and learning state. */
typedef struct SUBORNO_Engine SUBORNO_Engine;

/** Opaque typing session bound to one engine. */
typedef struct SUBORNO_Session SUBORNO_Session;

/**
 * Status and error codes returned by every suborno_session_* function.
 *
 * Non-negative values describe the input-processing outcome; negative values
 * are API-level errors.
 */
typedef enum SUBORNO_Status {
    SUBORNO_STATUS_UPDATED = 0, /**< Composition text changed. */
    SUBORNO_STATUS_PENDING = 1, /**< Input buffered, no output yet. */
    SUBORNO_STATUS_LITERAL_PASSTHROUGH = 2, /**< Keystrokes emitted verbatim. */
    SUBORNO_STATUS_INVALID_UTF8 = 3, /**< Input was not valid UTF-8. */
    SUBORNO_STATUS_INPUT_LIMIT = 4, /**< Composition buffer limit reached. */
    SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE = 5, /**< Candidate index does not exist. */
    SUBORNO_STATUS_EMPTY = 6, /**< Nothing to commit. */
    SUBORNO_ERROR_INVALID_ARGUMENT = -1, /**< NULL or otherwise invalid argument. */
    SUBORNO_ERROR_BUFFER_TOO_SMALL = -2, /**< Output buffer cannot hold the text. */
    SUBORNO_ERROR_INTERNAL = -3 /**< Unexpected internal failure. */
} SUBORNO_Status;

/**
 * @brief Creates an engine with the default configuration.
 *
 * @return New engine handle, or NULL when construction fails.
 * @note The returned handle must be released with suborno_engine_destroy().
 */
SUBORNO_API SUBORNO_Engine* suborno_engine_create(void);

/**
 * @brief Destroys an engine created by suborno_engine_create().
 *
 * @param engine Engine handle, or NULL (no-op).
 */
SUBORNO_API void suborno_engine_destroy(SUBORNO_Engine* engine);

/**
 * @brief Creates a typing session bound to the given engine.
 *
 * @param engine Engine handle created by suborno_engine_create().
 * @return New session handle, or NULL when @p engine is NULL or construction
 *         fails.
 * @note The returned handle must be released with suborno_session_destroy().
 */
SUBORNO_API SUBORNO_Session* suborno_session_create(SUBORNO_Engine* engine);

/**
 * @brief Destroys a session created by suborno_session_create().
 *
 * @param session Session handle, or NULL (no-op).
 */
SUBORNO_API void suborno_session_destroy(SUBORNO_Session* session);

/**
 * @brief Appends UTF-8 phonetic input to the active composition.
 *
 * @param session Session handle.
 * @param input NUL-terminated UTF-8 text supplied by the user.
 * @return SUBORNO_Status value describing the outcome; SUBORNO_ERROR_INVALID_ARGUMENT
 *         when @p session or @p input is NULL.
 */
SUBORNO_API int suborno_session_append_utf8(SUBORNO_Session* session, const char* input);

/**
 * @brief Erases the last composition character.
 *
 * @param session Session handle.
 * @return SUBORNO_Status value describing the outcome; SUBORNO_ERROR_INVALID_ARGUMENT
 *         when @p session is NULL.
 */
SUBORNO_API int suborno_session_backspace(SUBORNO_Session* session);

/**
 * @brief Copies the current preedit (composition) text into a caller buffer.
 *
 * @param session Session handle.
 * @param buffer Output buffer; always NUL-terminated on return, even on failure.
 * @param buffer_size Size of @p buffer in bytes, including the terminating NUL.
 * @return SUBORNO_Status value describing the outcome;
 *         SUBORNO_ERROR_BUFFER_TOO_SMALL when the preedit text does not fit.
 */
SUBORNO_API int suborno_session_snapshot(SUBORNO_Session* session,
                                         char* buffer,
                                         size_t buffer_size);

/**
 * @brief Writes the number of currently available candidates.
 *
 * @param session Session handle.
 * @param count Receives the candidate count; set to 0 before validation.
 * @return SUBORNO_Status value describing the outcome; SUBORNO_ERROR_INVALID_ARGUMENT
 *         when @p session or @p count is NULL.
 */
SUBORNO_API int suborno_session_candidate_count(SUBORNO_Session* session, size_t* count);

/**
 * @brief Copies one candidate's text into a caller buffer.
 *
 * @param session Session handle.
 * @param index Zero-based candidate index.
 * @param buffer Output buffer; always NUL-terminated on return, even on failure.
 * @param buffer_size Size of @p buffer in bytes, including the terminating NUL.
 * @return SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE when @p index is invalid,
 *         SUBORNO_ERROR_BUFFER_TOO_SMALL when the text does not fit, or another
 *         SUBORNO_Status value describing the outcome.
 */
SUBORNO_API int suborno_session_candidate_snapshot(SUBORNO_Session* session,
                                                   size_t index,
                                                   char* buffer,
                                                   size_t buffer_size);

/**
 * @brief Selects the candidate at the given index for the next commit.
 *
 * @param session Session handle.
 * @param index Zero-based candidate index.
 * @return SUBORNO_STATUS_UPDATED on success, SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE
 *         when @p index is invalid, or SUBORNO_ERROR_INVALID_ARGUMENT when
 *         @p session is NULL.
 */
SUBORNO_API int suborno_session_select_candidate(SUBORNO_Session* session, size_t index);

/**
 * @brief Commits the active composition and returns the Bangla text.
 *
 * @param session Session handle.
 * @param committed Receives a NUL-terminated UTF-8 allocation on success; set to
 *        NULL before validation and on failure.
 * @param committed_size Receives the byte length of @p committed (without the
 *        NUL); set to 0 before validation and on failure.
 * @return SUBORNO_Status value describing the outcome; SUBORNO_ERROR_INVALID_ARGUMENT
 *         when @p session, @p committed or @p committed_size is NULL.
 * @note On success the caller must release @p committed with suborno_string_free().
 */
SUBORNO_API int suborno_session_commit(SUBORNO_Session* session,
                                       char** committed,
                                       size_t* committed_size);

/**
 * @brief Commits the candidate at the given index and returns the Bangla text.
 *
 * @param session Session handle.
 * @param index Zero-based candidate index.
 * @param committed Receives a NUL-terminated UTF-8 allocation on success; set to
 *        NULL before validation and on failure.
 * @param committed_size Receives the byte length of @p committed (without the
 *        NUL); set to 0 before validation and on failure.
 * @return SUBORNO_Status value describing the outcome; SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE
 *         when @p index is invalid.
 * @note On success the caller must release @p committed with suborno_string_free().
 */
SUBORNO_API int suborno_session_commit_candidate(SUBORNO_Session* session,
                                                 size_t index,
                                                 char** committed,
                                                 size_t* committed_size);

/**
 * @brief Frees a text buffer returned by suborno_session_commit() or
 *        suborno_session_commit_candidate().
 *
 * @param text Allocation previously returned through @p committed, or NULL
 *        (no-op).
 */
SUBORNO_API void suborno_string_free(char* text);

#ifdef __cplusplus
}
#endif
