#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/thread.h"
#include "threads/malloc.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
{
	/* Kenneth and Adam drove here*/
	block_sector_t direct_blocks[10]; /* Hold the 10 direct blocks */
	block_sector_t first_level; /* Holds the location of the sector for the first indirection block*/
	block_sector_t second_level; /* Holds the location of the sector for the second indirection block*/

	//struct indirect_block *first_level; /* A pointer to an indirect_block that points to 128 blocks */
	// struct indirect_block *second_level; /* An array of indirect_blocks (128^2 total blocks) */
	/* End Driving*/

	block_sector_t start;               /* First data sector. */
	off_t length;                       /* File size in bytes. */
	unsigned magic;                     /* Magic number. */
	uint32_t unused[113];               /* Not used. */
};



/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
	static inline size_t
bytes_to_sectors (off_t size)
{
	return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
{
	struct list_elem elem;              /* Element in inode list. */
	block_sector_t sector;              /* Sector number of disk location. */
	int open_cnt;                       /* Number of openers. */
	bool removed;                       /* True if deleted, false otherwise. */
	int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
	struct inode_disk data;             /* Inode content. */
	bool isdir;
	off_t pos;
};


bool 
set_isdir(struct inode *inode, bool isdir)
{
	inode->isdir = isdir;
}

bool
get_isdir(struct inode *inode)
{
	return inode->isdir;
}

struct inode_disk *
get_data(struct inode *inode)
{
	return &inode->data;
}

block_sector_t
get_sector_from_index(struct inode *inode, block_sector_t sector)
{
		// printf("**** IN GET SECTOR FROM INDEX ****\n");
		// printf("\tsector: %u\n", sector);
		if(sector < 10)
		{
			// size_t i;
			// for(i = 0; i < 10; i++)
			// {
			//   printf("\tdirect_blocks[%u] = sector %u\n", i, inode->data.direct_blocks[i]);
			// }
			// printf("\treturning direct_sector[%u] = sector %u\n", sector, inode->data.direct_blocks[sector]);


			return inode->data.direct_blocks[sector];
		}
		else if (sector < 138)
		{
			// struct indirect_block *first = malloc(sizeof(struct indirect_block));
			block_sector_t first[128];
			// struct indirect_block first[128];
			// printf("\tfirst level sector: %u\n", inode->data.first_level);
			block_read(fs_device, inode->data.first_level, first);
			// size_t i;
			// for(i = 0; i < 128; i++)
			// {
			//   printf("\tfirst_level->blocks[%u] = sector %u\n", i, first->blocks[i]);
			// }

			block_sector_t temp = first[sector - 10];
			// free(first);
			// printf("\treturning first_level[%u] = sector %u\n", sector - 10, temp);
			return temp;
		}
		else
		{
			// printf("second_level[(sector - 138)/128] = sector %u\n", inode->data.second_level[(sector - 138)/128]);

			// struct indirect_block *second = malloc(128 * sizeof(struct indirect_block));
			block_sector_t second[128];
			block_read(fs_device, inode->data.second_level, second);
			// printf("This first level is stored at sector %u\n", second[(sector-138)/128]);
			// printf("\treturning second_level sector %u\n", second[(sector - 138)/128].blocks[(sector - 138) % 128]);
			block_sector_t first[128];
			block_read(fs_device, second[(sector - 138)/128], first);

			// block_sector_t temp = second[(sector - 138)/128].blocks[(sector-138) % 128];
			block_sector_t temp = first[(sector-138) % 128];
			// printf("returning second[%u] --> first[%u] = %u\n", (sector-138)/128, (sector-138) % 128, temp);
			// free(second);
			return temp;
		}
}

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
	static block_sector_t
byte_to_sector (struct inode *inode, off_t pos, bool write) 
{
	ASSERT (inode != NULL);

	// printf("****BYTE_TO_SECTOR****:\n");
	// printf("\tinode length is: %i\n", inode->data.length);
	block_sector_t sector = pos / BLOCK_SECTOR_SIZE;
	
	if (pos < inode->data.length)
	{
		// return inode->data.start + pos / BLOCK_SECTOR_SIZE;
		// return the sector that holds the byte that we need
		return get_sector_from_index(inode, sector);
	}
	else
	{
		//if this is a read, return -1
		if(!write)
		{
			return -1;
		}
		// printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>GROWING FILE>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		//grow the file
		if(pos > MAX_FILE_SIZE){
			PANIC ("Trying to extend past maximum file size");
		}
		size_t old_sectors = bytes_to_sectors(inode->data.length);
		block_sector_t next_free;

		//check if there is enough free blocks to allocate to
		// printf("\told_sectors: %u\n", old_sectors);
		off_t rounded_up_length = DIV_ROUND_UP(inode->data.length, BLOCK_SECTOR_SIZE) * 512;
		// printf("\tRounded up length: %u\n", rounded_up_length);
		if(pos < rounded_up_length)
		{
			next_free = get_sector_from_index(inode, sector);
			// printf("\tThere is still a sector thats not full, return sector: %u\n", next_free);
		}
		else
		{
			next_free = append_to_free_map(old_sectors, inode->data.direct_blocks, 
				&inode->data.first_level, &inode->data.second_level);
			// printf("\tAppending a new sector to end of file: %u\n", next_free);

		}
		// block_write(fs_device, inode->sector, &inode->data);
		// inode->data.length += (pos - inode->data.length);
		// printf("\tUPDATED LENGTH: %d\n", inode->data.length);

		//update its length to reflect the changes of file growth
		// inode->data.length += 512;

		// printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<END GROWING FILE<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
		if(next_free > 0)
		{
			// printf("returning next_free sector: %u\n", next_free);
			return next_free;
		}
		else
		{
			PANIC ("Ran out of file system disk space");
		}
	}
	// printf("****END BYTE_TO_SECTOR****\n\n");
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
	void
inode_init (void) 
{
	list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
{
	struct inode_disk *disk_inode = NULL;
	bool success = false;

	ASSERT (length >= 0);

	// printf("\n\n\n****INODE_CREATE****:\n\n\n");
	// sector = is_sector_free(sector);

	// printf("sector: %u, length: %u\n", sector, length);
	// 
	/* If this assertion fails, the inode structure is not exactly
	   one sector in size, and you should fix that. */
	ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

	disk_inode = calloc (1, sizeof *disk_inode);
	if (disk_inode != NULL)
	{
		// printf("\tTHREAD NAME: %s\n", thread_current()->name);
		size_t sectors = bytes_to_sectors (length);
		// printf("\tsectors: %i\n", sectors);
		disk_inode->length = length;
		// printf("\tdisk_inode length: %i\n", disk_inode->length);
		disk_inode->magic = INODE_MAGIC;

		// disk_inode->first_level = malloc(sizeof(struct indirect_block));
		// disk_inode->second_level = malloc(sizeof(struct indirect_block));
		//if (free_map_allocate (sectors, &disk_inode->start))
		// This checks the free list and allocates free blocks into our structs
		if(free_map_indirect_allocate(sectors, disk_inode->direct_blocks,
					&disk_inode->first_level, &disk_inode->second_level))
		{
			// printf("%d\n", sector);
			success = true;

		} 
		block_write (fs_device, sector, disk_inode);
		// free(disk_inode->first_level);
		// free(disk_inode->second_level);
		free (disk_inode);
	}
	// printf("\n\n****END INODE CREATE****\n\n");
	return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
	struct inode *
inode_open (block_sector_t sector)
{
	// printf("****INODE_OPEN****:\n");
	struct list_elem *e;
	struct inode *inode;

	/* Check whether this inode is already open. */
	// print_open_list();
	for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
			e = list_next (e)) 
	{
		inode = list_entry (e, struct inode, elem);
		if (inode->sector == sector) 
		{
			// printf("Reopening previous open inode\n");
			inode_reopen (inode);
			return inode; 
		}
	}

	/* Allocate memory. */
	inode = malloc (sizeof *inode);
	if (inode == NULL)
		return NULL;

	/* Initialize. */
	list_push_front (&open_inodes, &inode->elem);
	inode->sector = sector;
	inode->open_cnt = 1;
	inode->deny_write_cnt = 0;
	inode->removed = false;
	inode->pos = 0;
	// printf("\tinode sector: %u\n", inode->sector);
	block_read (fs_device, inode->sector, &inode->data);
	// printf("\tinode data length: %u\n", inode->data.length);

	// int i = 0;
	// while(i < 10){
	//   printf("IN OPEN ---> direct_blocks[%u] = sector %u\n", i, inode-<data.direct_blocks[i]);
	//   i++;
	// }

	// printf("****END INODE OPEN****\n\n");
	// print_open_list();
	return inode;
}

// void
// print_open_list()
// {
// 	// printf("*****PRINTING OPEN INODES*****\n");
// 	if(list_empty(&open_inodes))
// 	{
// 		// printf("INODE LIST IS EMPTY\n");
// 		// printf("*****DONE PRINTING OPEN INODES*****\n");
// 		return;
// 	}
// 	struct inode* inode;
// 	struct list_elem *e;
// 	int i = 0;
// 	for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
// 			e = list_next (e)) 
// 	{
// 		inode = list_entry (e, struct inode, elem);
// 		// printf("Inode #%i: sector: %u\n", i, inode->sector);
// 		i++;
// 	}
// 	// printf("*****DONE PRINTING OPEN INODES*****\n");
// }

/* Reopens and returns INODE. */
	struct inode *
inode_reopen (struct inode *inode)
{
	if (inode != NULL)
		inode->open_cnt++;
	return inode;
}

/* Returns INODE's inode number. */
	block_sector_t
inode_get_inumber (const struct inode *inode)
{
	return inode->sector;
}

/* Closes INODE and writes it to disk. (Does it?  Check code.)
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
	void
inode_close (struct inode *inode) 
{
	/* Ignore null pointer. */
	if (inode == NULL)
		return;

	// printf("<<<<<CLOSING INODE %u>>>>>\n", inode->sector);
	/* Release resources if this was the last opener. */
	if (--inode->open_cnt == 0)
	{
		/* Remove from inode list and release lock. */
		// printf("<<<<<REMOVING FROM INODE LIST>>>>>\n");
		list_remove (&inode->elem);

		/* Deallocate blocks if removed. */
		if (inode->removed) 
		{
			// printf("8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888\n");
			free_map_release (inode->sector, 1);
			// free_map_release (inode->data.start,
			//                   bytes_to_sectors (inode->data.length));
			free_map_indexed_release(inode->data.direct_blocks, inode->data.first_level, 
					inode->data.second_level, bytes_to_sectors (inode->data.length)); 
		}

		free (inode); 
	}
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
	void
inode_remove (struct inode *inode) 
{
	ASSERT (inode != NULL);
	inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
	off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
	uint8_t *buffer = buffer_;
	off_t bytes_read = 0;
	uint8_t *bounce = NULL;

	// printf("\n****SECTOR %u READ AT****:\n\tsize: %i, offset %i, inode length: %i\n", inode->sector, size, offset, inode->data.length);

	while (size > 0) 
	{
		/* Disk sector to read, starting byte offset within sector. */
		block_sector_t sector_idx = byte_to_sector (inode, offset, false);
		// printf("\tsector_idx: %i\n", sector_idx);
		int sector_ofs = offset % BLOCK_SECTOR_SIZE;

		 // Bytes left in inode, bytes left in sector, lesser of the two. 
		off_t inode_left = inode_length (inode) - offset;
		int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
		int min_left = inode_left < sector_left ? inode_left : sector_left;

		/* Number of bytes to actually copy out of this sector. */
		int chunk_size = size < min_left ? size : min_left;
		// printf("\tchunk_size: %i\n", chunk_size);
		if (chunk_size <= 0)
			break;

		if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
		{
			/* Read full sector directly into caller's buffer. */
			block_read (fs_device, sector_idx, buffer + bytes_read);
		}
		else 
		{
			/* Read sector into bounce buffer, then partially copy
			   into caller's buffer. */
			if (bounce == NULL) 
			{
				bounce = malloc (BLOCK_SECTOR_SIZE);
				if (bounce == NULL)
					break;
			}
			block_read (fs_device, sector_idx, bounce);
			memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
		}

		/* Advance. */
		size -= chunk_size;
		offset += chunk_size;
		bytes_read += chunk_size;
	}
	free (bounce);
	// printf("****END READ AT****\n\n");
	return bytes_read;
}

off_t
inode_get_pos(struct inode *inode){
	return inode->pos;
}

void
inode_increment_pos(struct inode *inode, off_t offset)
{
	inode->pos += offset;
	block_write(fs_device, inode->sector, inode);
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
	off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
		off_t offset) 
{

	const uint8_t *buffer = buffer_;
	off_t bytes_written = 0;
	uint8_t *bounce = NULL;

	// printf("\n****SECTOR %u WRITE AT****:\n\tsize: %i, offset %i, inode length: %i\n", inode->sector, size, offset, inode->data.length);

	if (inode->deny_write_cnt)
	{
		// printf("Deny write count was true\n");
		return 0;
	}

	while (size > 0) 
	{
		/* Sector to write, starting byte offset within sector. */
		block_sector_t sector_idx = byte_to_sector (inode, offset, true);
		// printf("\tsector returned from byte_to_sector: %d\n", sector_idx);
		// printf("\toffset: %u\n\tsize: %u\n", offset, size);

		int sector_ofs = offset % BLOCK_SECTOR_SIZE;

		/* Bytes left in inode, bytes left in sector, lesser of the two. */
		off_t inode_left = inode_length (inode) - offset;
		// printf("\tinode left: %d\n", inode_left);
		int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
		int min_left = inode_left < sector_left ? inode_left : sector_left;

		/* Number of bytes to actually write into this sector. */
		// int chunk_size = size < min_left ? size : min_left;
		int chunk_size = size < sector_left ? size : sector_left;
		// printf("\tchunk_size: %i\n", chunk_size);


		if (chunk_size <= 0)
			break;

		if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
		{
			/* Write full sector directly to disk. */
			block_write (fs_device, sector_idx, buffer + bytes_written);
		}
		else 
		{
			/* We need a bounce buffer. */
			if (bounce == NULL) 
			{
				bounce = malloc (BLOCK_SECTOR_SIZE);
				if (bounce == NULL)
					break;
			}

			/* If the sector contains data before or after the chunk
			   we're writing, then we need to read in the sector
			   first.  Otherwise we start with a sector of all zeros. */
			if (sector_ofs > 0 || chunk_size < sector_left) 
				block_read (fs_device, sector_idx, bounce);
			else
				memset (bounce, 0, BLOCK_SECTOR_SIZE);

			memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
			block_write (fs_device, sector_idx, bounce);
		}
		/* Advance. */
		size -= chunk_size;
		offset += chunk_size;
		bytes_written += chunk_size;

		// printf("\tBEFORE inode length: %d\n", inode->data.length);
		if((offset) > inode->data.length)
		{
			// printf("\t\tCHANGED LENGTH\n");
			inode->data.length += chunk_size;
			block_write(fs_device, inode->sector, (const void*)&inode->data);
		}
		// printf("\tAFTER inode length: %d\n", inode->data.length);
		// printf("\tbytes written: %d\n\n\n", bytes_written);
	}
	//if there was growth, update the file size
	free (bounce);
	
	// printf("****END SECTOR %u WRITE AT****\n\n", inode->sector);
	return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
	void
inode_deny_write (struct inode *inode) 
{
	inode->deny_write_cnt++;
	// printf("DW: Deny_write_cnt: %i\n", inode->deny_write_cnt);
	ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
	void
inode_allow_write (struct inode *inode) 
{
	ASSERT (inode->deny_write_cnt > 0);
	ASSERT (inode->deny_write_cnt <= inode->open_cnt);
	inode->deny_write_cnt--;
	// printf("AW: Deny_write_cnt: %i\n", inode->deny_write_cnt);

}

/* Returns the length, in bytes, of INODE's data. */
	off_t
inode_length (const struct inode *inode)
{
	return inode->data.length;
}
