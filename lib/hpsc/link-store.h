#ifndef LINK_STORE_H
#define LINK_STORE_H

#include <stdbool.h>

#include <rtems.h>

#include "link.h"

rtems_status_code link_store_append(struct link *link);

bool link_store_contains(const char *name);

bool link_store_is_empty(void);

struct link *link_store_get(const char *name);

struct link *link_store_extract(const char *name);

struct link *link_store_extract_first(void);

#endif // LINK_STORE_H
