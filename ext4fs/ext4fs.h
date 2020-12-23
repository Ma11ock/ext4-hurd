/* Common definitions for the ext4 filesystem translator

   Copyright (C) 1995, 1996, 1999, 2002, 2004 Free Software Foundation, Inc.
   Written by Miles Bader <miles@gnu.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifndef _EXT4FS_H
#define _EXT4FS_H

#include <mach.h>
#include <hurd.h>
#include <hurd/ports.h>
#include <hurd/pager.h>
#include <hurd/fshelp.h>
#include <hurd/iohelp.h>
#include <hurd/store.h>
#include <hurd/diskfs.h>
#include <hurd/ihash.h>
#include <assert-backtrace.h>
#include <pthread.h>
#include <sys/mman.h>
#include <endian.h>

#define __hurd__		/* Enable some hurd-specific fields.  */

/* Types used by the ext4 header files.  */
typedef u_int32_t __u32;
typedef int32_t   __s32;
typedef u_int16_t __u16;
typedef int16_t   __s16;
typedef u_int8_t  __u8;
typedef int8_t    __s8;

#include "ext4_fs.h"
#include "ext4_fs_i.h"

#define i_mode_high	osd2.hurd2.h_i_mode_high /* missing from ext4_fs.h */


/* If ext4_fs.h defined a debug routine, undef it and use our own.  */
#undef ext4_debug

