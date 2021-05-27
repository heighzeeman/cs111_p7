
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

/**
 * Takes a substring by specifying the starting pointer and target length.
 * Caller function must deallocate after use. Returns pointer to substring
 * if successful, NULL otherwise.
 */
static char *strsplice(const char *str, int length) {
	char *result = malloc(length + 1);
	if (!result) {
		fprintf(stderr, "malloc failed on size: %d\n", length + 1);
		return NULL;
	}
	result[length] = '\0';
	return memcpy(result, str, length);
}

 /**
 * Given a string, split it into at most maxtokens tokens by delimiting with delim,
 * passing pointers to the tokens into a given array of strings. Tokens each begin
 * with the delim character. Caller function must deallocate tokens after use.
 * Returns number of tokens in string.
 */
static int tokenize(const char *line, char *tokens[], int maxtokens, char delim) {
	int numtokens = 0;
	int counter = 0;
	while (numtokens < maxtokens) {
		while (line[counter] && line[counter] != delim) counter++;
		if (line[counter]) {	// Equiv to line[counter] != '\0'
			int start = counter;
			int end = start+1;
			while (line[end] && line[end] != delim) end++;
			char *toAdd = strsplice(line + start, end - start);
			if (!toAdd) {	// Splicing failed, abort current progress
				for (int i = 0; i < numtokens; ++i) free(tokens[i]);
				return -1;
			}
			tokens[numtokens++] = toAdd;
			counter = end;
		} else break;
	}
	
	return numtokens;
}

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    int numslashes = 0;
	for (const char *c = pathname; *c; c++)
		if (*c == '/') ++numslashes;
	
	char *tokens[numslashes+1];	// Number of tokens is at most number of slashes + 1.
	int dirinumber = ROOT_INUMBER;
	int numtokens = tokenize(pathname, tokens, numslashes+1, '/');
	
	if (numtokens == -1) {
		fprintf(stderr, "tokenization of input path failed\n");
		return -1;
	}
	
	for (int i = 0; i < numtokens; ++i) {
		struct direntv6 dirEnt;
		if (!tokens[i][1]) break;	// True iff strcmp(token, "/") == 0. In which case no progress can be made.
		if (directory_findname(fs, tokens[i]+1, dirinumber, &dirEnt)) {	// tokens[i]+1 to skip the '/' each token begins with
			fprintf(stderr, "Unable to find %s in entries for inode %d\n", tokens[i]+1, dirinumber);
			return -1;
		}
		dirinumber = dirEnt.d_inumber;
	}
	
	for (int i = 0; i < numtokens; ++i) free(tokens[i]);
	
    return dirinumber;
}
