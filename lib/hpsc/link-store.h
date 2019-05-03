#ifndef LINK_STORE_H
#define LINK_STORE_H

#include <stdbool.h>

#include <rtems.h>

#include "link.h"

// Link store is a central place to store link pointers -- it does not provide
// synchronization beyond maintaining internal consistency.
// Link pointers are stored in a chain (linked list), so access time is O(n).
// If link names are not unique, "get"/"extract" will return the first match.
// Link store functions may _not_ be called from an interrupt context.

/**
 * Append a link to the store.
 */
rtems_status_code link_store_append(struct link *link);

/**
 * Check if the store contains a link with the provided name.
 */
bool link_store_contains(const char *name);

/**
 * Check if the store is empty.
 */
bool link_store_is_empty(void);

/**
 * Get a link from the store with the provided name, or NULL.
 */
struct link *link_store_get(const char *name);

/**
 * Remove a link from the store with the provided name, or NULL.
 */
struct link *link_store_extract(const char *name);

/**
 * Remove the first link from the store, or NULL.
 * Primarily used in a loop to drain empty the store.
 */
struct link *link_store_extract_first(void);

#endif // LINK_STORE_H
