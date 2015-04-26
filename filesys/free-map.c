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
  // printf("cnt: %u, BEFORE\n", cnt);
  // bitmap_dump(free_map);
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
  // printf("AFTER\n");
  // bitmap_dump(free_map);
  // printf("out yeah\n");
  return sector != BITMAP_ERROR;
}


/* Checks the free map to see if the given "sector" is free.
    If it is, we return that sector, else we return the next available
    sector */
block_sector_t
is_sector_free(block_sector_t sector)
{
  // printf("Asked for sector: %u\nBEFORE:\n", sector);
  // bitmap_dump(free_map);
  //if it is empty, return that sector
  if(!bitmap_test(free_map, sector) || sector == FREE_MAP_SECTOR
   || sector == ROOT_DIR_SECTOR)
  {
    return -1;
  }
  else
  {
    block_sector_t new_sector = bitmap_scan_and_flip(free_map, 0, 1, false);
    if (new_sector != BITMAP_ERROR
      && free_map_file != NULL
      && !bitmap_write (free_map, free_map_file))
    {
      // printf("Bitmap error\n");
      return false;
    }
    // printf("That sector is taken. New sector given: %u\n", new_sector);
    return new_sector;
  }
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
  block_sector_t *first_level, block_sector_t *second_level)
{
  if(bitmap_count(free_map, 0, bitmap_size(free_map), false) < sectors)
  {
    // printf("\tfree_map doesn't have %i free sectors\n", sectors);
    return false;
  }

  size_t count = 0;
  size_t first_index = 0;
  size_t second_level_index = 0;
  size_t second_level_offset = 0;
  struct indirect_block *first = malloc(sizeof(struct indirect_block));
  struct indirect_block *second = malloc(128 * sizeof(struct indirect_block));

  bool set_first = false;
  bool set_second = false;

  // printf("****FREE-MAP-IA****:\n\tSectors: %u\n", sectors);

  //while the count is less than the number of total sectors to allocate
  while(count < sectors)
  {
    //find the next free sector
    block_sector_t next_free = bitmap_scan_and_flip(free_map, 0, 1, false);
    if (next_free != BITMAP_ERROR
      && free_map_file != NULL
      && !bitmap_write (free_map, free_map_file))
    {
      // printf("Bitmap error\n");
      // bitmap_set_multiple (free_map, next_free, 1, false);
      return false;
    }
    //if we have allocated <=10 sectors, put them in direct_blocks
    if(count < 10)
    {
      //put them in direct_blocks
      direct_blocks[count] = next_free;
      // printf("\tdirect_block[%i] allocated to %u\n", count, next_free);
    }
    //else if we have allocated <= (10 + 128) sectors, put the in first level
    else if(count < 138)
    {
      //put in first_level
      first->blocks[first_index] = next_free;
      // printf("\tfirst_level->blocks[%i] allocated to %u\n", first_index, next_free);
      first_index++;
      set_first = true;
    }
    //else, >138 sectors have been allocated, so put the rest in second_level
    else
    {
      //put in second_level

      second[second_level_index].blocks[second_level_offset] = next_free;

      // printf("\tsecond_level[%u].blocks[%u] allocated to %u\n", second_level_index,
        // second_level_offset, next_free);
      
      second_level_offset++;
      second_level_index = (second_level_offset == 128) ? second_level_index++ : second_level_index;
      second_level_offset = second_level_offset % 128;
      set_second = true;
    }

    count++;
  }

  *first_level = bitmap_scan_and_flip(free_map, 0, 1, false);
  block_write(fs_device, *first_level, first);
  free(first);
  // printf("first level indirection block stored at sector %u\n", *first_level);

  if (*first_level != BITMAP_ERROR
    && free_map_file != NULL
    && !bitmap_write (free_map, free_map_file))
  {
    // printf("Bitmap error\n");
    return false;
  }

  *second_level = bitmap_scan_and_flip(free_map, 0, 1, false);
  block_write(fs_device, *second_level, second);
  free(second);
  // printf("second level indirection block stored at sector %u\n", *second_level);

  if (*second_level != BITMAP_ERROR
    && free_map_file != NULL
    && !bitmap_write (free_map, free_map_file))
  {
    // printf("Bitmap error\n");
    return false;
  }

  // printf("AFTER:\n");
  // bitmap_dump(free_map);
  // printf("****END FREE-MAP-IA****\n");
  return true;
}
/* End Adam driving */

