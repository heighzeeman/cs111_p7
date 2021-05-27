#include <stdio.h>
#include <assert.h>

#include "inode.h"
#include "diskimg.h"
#include <stdbool.h>
#include <string.h>

#define INODES_PER_BLOCK (DISKIMG_SECTOR_SIZE/sizeof(struct inode))
#define NUM_BLOCK_NUMS_PER_BLOCK (DISKIMG_SECTOR_SIZE/sizeof(uint16_t))

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
	int fd = fs->dfd;
	int num_iblocks = fs->superblock.s_isize;
	int inode_block = (inumber-ROOT_INUMBER) / INODES_PER_BLOCK;
	if (inode_block < 0 || inode_block >= num_iblocks) {
		fprintf(stderr, "Invalid inode_block: %d specified\n", inode_block);
		return -1;
	}
	struct inode buf[INODES_PER_BLOCK];
	int read;
	if ((read = diskimg_readsector(fd, INODE_START_SECTOR + inode_block, buf)) != DISKIMG_SECTOR_SIZE) {
		fprintf(stderr, "Invalid read of inode block: %d. Only read %d bytes.\n", inode_block, read);
		return -1;
	}
	
	memcpy(inp, buf + (inumber-ROOT_INUMBER)%INODES_PER_BLOCK, sizeof(struct inode));
    return 0;
}

int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
	if (blockNum < 0) {
		fprintf(stderr, "Negative block num given: %d\n", blockNum);
		return -1;
	}
	
	int fd = fs->dfd;
	int read;
	
    if (inp->i_mode & ILARG) {
		int i_addr_idx = blockNum / NUM_BLOCK_NUMS_PER_BLOCK;
		if (i_addr_idx >= 7) {
			blockNum -= NUM_BLOCK_NUMS_PER_BLOCK*7;
			int first_idx = blockNum / NUM_BLOCK_NUMS_PER_BLOCK;
			if (first_idx < 0 || first_idx >= (int)NUM_BLOCK_NUMS_PER_BLOCK) {
				fprintf(stderr, "Invalid first index: %d\n", first_idx);
				return -1;
			}
			uint16_t buf[NUM_BLOCK_NUMS_PER_BLOCK];
		
			if ((read = diskimg_readsector(fd, inp->i_addr[7], buf)) != DISKIMG_SECTOR_SIZE) {
				fprintf(stderr, "Invalid read of sector: %d. Only read %d bytes.\n", inp->i_addr[i_addr_idx], read);
				return -1;
			}
			if ((read = diskimg_readsector(fd, buf[first_idx], buf)) != DISKIMG_SECTOR_SIZE) {
				fprintf(stderr, "Invalid read of sector: %d. Only read %d bytes.\n", inp->i_addr[i_addr_idx], read);
				return -1;
			}
			
			return buf[blockNum % NUM_BLOCK_NUMS_PER_BLOCK];
		}
		
		uint16_t buf[NUM_BLOCK_NUMS_PER_BLOCK];
		if ((read = diskimg_readsector(fd, inp->i_addr[i_addr_idx], buf)) != DISKIMG_SECTOR_SIZE) {
			fprintf(stderr, "Invalid read of sector: %d. Only read %d bytes.\n", inp->i_addr[i_addr_idx], read);
			return -1;
		}
		return buf[blockNum % NUM_BLOCK_NUMS_PER_BLOCK];
		
	} else {
		if (blockNum < 8) return inp->i_addr[blockNum];
		fprintf(stderr, "Small inode addressing but block num is: %d\n", blockNum);
	}
	
    return -1;
}

int inode_getsize(struct inode *inp) {
    return ((inp->i_size0 << 16) | inp->i_size1);
}
