# Makefile for ext4fs
#
#   Copyright (C) 1994,95,96,99,2000,02,12 Free Software Foundation, Inc.
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License as
#   published by the Free Software Foundation; either version 2, or (at
#   your option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

dir := ext4fs
makemode := server
target = ext4fs
SRCS = acl.c  balloc.c bitmap.c block_validity.c dir.c \
	ext4fs.c ext4_jbd2.c extents.c extents_status.c fast_commit.c file.c \
	fsmap.c fsync.c hash.c ialloc.c indirect.c inline.c inode.c \
	inode-test.c ioctl.c mballoc.c migrate.c mmp.c \
	move_extent.c namei.c page-io.c readpage.c resize.c super.c symlink.c \
	sysfs.c  verity.c xattr.c xattr_hurd.c xattr_security.c \
	xattr_trusted.c xattr_user.c

OBJS = $(SRCS:.c=.o)
HURDLIBS = diskfs pager iohelp fshelp store ports ihash shouldbeinlibc
LDLIBS = -lpthread $(and $(HAVE_LIBBZ2),-lbz2) $(and $(HAVE_LIBZ),-lz)

include ../Makeconf

ext4fs.static: $(boot-store-types:%=../libstore/libstore_%.a)
