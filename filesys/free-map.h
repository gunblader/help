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
bool free_map_indirect_allocate(size_t sectors, block_sector_t *direct_blocks,
	block_sector_t first_level, block_sector_t second_level);

block_sector_t is_sector_free(block_sector_t sector);

block_sector_t append_to_free_map(size_t current_sectors, 
	block_sector_t* direct_blocks, block_sector_t first_level, 
	block_sector_t second_level);

void free_map_indexed_release(block_sector_t *direct_blocks,
  block_sector_t first_level, block_sector_t second_level, size_t sectors);
/* End Kenneth driving*/
void free_map_release (block_sector_t, size_t);

#endif /* filesys/free-map.h */
