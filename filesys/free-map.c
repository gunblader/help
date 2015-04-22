#include "filesys/free-map.h"
#include <bitmap.h>
#include <debug.h>
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"

static struct file *free_map_file;   /* Free map file. */
static struct bitmap *free_map;      /* Free map, one bit per sector. */

/* Initializes the free map. */
void
free_map_init (void) 
{
  free_map = bitmap_create (block_size (fs_device));
  if (free_map == NULL)
    PANIC ("bitmap creation failed--file system device is too large");
  bitmap_mark (free_map, FREE_MAP_SECTOR);
  bitmap_mark (free_map, ROOT_DIR_SECTOR);
}

/* Allocates CNT consecutive sectors from the free map and stores
   the first into *SECTORP.
   Returns true if successful, false if not enough consecutive
   sectors were available or if the free_map file could not be
   written. */
bool
free_map_allocate (size_t cnt, block_sector_t *sectorp)
{
  block_sector_t sector = bitmap_scan_and_flip (free_map, 0, cnt, false);
  if (sector != BITMAP_ERROR
      && free_map_file != NULL
      && !bitmap_write (free_map, free_map_file))
    {
      bitmap_set_multiple (free_map, sector, cnt, false); 
      sector = BITMAP_ERROR;
    }
  if (sector != BITMAP_ERROR)
    *sectorp = sector;
  return sector != BITMAP_ERROR;
}

/* Adam drove here */
/* Allocates free sectors into our structs in order:
        1. 10 direct_blocks
        2. 1 indirect block (128 blocks)
        3. An array of indirect blocks (128^2 blocks)
  Returns true if there is enough space and it was properly allocated
  false otherwise.
*/
bool
free_map_indirect_allocate(size_t sectors, block_sector_t *direct_blocks,
  struct indirect_block *first_level, struct indirect_block *second_level)
{
  if(bitmap_count(free_map, 0, bitmap_size(free_map), true) < sectors)
  {
    return false;
  }

  size_t count = 0;
  size_t first_index = 0;
  size_t second_level_index = 0;
  size_t second_level_offset = 0;

  //while the count is less than the number of total sectors to allocate
  while(count < sectors)
  {
    //find the next free sector (TRUE = free; false = taken)
    block_sector_t next_free = bitmap_scan_and_flip(free_map, 0, 1, true);
    if(next_free == BITMAP_ERROR)
      return false;
    //if we have allocated <=10 sectors, put them in direct_blocks
    if(count < 10)
    {
      //put them in direct_blocks
      direct_blocks[count] = next_free;
    }
    //else if we have allocated <= (10 + 128) sectors, put the in first level
    else if(count < 138)
    {
      //put in first_level
      first_level->blocks[first_index] = next_free;
      first_index++;
    }
    //else, >138 sectors have been allocated, so put the rest in second_level
    else
    {
      //put in second_level
      second_level[second_level_index].blocks[second_level_offset] = next_free;
      
      second_level_offset++;
      second_level_index = (second_level_offset == 128) ? second_level_index++ : second_level_index;
      second_level_offset = second_level_offset % 128;
    }
    count++;
  }

  return true;
}
/* End Adam driving */

/* Makes CNT sectors starting at SECTOR available for use. */
void
free_map_release (block_sector_t sector, size_t cnt)
{
  ASSERT (bitmap_all (free_map, sector, cnt));
  bitmap_set_multiple (free_map, sector, cnt, false);
  bitmap_write (free_map, free_map_file);
}

/* Opens the free map file and reads it from disk. */
void
free_map_open (void) 
{
  free_map_file = file_open (inode_open (FREE_MAP_SECTOR));
  if (free_map_file == NULL)
    PANIC ("can't open free map");
  if (!bitmap_read (free_map, free_map_file))
    PANIC ("can't read free map");
}

/* Writes the free map to disk and closes the free map file. */
void
free_map_close (void) 
{
  file_close (free_map_file);
}

/* Creates a new free map file on disk and writes the free map to
   it. */
void
free_map_create (void) 
{
  /* Create inode. */
  if (!inode_create (FREE_MAP_SECTOR, bitmap_file_size (free_map)))
    PANIC ("free map creation failed");

  /* Write bitmap to file. */
  free_map_file = file_open (inode_open (FREE_MAP_SECTOR));
  if (free_map_file == NULL)
    PANIC ("can't open free map");
  if (!bitmap_write (free_map, free_map_file))
    PANIC ("can't write free map");
}
