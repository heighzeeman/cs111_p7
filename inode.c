#include <stdio.h>
#include <assert.h>

#include "inode.h"
#include "diskimg.h"
#include <stdbool.h>
#include <string.h>

#define INODES_PER_BLOCK (DISKIMG_SECTOR_SIZE/sizeof(struct inode))
#define NUM_BLOCK_NUMS_PER_BLOCK (DISKIMG_SECTOR_SIZE/sizeof(uint16_t))
#define NUM_IADDR ((int)(sizeof(inp->i_addr)/sizeof(uint16_t)))	 // Number of addresses in inode i_addr array

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
	int fd = fs->dfd;
	int num_iblocks = fs->superblock.s_isize;
	int inode_block = (inumber-ROOT_INUMBER) / INODES_PER_BLOCK;
	
	if (inode_block < 0 || inode_block >= num_iblocks) {
		fprintf(stderr, "Invalid inode_block: %d specified\n", inode_block);
		return -1;
	}
	
	struct inode buf[INODES_PER_BLOCK];
	int read;	// Useful variable to keep track of bytes read during disk reads
	if ((read = diskimg_readsector(fd, INODE_START_SECTOR+inode_block, buf)) != DISKIMG_SECTOR_SIZE) {
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
	int read;	// Useful variable to keep track of bytes read during disk reads
	
    if (inp->i_mode & ILARG) {
		int i_addr_idx = blockNum / NUM_BLOCK_NUMS_PER_BLOCK;
		
		// Using the doubly-indirect block table
		if (i_addr_idx >= (NUM_IADDR-1)) {
			blockNum -= NUM_BLOCK_NUMS_PER_BLOCK*(NUM_IADDR-1);
			int first_idx = blockNum / NUM_BLOCK_NUMS_PER_BLOCK;  // Index into first indirect block table

			// First case: blockNum is too small to warrant using the double indirection.
			// Second case: file is too big to be able to fit into an inode.
			// Both cases: the inode record must be corrupted.
			if (first_idx < 0 || first_idx >= (int)NUM_BLOCK_NUMS_PER_BLOCK) {
				fprintf(stderr, "Invalid first index: %d\n", first_idx);
				return -1;
			}
			
			uint16_t buf[NUM_BLOCK_NUMS_PER_BLOCK];
			// First read the first indirect block table
			if ((read = diskimg_readsector(fd, inp->i_addr[(NUM_IADDR-1)], buf)) != DISKIMG_SECTOR_SIZE) {
				fprintf(stderr, "Invalid read of sector: %d. Only read %d bytes.\n", inp->i_addr[i_addr_idx], read);
				return -1;
			}
			// Next read the direct blocks from the second block table
			if ((read = diskimg_readsector(fd, buf[first_idx], buf)) != DISKIMG_SECTOR_SIZE) {
				fprintf(stderr, "Invalid read of sector: %d. Only read %d bytes.\n", inp->i_addr[i_addr_idx], read);
				return -1;
			}
			
			return buf[blockNum % NUM_BLOCK_NUMS_PER_BLOCK];
		}
		
		// Not using indirect block table, but using extended size protocol
		uint16_t buf[NUM_BLOCK_NUMS_PER_BLOCK];
		if ((read = diskimg_readsector(fd, inp->i_addr[i_addr_idx], buf)) != DISKIMG_SECTOR_SIZE) {
			fprintf(stderr, "Invalid read of sector: %d. Only read %d bytes.\n", inp->i_addr[i_addr_idx], read);
			return -1;
		}
		return buf[blockNum % NUM_BLOCK_NUMS_PER_BLOCK];
		
	} else {
		// Using all direct block tables
		if (blockNum <= (NUM_IADDR-1)) return inp->i_addr[blockNum];
		fprintf(stderr, "Small inode addressing but block num is: %d\n", blockNum);
	}
	
    return -1;
}

int inode_getsize(struct inode *inp) {
    return ((inp->i_size0 << 16) | inp->i_size1);
}
