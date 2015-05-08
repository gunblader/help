#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/syscall.h"

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
  else
  {
    struct dir *root = dir_open_root();
  }

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
  // printf("Filesys_create: name: %s\n", name);
  block_sector_t inode_sector = 0;
  if(!free_map_allocate(1, &inode_sector))
  {
    if(inode_sector != 0)
      free_map_release(inode_sector, 1);
    return false;
  }
  //create the inode.
  if(!inode_create(inode_sector, initial_size))
  {
    if(inode_sector != 0)
      free_map_release(inode_sector, 1);
    return false;
  }

  char *file_name = NULL;
  block_sector_t curdir_sector = (*name == '/') ? 
    ROOT_DIR_SECTOR : thread_current()->curdir_sector;

  if(curdir_sector == NULL)
  {
    //we removed the current directory
    return NULL;
  }

  struct inode *cur_inode = inode_open(curdir_sector);
  
  set_isdir(inode_open(inode_sector), false);

  //parse the path
  char *path_cpy = malloc(strlen(name) + 1);
  strlcpy(path_cpy, name, strlen(name) + 1);
  // printf("path_cpy: %s\n", path_cpy);
  if(end_parse(path_cpy, &cur_inode, &file_name))
  {
    if(inode_sector != 0)
      free_map_release(inode_sector, 1);
    return false;
  }

  //Adds this file to its new  directory
  struct dir *dir = dir_open(cur_inode);
  if(!dir_add(dir, file_name, inode_sector))
  {
    // printf("file_name %s failed to be added to the directory\n", file_name);
    if(inode_sector != 0)
      free_map_release(inode_sector, 1);
    // dir_close(dir);
    return false;
  }

  // printf("\tCreating File with name: %s, in dir_inode: %u\n", file_name, inode_get_inumber(cur_inode));
  free(path_cpy);
  // dir_close(dir);
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

  // printf("FILESYS_OPEN\n");
  block_sector_t curdir_sector = (*name == '/') ? ROOT_DIR_SECTOR : thread_current()->curdir_sector;
  
  if(curdir_sector == NULL)
  {
    //we removed the current directory
    return NULL;
  }

  struct inode *cur_inode = inode_open(curdir_sector);
  
  //parse the path
  char *path_cpy = malloc(strlen(name) + 1);
  strlcpy(path_cpy, name, strlen(name) + 1);

  char *file_name = NULL;
  if(!end_parse(path_cpy, &cur_inode, &file_name))
  {
    // printf("PARSING FAILED\n");
    return NULL;
  }

  if(file_name == NULL)
  {
    //we know that we are trying to open root
    return file_open(inode_open(ROOT_DIR_SECTOR));
  }
  // printf("\tfile_name: %s\n", file_name);
  struct inode *temp = NULL;
  struct dir *dir = dir_open(cur_inode);
  if(!dir_lookup(dir, file_name, &temp)){
    // printf("filesys_open: LOOKUP FAILED\n");
    return NULL;
  }
  // printf("\t\tinode to open: %u\n", inode_get_inumber(temp));
  // if(curdir_sector != ROOT_DIR_SECTOR)
  //     dir_close(dir);
  // printf("END FILSYS_OPEN\n");
  free(path_cpy);
  return file_open (temp);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. NANANABOO*/
bool
filesys_remove (const char *name) 
{
  // struct dir *dir = dir_open_root ();
  // bool success = dir != NULL && dir_remove (dir, name);
  // dir_close (dir); 

  // return success;
  // printf("path: %s\n", name);
  block_sector_t curdir_sector = (*name == '/') ? ROOT_DIR_SECTOR : thread_current()->curdir_sector;
  
  if(curdir_sector == NULL)
  {
    //we removed the current directory
    return NULL;
  }

  struct inode *cur_inode = inode_open(curdir_sector);

  //parse the path
  char *file_name = NULL;

  char *path_cpy = malloc(strlen(name) + 1);
  strlcpy(path_cpy, name, strlen(name) + 1);
  bool success = false;
  
  if(!end_parse(path_cpy, &cur_inode, &file_name))
  {
    // printf("PARSING FAILED\n");
    return false;
  }

  if(file_name == NULL)
    return false;
  
  struct dir *dir = dir_open(cur_inode);

  /* Remove if the directory is empty */
  struct inode *inode = NULL;
  if(!dir_lookup(dir, file_name, &inode))
  {
    // printf("LOOKUP FAILED\n");
    return false;
  }
  if(get_isdir(inode))
  {
    char temp[NAME_MAX + 1];
    // if(get_isopen(inode))
    //   return false;
    struct dir *tempdir = dir_open(inode);
    // if(!get_removed(inode))
    //   return false;
    if(dir_readdir(tempdir, temp))
    {
      // printf("This directory is not empty\n");
      return false;
    }
    if(inode_get_inumber(inode) == thread_current()->curdir_sector)
    {
      //don't allow the removal of your cwd
      // printf("Don't allow removal of cwd\n");
      success = dir != NULL && dir_remove(dir, file_name);

      thread_current()->curdir_sector = NULL;
      return success;
    }
    
  }
  
  // printf("file_name: %s\n", file_name);
  success = dir != NULL && dir_remove(dir, file_name);
  free(path_cpy);
  // dir_close(dir);
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

  /* Kenneth drove here */
  struct dir *root = dir_open_root();
  
  if(!dir_add(root, ".", ROOT_DIR_SECTOR) 
    || !dir_add(root, "..", ROOT_DIR_SECTOR))
  {
    PANIC("ADDING . and .. TO ROOT FAILED");
  }
  // dir_close(root);
  /* End driving */

  free_map_close ();
  printf ("done.\n");
}
