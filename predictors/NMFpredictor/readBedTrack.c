#include "readBedTrack.h"

/***************************************************
** cszeto Aug 2009 
** 
** Defines a DLL bedEntry structure, and how to 
** extract the distances to and from each probe to their 
** neighbours.
***************************************************/

struct bedEntry * init_bedEntry(int chr, int start, int end){
	struct bedEntry * result = malloc(sizeof(struct bedEntry));
	result->chr = chr;
	result->start = start;
	result->end = end;
	result->next = NULL;
	result->last = NULL;
	return result;
}

void pushBedEntry(struct bedEntry * dest, int chr, int start, int end){
	struct bedEntry * new = init_bedEntry(chr, start, end); 
	struct bedEntry *ptr;
	ptr = dest;
	while(ptr->next)
		ptr=ptr->next;
	ptr->next = new;
	new->last = ptr;
}

int chrom_stringToInt(char *string)
/*Returns an array index for the chromosome string, i.e. number between 0-23*/
{
    int chr = 0;
    if(strcmp(string, "chrX") == 0)
        return 22;
    else if (strcmp(string, "chrY") == 0)
        return 23;
    else{
        char * ptr = strrchr(string, 'r');
        ptr++;
        chr = atoi(ptr);
        chr--;
        return chr;
    }
}

void chrom_intToString(char *string, int chr)
/*Converts an array index for achrom into a string, i.e. 0 = chr1, 1 = chr2 etc.*/
{
    int chrNum = chr;
    if(chrNum == 22)
        sprintf(string, "chrX");
    else if(chrNum == 23)
        sprintf(string, "chrY");
    else{
        chrNum++;
        sprintf(string, "chr%d", chrNum);
    }
}

void calculate_distances(struct bedEntry *b, int * distances, int number){
	if((number %2) != 1){
		fprintf(stderr, "ERROR: Can't fill an even numbered array with distances. Check your code\n ");	
		exit(1);
	}else if(!b){
		fprintf(stderr, "ERROR: Your bedEntry list is empty.\n");
		fprintf(stderr, "Can't calculate distances on this. Exiting..\n");	
		exit(1);
	}
	
	int middle = ceil(number/2), i;

	struct bedEntry *ptr = malloc(sizeof(struct bedEntry *));
	if(ptr == NULL){
		fprintf(stderr,"ERROR: Couldn't assign a memory pointer. Exiting...\n");	
		exit(1);
	}
	ptr = b;
	int distance = 0;
	for(i = (middle+1); i < number; i++){
		if(!ptr->next || (ptr->next->chr != ptr->chr)){
			distances[i] = -1;
		}else if(ptr->next->start < ptr->end){
			distances[i] = -1;
		}else{
			//calcs the distance between the centers of the probes
			distance += floor((ptr->next->start + ptr->next->end)/2 - (ptr->start + ptr->end)/2);
			distances[i] = distance;
		}
		if(ptr->next)//only advance if you have something to advance to
			ptr=ptr->next;
	}
	ptr=b;
	distance = 0;
	for(i = (middle-1); i >=0 ; i--){
		if(ptr->last == NULL || ptr->last->chr != ptr->chr)
            distances[i] = -1;
        else if(ptr->last->end > ptr->start)
            distances[i] = -1;
        else{
			distance += floor((ptr->start + ptr->end)/2 - (ptr->last->start + ptr->last->end)/2);
            distances[i] = distance;
		}
		if(ptr->last)//only advance if you have something to advance to.
			ptr = ptr->last;
    }
	distances[middle] = 0;
}

struct bedEntry * readBedFile(FILE * fp){
	char line[MAXLINE], chrStr[MAX_TOKEN], *tok;
	int chr, start, end, i;	
	struct bedEntry * b = NULL;	

	while(fgets(line, (MAXLINE), fp)){
		//chomp newlines
		if(line[strlen(line) - 1] == '\n' || line[strlen(line) - 1] == '\r')
			line[strlen(line) - 1] = '\0';

		//break the 15 tokens up and handle separately.
		tok = strtok(line, "\t\n");
		for(i = 0; i <= 15 && tok != NULL; i++)
			{
			if(i ==  0)
				strcpy(chrStr, tok);
			else if(i == 1)
				start = atoi(tok);
			else if(i == 2)
				end = atoi(tok);
            tok = strtok(NULL,"\t");
        }
		chr = chrom_stringToInt(chrStr);
		if(b == NULL){
			b = init_bedEntry(chr, start, end);
		}else{
			pushBedEntry(b,chr, start, end);
		}
    }
	return b;
}	