#ifdef EXT4FS_DEBUG
#include <stdio.h>
extern int ext4_debug_flag;
#define ext4_debug_(f, a...) \
 fprintf (stderr, "ext4fs: (debug) %s: " f "\n", __FUNCTION__ , ## a)
#define ext4_debug(f, a...) \
 do { if (ext4_debug_flag) ext4_debug_(f, ## a); } while (0)
#else
#define ext4_debug(f, a...)	(void)0
#endif

#undef __hurd__

/* Define this if memory objects should not be cached by the kernel.
   Normally, don't define it, but defining it causes a much greater rate
   of paging requests, which may be helpful in catching bugs. */

#undef DONT_CACHE_MEMORY_OBJECTS

/* A block number.  */
typedef __u32 block_t;

/* ---------------------------------------------------------------- */

struct poke
{
  vm_offset_t offset;
  vm_size_t length;
  struct poke *next;
};

struct pokel
{
  struct poke *pokes, *free_pokes;
  pthread_spinlock_t lock;
  struct pager *pager;
  void *image;
};

void pokel_init (struct pokel *pokel, struct pager *pager, void *image);
/* Clean up any state associated with POKEL (but don't free POKEL).  */
void pokel_finalize (struct pokel *pokel);

/* Remember that data here on the disk has been modified. */
void pokel_add (struct pokel *pokel, void *loc, vm_size_t length);

/* Sync all the modified pieces of disk */
void pokel_sync (struct pokel *pokel, int wait);

/* Flush (that is, drop on the ground) all pending pokes in POKEL.  */
void pokel_flush (struct pokel *pokel);

/* Transfer all regions from FROM to POKEL, which must have the same pager. */
void pokel_inherit (struct pokel *pokel, struct pokel *from);

#include <features.h>
#ifdef EXT4FS_DEFINE_EI
#define EXT4FS_EI
#else
#define EXT4FS_EI __extern_inline
#endif

/* ---------------------------------------------------------------- */
/* Bitmap routines.  */

#include <stdint.h>

/* Forward declarations for the following functions that are usually
   inlined.  In case inlining is disabled, or inlining is not
   applicable, or a reference is taken to one of these functions, an
   implementation is provided in 'xinl.c'.  */
extern int test_bit (unsigned num, unsigned char *bitmap);
extern int set_bit (unsigned num, unsigned char *bitmap);
extern int clear_bit (unsigned num, unsigned char *bitmap);

#if defined(__USE_EXTERN_INLINES) || defined(EXT4FS_DEFINE_EI)
/* Returns TRUE if bit NUM is set in BITMAP.  */
EXT4FS_EI int
test_bit (unsigned num, unsigned char *bitmap)
{
  const uint32_t *const bw = (uint32_t *) bitmap + (num >> 5);
  const uint_fast32_t mask = 1 << (num & 31);
  return *bw & mask;
}

/* Sets bit NUM in BITMAP, and returns the previous state of the bit.  Unlike
   the linux version, this function is NOT atomic!  */
EXT4FS_EI int
set_bit (unsigned num, unsigned char *bitmap)
{
  uint32_t *const bw = (uint32_t *) bitmap + (num >> 5);
  const uint_fast32_t mask = 1 << (num & 31);
  return (*bw & mask) ?: (*bw |= mask, 0);
}

/* Clears bit NUM in BITMAP, and returns the previous state of the bit.
   Unlike the linux version, this function is NOT atomic!  */
EXT4FS_EI int
clear_bit (unsigned num, unsigned char *bitmap)
{
  uint32_t *const bw = (uint32_t *) bitmap + (num >> 5);
  const uint_fast32_t mask = 1 << (num & 31);
  return (*bw & mask) ? (*bw &= ~mask, mask) : 0;
}
#endif /* Use extern inlines.  */

/* ---------------------------------------------------------------- */

/* ext4fs specific per-file data.  */
struct disknode
{
  /* For a directory, this array holds the number of directory entries in
     each DIRBLKSIZE piece of the directory. */
  int *dirents;

  /* Lock to lock while fiddling with this inode's block allocation info.  */
  pthread_rwlock_t alloc_lock;

  /* Where changes to our indirect blocks are added.  */
  struct pokel indir_pokel;

  /* Random extra info used by the ext4 routines.  */
  struct ext4_inode_info info;
  uint32_t info_i_translator;	/* That struct from Linux source lacks this. */

  /* This file's pager.  */
  struct pager *pager;

  /* True if the last page of the file has been made writable, but is only
     partially allocated.  */
  int last_page_partially_writable;

  /* Index to start a directory lookup at.  */
  int dir_idx;
};

struct user_pager_info
{
  enum pager_type
    {
      DISK,
      FILE_DATA,
    } type;
  struct node *node;
  vm_prot_t max_prot;
};

/* ---------------------------------------------------------------- */
/* pager.c */

#define DISK_CACHE_BLOCKS	65536

#include <hurd/diskfs-pager.h>

/* Set up the disk pager.  */
void create_disk_pager (void);

/* Inhibit the disk pager.  */
error_t inhibit_ext4_pager (void);

/* Resume the disk pager.  */
void resume_ext4_pager (void);

/* Call this when we should turn off caching so that unused memory object
   ports get freed.  */
void drop_pager_softrefs (struct node *node);

/* Call this when we should turn on caching because it's no longer
   important for unused memory object ports to get freed.  */
void allow_pager_softrefs (struct node *node);

/* Invalidate any pager data associated with NODE.  */
void flush_node_pager (struct node *node);

/* ---------------------------------------------------------------- */

/* The physical media.  */
extern struct store *store;
/* What the user specified.  */
extern struct store_parsed *store_parsed;

/* Mapped image of cached blocks of the disk.  */
extern void *disk_cache;
extern store_offset_t disk_cache_size;
extern int disk_cache_blocks;

#define DC_INCORE	0x01	/* Not in core.  */
#define DC_UNTOUCHED	0x02	/* Not touched by disk_pager_read_paged
				   or disk_cache_block_ref.  */
#define DC_FIXED	0x04	/* Must not be re-associated.  */

/* Flags that forbid re-association of page.  DC_UNTOUCHED is included
   because this flag is used only when page is already to be
   re-associated, so it's not good candidate for another
   remapping.  */
#define DC_DONT_REUSE	(DC_INCORE | DC_UNTOUCHED | DC_FIXED)

#define DC_NO_BLOCK	((block_t) -1L)

#ifdef DEBUG_DISK_CACHE
#define DISK_CACHE_LAST_READ_XOR	0xDEADBEEF
#endif

/* Disk cache blocks' meta info.  */
struct disk_cache_info
{
  block_t block;
  uint16_t flags;
  uint16_t ref_count;
  struct disk_cache_info *next;	/* List of reusable entries.  */
#ifdef DEBUG_DISK_CACHE
  block_t last_read, last_read_xor;
#endif
};

/* block num --> pointer to in-memory block */
extern hurd_ihash_t disk_cache_bptr;
/* Metadata about cached block. */
extern struct disk_cache_info *disk_cache_info;
/* Lock for these mappings */
extern pthread_mutex_t disk_cache_lock;
/* Fired when a re-association is done.  */
extern pthread_cond_t disk_cache_reassociation;

void *disk_cache_block_ref (block_t block);
void disk_cache_block_ref_ptr (void *ptr);
void _disk_cache_block_deref (void *ptr);
#define disk_cache_block_deref(PTR)                             \
  do { _disk_cache_block_deref (PTR); PTR = NULL; } while (0)
int disk_cache_block_is_ref (block_t block);

/* Our in-core copy of the super-block (pointer into the disk_cache).  */
extern struct ext4_super_block *sblock;
/* True if sblock has been modified.  */
extern int sblock_dirty;

/* Where the super-block is located on disk (at min-block 1).  */
#define SBLOCK_BLOCK	1	/* Default location, second 1k block.  */
#define SBLOCK_SIZE	(sizeof (struct ext4_super_block))
extern unsigned int sblock_block; /* Specified location (in 1k blocks).  */
#define SBLOCK_OFFS	(sblock_block << 10) /* Byte offset of superblock.  */

/* The filesystem block-size.  */
extern unsigned int block_size;
/* The log base 2 of BLOCK_SIZE.  */
extern unsigned int log2_block_size;

/* The number of bits to scale min-blocks to get filesystem blocks.  */
#define BLOCKSIZE_SCALE	(sblock->s_log_block_size)

/* log2 of the number of device blocks in a filesystem block.  */
extern unsigned log2_dev_blocks_per_fs_block;

/* log2 of the number of stat blocks (512 bytes) in a filesystem block.  */
extern unsigned log2_stat_blocks_per_fs_block;

/* A handy page of page-aligned zeros.  */
extern vm_address_t zeroblock;

/* Get the superblock from the disk, point `sblock' to it, and setup
   various global info from it.  */
void get_hypermetadata ();

/* Map `group_desc_image' pointers to disk cache.  Also, establish a
   non-exported mapping to the superblock that will be used by
   diskfs_set_hypermetadata to update the superblock from the cache
   `sblock' points to.  */
void map_hypermetadata ();

/* ---------------------------------------------------------------- */
/* Random stuff calculated from the super block.  */

extern unsigned long frag_size;	/* Size of a fragment in bytes */
extern unsigned long frags_per_block;	/* Number of fragments per block */
extern unsigned long inodes_per_block;	/* Number of inodes per block */

extern unsigned long itb_per_group;	/* Number of inode table blocks per group */
extern unsigned long db_per_group;	/* Number of descriptor blocks per group */
extern unsigned long desc_per_block;	/* Number of group descriptors per block */
extern unsigned long addr_per_block;	/* Number of disk addresses per block */

extern unsigned long groups_count;	/* Number of groups in the fs */

/* ---------------------------------------------------------------- */

extern pthread_spinlock_t node_to_page_lock;

extern pthread_spinlock_t generation_lock;
extern unsigned long next_generation;

/* ---------------------------------------------------------------- */
/* Functions for looking inside disk_cache */

#define trunc_block(offs) \
  ((off_t) ((offs) >> log2_block_size) << log2_block_size)
#define round_block(offs) \
  ((off_t) (((offs) + block_size - 1) >> log2_block_size) << log2_block_size)

/* block num --> byte offset on disk */
#define boffs(block) ((off_t) (block) << log2_block_size)
/* byte offset on disk --> block num */
#define boffs_block(offs) ((offs) >> log2_block_size)

/* pointer to in-memory block -> index in disk_cache_info */
#define bptr_index(ptr) (((char *)ptr - (char *)disk_cache) >> log2_block_size)

/* Forward declarations for the following functions that are usually
   inlined.  In case inlining is disabled, or inlining is not
   applicable, or a reference is taken to one of these functions, an
   implementation is provided in 'xinl.c'.  */
extern char *boffs_ptr (off_t offset);
extern off_t bptr_offs (void *ptr);

#if defined(__USE_EXTERN_INLINES) || defined(EXT4FS_DEFINE_EI)

/* byte offset on disk --> pointer to in-memory block */
EXT4FS_EI char *
boffs_ptr (off_t offset)
{
  block_t block = boffs_block (offset);
  pthread_mutex_lock (&disk_cache_lock);
  char *ptr = hurd_ihash_find (disk_cache_bptr, block);
  pthread_mutex_unlock (&disk_cache_lock);
  assert_backtrace (ptr);
  ptr += offset % block_size;
  ext4_debug ("(%lld) = %p", offset, ptr);
  return ptr;
}

/* pointer to in-memory block --> byte offset on disk */
EXT4FS_EI off_t
bptr_offs (void *ptr)
{
  vm_offset_t mem_offset = (char *)ptr - (char *)disk_cache;
  off_t offset;
  assert_backtrace (mem_offset < disk_cache_size);
  pthread_mutex_lock (&disk_cache_lock);
  offset = (off_t) disk_cache_info[boffs_block (mem_offset)].block
    << log2_block_size;
  assert_backtrace (offset || mem_offset < block_size);
  offset += mem_offset % block_size;
  pthread_mutex_unlock (&disk_cache_lock);
  ext4_debug ("(%p) = %lld", ptr, offset);
  return offset;
}

#endif /* Use extern inlines.  */

/* block num --> pointer to in-memory block */
#define bptr(block) boffs_ptr(boffs(block))
/* pointer to in-memory block --> block num */
#define bptr_block(ptr) boffs_block(bptr_offs(ptr))

/* Get the descriptor for block group NUM.  The block group descriptors are
   stored starting in the filesystem block following the super block.
   We cache a pointer into the disk image for easy lookup.  */
#define group_desc(num)	(&group_desc_image[num])
extern struct ext4_group_desc *group_desc_image;

#define inode_group_num(inum) (((inum) - 1) / sblock->s_inodes_per_group)

/* Forward declarations for the following functions that are usually
   inlined.  In case inlining is disabled, or inlining is not
   applicable, or a reference is taken to one of these functions, an
   implementation is provided in 'xinl.c'.  */
extern struct ext4_inode * dino_ref (ino_t inum);
extern void _dino_deref (struct ext4_inode *inode);

#if defined(__USE_EXTERN_INLINES) || defined(EXT4FS_DEFINE_EI)
/* Convert an inode number to the dinode on disk. */
EXT4FS_EI struct ext4_inode *
dino_ref (ino_t inum)
{
  unsigned long inodes_per_group = sblock->s_inodes_per_group;
  unsigned long bg_num = (inum - 1) / inodes_per_group;
  unsigned long group_inum = (inum - 1) % inodes_per_group;
  struct ext4_group_desc *bg = group_desc (bg_num);
  block_t block = bg->bg_inode_table + (group_inum / inodes_per_block);
  struct ext4_inode *inode = disk_cache_block_ref (block);
  inode += group_inum % inodes_per_block;
  ext4_debug ("(%llu) = %p", inum, inode);
  return inode;
}

EXT4FS_EI void
_dino_deref (struct ext4_inode *inode)
{
  ext4_debug ("(%p)", inode);
  disk_cache_block_deref (inode);
}
#endif /* Use extern inlines.  */
#define dino_deref(INODE)                               \
  do { _dino_deref (INODE); INODE = NULL; } while (0)

/* ---------------------------------------------------------------- */
/* inode.c */

/* Write all active disknodes into the inode pager. */
void write_all_disknodes ();

/* ---------------------------------------------------------------- */

/* What to lock if changing global data data (e.g., the superblock or block
   group descriptors or bitmaps).  */
extern pthread_spinlock_t global_lock;

/* Where to record such changes.  */
extern struct pokel global_pokel;

/* If the block size is less than the page size, then this bitmap is used to
   record which disk blocks are actually modified, so we don't stomp on parts
   of the disk which are backed by file pagers.  */
extern unsigned char *modified_global_blocks;
extern pthread_spinlock_t modified_global_blocks_lock;

/* Forward declarations for the following functions that are usually
   inlined.  In case inlining is disabled, or inlining is not
   applicable, or a reference is taken to one of these functions, an
   implementation is provided in 'xinl.c'.  */
extern int global_block_modified (block_t block);
extern void record_global_poke (void *ptr);
extern void sync_global_ptr (void *bptr, int wait);
extern void record_indir_poke (struct node *node, void *ptr);
extern void sync_global (int wait);
extern void alloc_sync (struct node *np);

#if defined(__USE_EXTERN_INLINES) || defined(EXT4FS_DEFINE_EI)
/* Marks the global block BLOCK as being modified, and returns true if we
   think it may have been clean before (but we may not be sure).  Note that
   this isn't enough to cause the block to be synced; you must call
   record_global_poke to do that.  */
EXT4FS_EI int
global_block_modified (block_t block)
{
  if (modified_global_blocks)
    {
      int was_clean;
      pthread_spin_lock (&modified_global_blocks_lock);
      was_clean = !set_bit(block, modified_global_blocks);
      pthread_spin_unlock (&modified_global_blocks_lock);
      return was_clean;
    }
  else
    return 1;
}

/* This records a modification to a non-file block.  */
EXT4FS_EI void
record_global_poke (void *ptr)
{
  block_t block = boffs_block (bptr_offs (ptr));
  void *block_ptr = bptr (block);
  ext4_debug ("(%p = %p)", ptr, block_ptr);
  assert_backtrace (disk_cache_block_is_ref (block));
  global_block_modified (block);
  pokel_add (&global_pokel, block_ptr, block_size);
}

/* This syncs a modification to a non-file block.  */
EXT4FS_EI void
sync_global_ptr (void *ptr, int wait)
{
  block_t block = boffs_block (bptr_offs (ptr));
  void *block_ptr = bptr (block);
  ext4_debug ("(%p -> %u)", ptr, block);
  global_block_modified (block);
  disk_cache_block_deref (block_ptr);
  pager_sync_some (diskfs_disk_pager,
		   block_ptr - disk_cache, block_size, wait);

}

/* This records a modification to one of a file's indirect blocks.  */
EXT4FS_EI void
record_indir_poke (struct node *node, void *ptr)
{
  block_t block = boffs_block (bptr_offs (ptr));
  void *block_ptr = bptr (block);
  ext4_debug ("(%llu, %p)", node->cache_id, ptr);
  assert_backtrace (disk_cache_block_is_ref (block));
  global_block_modified (block);
  pokel_add (&diskfs_node_disknode (node)->indir_pokel, block_ptr, block_size);
}

/* ---------------------------------------------------------------- */

EXT4FS_EI void
sync_global (int wait)
{
  ext4_debug ("%d", wait);
  pokel_sync (&global_pokel, wait);
}

/* Sync all allocation information and node NP if diskfs_synchronous. */
EXT4FS_EI void
alloc_sync (struct node *np)
{
  if (diskfs_synchronous)
    {
      if (np)
	{
	  diskfs_node_update (np, 1);
	  pokel_sync (&diskfs_node_disknode (np)->indir_pokel, 1);
	}
      diskfs_set_hypermetadata (1, 0);
    }
}
#endif /* Use extern inlines.  */

/* ---------------------------------------------------------------- */
/* getblk.c */

void ext4_discard_prealloc (struct node *node);

/* Returns in DISK_BLOCK the disk block corresponding to BLOCK in NODE.
   If there is no such block yet, but CREATE is true, then it is created,
   otherwise EINVAL is returned.  */
error_t ext4_getblk (struct node *node, block_t block, int create, block_t *disk_block);

block_t ext4_new_block (block_t goal,
			block_t prealloc_goal,
			block_t *prealloc_count, block_t *prealloc_block);

void ext4_free_blocks (block_t block, unsigned long count);

/* ---------------------------------------------------------------- */

/* Write disk block ADDR with DATA of LEN bytes, waiting for completion.  */
error_t dev_write_sync (block_t addr, vm_address_t data, long len);

/* Write diskblock ADDR with DATA of LEN bytes; don't bother waiting
   for completion. */
error_t dev_write (block_t addr, vm_address_t data, long len);

/* Read disk block ADDR; put the address of the data in DATA; read LEN
   bytes.  Always *DATA should be a full page no matter what.   */
error_t dev_read_sync (block_t addr, vm_address_t *data, long len);

/* ---------------------------------------------------------------- */

#define ext4_error(fmt, args...) _ext4_error (__FUNCTION__, fmt , ##args)
extern void _ext4_error (const char *, const char *, ...)
     __attribute__ ((format (printf, 2, 3)));

#define ext4_panic(fmt, args...) _ext4_panic (__FUNCTION__, fmt , ##args)
extern void _ext4_panic (const char *, const char *, ...)
     __attribute__ ((format (printf, 2, 3)));

extern void ext4_warning (const char *, ...)
     __attribute__ ((format (printf, 1, 2)));

/* ---------------------------------------------------------------- */
/* xattr.c */

error_t ext4_list_xattr (struct node *np, char *buffer, size_t *len);
error_t ext4_get_xattr (struct node *np, const char *name, char *value, size_t *len);
error_t ext4_set_xattr (struct node *np, const char *name, const char *value, size_t len, int flags);
error_t ext4_free_xattr_block (struct node *np);

/* Use extended attribute-based translator records.
 *
 * This flag allows users to opt-in to the use of extended attributes
 * for storing translator records.  We will make this the default once
 * we feel confident that the implementation is fine.
 *
 * XXX: Remove this in Hurd 1.0 (or 0.10, or whatever follows 0.9).
 */
extern int use_xattr_translator_records;

/* ---------------------------------------------------------------- */
/* Helper Functions. */

/* Adds a little-endian 32 bit integer VAR to whatever HOSTVAL is on the host machine. */
static inline void le32addh(__le32 *var, u32 hostval)
{
  *var = htole32(le32toh(*var) + hostval);
}

/* Adds a little-endian 16 bit integer VAR to whatever HOSTVAL is on the host machine. */
static inline void le16addh(__le16 *var, u16 hostval)
{
  *var = htole16(le16toh(*var) + hostval);
}

#endif
