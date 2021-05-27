#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define NUM_DIRENTS_PER_BLOCK (DISKIMG_SECTOR_SIZE/sizeof(struct direntv6))
#define MAX_COMPONENT_LENGTH sizeof(dirEnt->d_name)

int directory_findname(struct unixfilesystem *fs, const char *name,
                       int dirinumber, struct direntv6 *dirEnt) {
    struct inode inp;
	if (inode_iget(fs, dirinumber, &inp)) {
		fprintf(stderr, "Couldn't fetch directory inode with number: %d\n", dirinumber);
		return -1;
	}
	
	if (!(inp.i_mode & IALLOC)) {
		fprintf(stderr, "Specified inode: %d has not been allocated.\n", dirinumber);
		return -1;
	}
	
	if ((inp.i_mode & IFMT) != IFDIR) {
		fprintf(stderr, "Specified inode: %d is not a directory inode.\n", dirinumber);
		return -1;
	}
	
	int max_blockNum = inode_getsize(&inp) / DISKIMG_SECTOR_SIZE;
	int fd = fs->dfd;
	int read;
	struct direntv6 buf[NUM_DIRENTS_PER_BLOCK];
	for (int blockNum = 0; blockNum <= max_blockNum; ++blockNum) {
		read = file_getblock(fs, dirinumber, blockNum, buf);
		if (read == -1) {
			fprintf(stderr, "File reading failed on directory inode: %d, block num: %d\n",\
					dirinumber, blockNum);
			return -1;
		}
		
		unsigned num_entries = read/sizeof(struct direntv6);
		for (unsigned i = 0; i < num_entries; ++i) {
			if (strncmp(buf[i].d_name, name, MAX_COMPONENT_LENGTH) == 0) {
				// Match on any directory name
				memcpy(dirEnt, buf + i, sizeof(struct direntv6));
				return 0;
			}
		}
	}
	
    return -1;
}
