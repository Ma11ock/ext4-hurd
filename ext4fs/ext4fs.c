/* Main entry point for the ext4 file system translator.
 * 
 * Ryan Jeffrey
 */

#include <stdarg.h>
#include <stdio.h>
#include <device/device.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <argz.h>
#include <argp.h>
#include <hurd/store.h>
#include <version.h>
#include "ext4fs.h"

/*****************************************************************************/

int diskfs_link_max = EXT4_LINK_MAX;
int diskfs_name_max = EXT4_NAME_LEN;
int diskfs_maxsymlinks = 8;
int diskfs_shortcut_symlink = 1;
int diskfs_shortcut_chrdev = 1;
int diskfs_shortcut_blkdev = 1;
int diskfs_shortcut_fifo = 1;
int diskfs_shortcut_ifsock = 1;


const char *diskfs_server_name = "ext4fs";
const char *diskfs_server_version = HURD_VERSION;
const char *diskfs_extra_version = "GNU Hurd; ext4 " EXT4FS_VERSION;

struct store *store;
struct store_parsed *store_parsed;



int main(int argc, char *argv[])
{
  error_t err;
  
  
  return 0;
}
