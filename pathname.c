
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>


static char *strsplice(const char *str, int length) {
	char *result = malloc(length + 1);
	if (!result) {
		fprintf(stderr, "malloc failed on size: %d\n", length + 1);
		return NULL;
	}
	result[length] = '\0';
	return memcpy(result, str, length);
}

static int tokenize(const char *line, char *tokens[], int maxTokens) {
	int numTokens = 0;
	int counter = 0;
	while (numTokens < maxTokens) {
		while (line[counter] && line[counter] != '/') counter++;
		if (line[counter]) {
			int start = counter;
			int end = start+1;
			while (line[end] && line[end] != '/') end++;
			char *toAdd = strsplice(line + start, end - start);
			if (!toAdd) {
				for (int i = 0; i < numTokens; ++i) free(tokens[i]);
				return -1;
			}
			tokens[numTokens++] = toAdd;
			counter = end;
		} else break;
	}
	
	return numTokens;
}

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    int numslashes = 1;
	for (const char *c = pathname; *c; c++)
		if (*c == '/') ++numslashes;
	
	char *tokens[numslashes];
	int dirinumber = ROOT_INUMBER;
	int numTokens = tokenize(pathname, tokens, numslashes);
	
	if (numTokens == -1) {
		fprintf(stderr, "tokenization of input path failed\n");
		return -1;
	}
	
	for (int i = 0; i < numTokens; ++i) {
		struct direntv6 dirEnt;
		if (!tokens[i][1]) break;
		if (directory_findname(fs, tokens[i]+1, dirinumber, &dirEnt)) {
			fprintf(stderr, "Unable to find %s in entries for inode %d\n", tokens[i]+1, dirinumber);
			return -1;
		}
		
		dirinumber = dirEnt.d_inumber;
	}
	
	for (int i = 0; i < numTokens; ++i) free(tokens[i]);
	
    return dirinumber;
}
