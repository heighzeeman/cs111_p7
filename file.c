#include <stdio.h>
#include <assert.h>

#include "file.h"
#include "inode.h"
#include "diskimg.h"

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    struct inode inp;
	if (inode_iget(fs, inumber, &inp)) {
		fprintf(stderr, "Couldn't fetch inode with number: %d\n", inumber);
		return -1;
	}
	int sector_from_inode = inode_indexlookup(fs, &inp, blockNum);
	if (sector_from_inode == -1) {
		fprintf(stderr, "Couldn't fetch sector number of inode: %d, block number: %d\n", inumber, blockNum);
		return -1;
	}
	
	int fd = fs->dfd;
	int read;
	if ((read = diskimg_readsector(fd, sector_from_inode, buf)) != DISKIMG_SECTOR_SIZE) {
		fprintf(stderr, "Invalid read of sector: %d. Only read %d bytes.\n", sector_from_inode, read);
		return -1;
	}
	
	int inode_size = inode_getsize(&inp);
	if (blockNum == inode_size/DISKIMG_SECTOR_SIZE) return inode_size%DISKIMG_SECTOR_SIZE; 
	
    return DISKIMG_SECTOR_SIZE;
}