/* Kenneth and Adam driving */
block_sector_t
append_to_free_map(size_t current_sectors, 
  block_sector_t* direct_blocks, block_sector_t* first_level, 
  block_sector_t* second_level)
{
  if(bitmap_count(free_map, 0, bitmap_size(free_map), false) < 1)
  {
    // printf("\tfree_map doesn't have %i free sectors\n", 1);
    return -1;
  }

  int count = 0;
  struct indirect_block *first = malloc(sizeof(struct indirect_block));
  struct indirect_block *second = malloc(128 * sizeof(struct indirect_block));
  size_t second_level_offset = 0;
  bool set_first = false;
  bool set_second = false;
  block_sector_t next_free = -1;

  // printf("*** IN APPEND_TO_FREE_MAP ***\n");
  // printf("\t*first_level: %u\n", *first_level);
  // printf("\t*second_level: %u\n", *second_level);

  if(first_level != NULL && first != NULL)
    block_read(fs_device, *first_level, first);
  if(second_level != NULL && second != NULL)
    block_read(fs_device, *second_level, second);

  // allocated count sectors to this inode
  next_free = bitmap_scan_and_flip(free_map, 0, 1, false);

  if (next_free != BITMAP_ERROR
        && free_map_file != NULL
        && !bitmap_write (free_map, free_map_file))
  {
        // printf("Bitmap error\n");
        return false;
  }

  // printf("\tnext_free: %u\n", next_free);
  if(current_sectors < 10)
  {
    direct_blocks[current_sectors] = next_free;
    // printf("\tdirect_blocks[%u] = sector %u\n", current_sectors, direct_blocks[current_sectors]);
  }
  else if(current_sectors < 138)
  {
    //put in first_level
    first->blocks[current_sectors-10] = next_free;
    set_first = true;
  }
  else
  {
    //put in second_level
    second_level_offset = (current_sectors-138) % 128;
    second[(current_sectors-138)/128].blocks[second_level_offset] = next_free;
    set_second = true;
  }

  if (first_level != NULL)
  {
    // *first_level = bitmap_scan_and_flip(free_map, 0, 1, false);
    block_write(fs_device, *first_level, first);
    free(first);
  }

  if (second_level != NULL)
  {
    // *second_level = bitmap_scan_and_flip(free_map, 0, 1, false);
    block_write(fs_device, *second_level, second);
    free(second);
  }

  // printf("*** END OF APPEND_TO_FREE_MAP ***\n");
  return next_free;
}
/* End Kenneth and Adam driving */



/* Makes CNT sectors starting at SECTOR available for use. */
void
free_map_release (block_sector_t sector, size_t cnt)
{
  ASSERT (bitmap_all (free_map, sector, cnt));
  bitmap_set_multiple (free_map, sector, cnt, false);
  bitmap_write (free_map, free_map_file);
}

// #Kenneth Drove here
void
free_map_indexed_release(block_sector_t *direct_blocks,
  block_sector_t *first_level, block_sector_t *second_level, size_t sectors)
{
  // printf("*****IN FREE MAP RELEASE*****\n");
  struct indirect_block *first = malloc(sizeof(struct indirect_block));
  block_read(fs_device, *first_level, first);
  struct indirect_block *second = malloc(128 * sizeof(struct indirect_block));
  block_read(fs_device, *second_level, second);

  size_t second_level_index = 0;
  size_t second_level_offset = 0;

  size_t count = 0;
  while(count < sectors)
  {
    if(count < 10)
    {
      // printf("previous setting: %i\n", bitmap_test(free_map, direct_blocks[count]));
      bitmap_set(free_map, direct_blocks[count], false);
    }
    else if(count < 138)
    {
      // printf("previous setting: %i\n", bitmap_test(free_map, first->blocks[count - 10]));
      bitmap_set(free_map, first->blocks[count - 10], false);
    }
    else
    {
      // printf("previous setting: %i\n", bitmap_test(free_map, second[second_level_index].blocks[second_level_offset]));
      bitmap_set(free_map, second[second_level_index].blocks[second_level_offset], false);
      second_level_offset++;
      second_level_index = (second_level_offset == 128) ? second_level_index++ : second_level_index;
      second_level_offset = second_level_offset % 128;
    }
  }
  bitmap_write(free_map, free_map_file);
}

/* Opens the free map file and reads it from disk. */
void
free_map_open (void) 
{
  free_map_file = file_open (inode_open (FREE_MAP_SECTOR));
  if (free_map_file == NULL)
    PANIC ("can't open free map");
  // printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<DOES READS>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
  if (!bitmap_read (free_map, free_map_file))
    PANIC ("can't read free map");
  // printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Finished READS>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
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
    // printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<DOES WRITES>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
  if (!bitmap_write (free_map, free_map_file))
    PANIC ("can't write free map");
    // printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Finished WRITES>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

}
