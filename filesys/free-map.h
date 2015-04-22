#ifndef FILESYS_FREE_MAP_H
#define FILESYS_FREE_MAP_H

#include <stdbool.h>
#include <stddef.h>
#include "devices/block.h"

void free_map_init (void);
void free_map_read (void);
void free_map_create (void);
void free_map_open (void);
void free_map_close (void);

bool free_map_allocate (size_t, block_sector_t *);
/* Kenneth drove here*/
// bool free_map_indirect_allocate(size_t sectors, block_sector_t *direct_blocks,
// 	struct indirect_block *first_level, struct indirect_block *second_level);
/* End Kenneth driving*/
void free_map_release (block_sector_t, size_t);

#endif /* filesys/free-map.h */
