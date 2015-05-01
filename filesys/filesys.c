#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
  // block_sector_t inode_sector = 0;
  // struct dir *dir = dir_open_root ();
  // // struct dir *dir = get_dir(name);
  // bool success = (dir != NULL
  //                 && free_map_allocate(1, &inode_sector)
  //                 && inode_create (inode_sector, initial_size)
  //                 && dir_add (dir, name, inode_sector));
  // if (!success && inode_sector != 0) 
  //   free_map_release (inode_sector, 1);
  // dir_close (dir);
  // return success;
  block_sector_t inode_sector = 0;
  if(!free_map_allocate(1, &inode_sector))
  {
    if(inode_sector != 0)
      free_map_release(inode_sector, 1);
    return false;
  }
  //create the inode
  if(!inode_create(inode_sector, initial_size))
  {
    if(inode_sector != 0)
      free_map_release(inode_sector, 1);
    return false;
  }

  char *file_name = NULL;
  char *save_ptr = NULL;
  block_sector_t curdir_sector = (*name == "/") ? ROOT_DIR_SECTOR : thread_current()->curdir_sector;
  struct inode *cur_inode = inode_open(curdir_sector);
  
  //parse the path
  char *path_cpy = malloc(strlen(name) + 1);
  strlcpy(path_cpy, name, strlen(name) + 1);
  // printf("path_cpy: %s\n", path_cpy);
  if(!parse(path_cpy, &cur_inode, &file_name, save_ptr))
  {
    if(inode_sector != 0)
      free_map_release(inode_sector, 1);
    return false;
  }

  //Adds this file to its new  directory
  struct dir *dir = dir_open(cur_inode);
  if(!dir_add(dir, file_name, inode_sector))
  {
    if(inode_sector != 0)
      free_map_release(inode_sector, 1);
    return false;
  }

  // printf("\tCreating File with name: %s, in dir_inode: %u\n", file_name, inode_get_inumber(cur_inode));
  dir_close(dir);
  return true;

}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  // struct dir *dir = dir_open_root ();
  // struct inode *inode = NULL;

  // if (dir != NULL)
  //   dir_lookup (dir, name, &inode);
  // dir_close (dir);

  // return file_open (inode);


  block_sector_t curdir_sector = (*name == "/") ? ROOT_DIR_SECTOR : thread_current()->curdir_sector;
  struct inode *cur_inode = inode_open(curdir_sector);
  
  //parse the path
  char *path_cpy = malloc(strlen(name) + 1);
  strlcpy(path_cpy, name, strlen(name) + 1);

  if(!end_parse(path_cpy, &cur_inode))
  {
    // printf("PARSING FAILED\n");
    return NULL;
  }

  return file_open (cur_inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir); 

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
