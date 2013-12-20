#ifndef _readBedTrack_
#define _readBedTrack_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define MAX_TOKEN 100
#define MAXLINE 30000

/***************************************************
** cszeto Aug 2009
**
** Defines a DLL bedEntry structure, and how to
** extract the distances to and from each probe to their
** neighbours.
***************************************************/

struct bedEntry{
	int chr;
    int start;
    int end;
	int isValid;
	struct bedEntry *next;
	struct bedEntry *last;
};


struct bedEntry * init_bedEntry(int chr, int start, int end);
void pushBedEntry(struct bedEntry *dest, int chr, int start, int end);
void chrom_intToString(char *string, int chr);
int chrom_stringToInt(char *string);
struct bedEntry * readBedFile(FILE *fp);
void calculate_distances(struct bedEntry *b,int * distances, int number);

#endif
