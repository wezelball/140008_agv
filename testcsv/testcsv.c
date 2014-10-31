#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Define a structure for a line segment */
typedef struct lines
{
	int segId;	//canonical segment identifier
	int end1; 	//endpoint 1
	int end2; 	//endpoint 2
	int length;	//guess
} line_segment;

/* Get the number of lines in the csv file*/
size_t getNumLines()
{
	char ch;
	size_t lines = 0;

	FILE* fp = fopen("map.dat", "r");
	/* 
	 * Iterate through each character until feof()
	 * If a newline, increment line number count
	 */
	while(!feof(fp))
	{
  		ch = fgetc(fp);
  		if(ch == '\n')
  		{
    		lines++;
  		}

	}
	fclose(fp);  // close the file
	return(lines);
}
/*
 * Loads a line segment structure using 
 * damn pointers, which I had no choice
 * to use cuz c iz stoopid
 *
 * A line segment structure is an array
 * structure of the following form:
 * line[n].segid	- ID number of segment as defined in physical layout
 * line[n].end1 	- Endpoint 1 of segment (event)
 * line[n].end2 	- Endpoint 2 of segment (event)
 * line[n].length 	- Length of line in inches, rounded to nearest inch
 * 
 * n is an integer array reference
 * 
 * All of the above values are in int
*/
line_segment* initLineSegment()
{
	size_t numLines;
	char line[1024];
	int i = 0;
	
	/* Open the data file*/ 
	FILE* stream = fopen("map.dat", "r");

	numLines = getNumLines(stream);
	/* 
	 * Rewind sets file pointer back to beginning of file,
	 * necessary for strtok calls below to work 
	 */
	rewind(stream);
	
	/* Allocate space for segment array */ 
	line_segment* segment = malloc(sizeof(line) * numLines);

	while (fgets(line, 1024, stream))
    {
        char* tmp = strdup(line);
		segment[i].segId = atoi(strtok(tmp, ","));
		segment[i].end1 = atoi(strtok(NULL, ","));
		segment[i].end2 = atoi(strtok(NULL, ","));
		segment[i].length = atoi(strtok(NULL, "\n"));
		
		// NOTE strtok clobbers tmp
        free(tmp);
		i++;	
    }
	fclose(stream); // close file before leaving
	return segment;
}


int main()
{
 	line_segment* lineSegment;
	size_t i;
	size_t nlines;

	/*
	 * initLineSegment returns a loaded line segment
	 * from 'map.dat'
	 * 
	 */
	lineSegment = initLineSegment();
	/* The number of lines in the segment structure*/
	nlines = getNumLines();

	/* This is just some test crap*/
	for (i=0;i<nlines;i++)
	{
		printf("Line segment: %d\n", i);
		printf("Segment Id: %d\n", lineSegment[i].segId);
		printf("End1: %d\n", lineSegment[i].end1);
		printf("End2: %d\n", lineSegment[i].end2);
		printf("Length: %d\n", lineSegment[i].length);
		printf("\n");
	}
	return(0);
}
