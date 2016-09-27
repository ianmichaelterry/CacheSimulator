///////////////////////////
//
//  Ian Michael Terry
//  imt12
//
///////////////////////////


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

//struct for cache entries
typedef struct entryStruct {
		unsigned int valid;
		unsigned int tag;
		unsigned int dirty;
		unsigned int LRU;
} entryType;

int main(int argc, char * argv[]){
	char *blockSize;
	char *sets;
	char *associativity;

	//parse command line arguments
	int i;
	for(i = 1; i< argc; i+=2){
		if(strcmp(argv[i],"-n") == 0){
			associativity = argv[i+1];
		}
		if(strcmp(argv[i],"-s") == 0){
			sets = argv[i+1];
		}
		if(strcmp(argv[i],"-b") == 0){
			blockSize = argv[i+1];
		}
	}

	printf("Block Size: %s\n",blockSize);
	printf("Number of Sets: %s\n",sets);
	printf("Associativity: %s\n",associativity);

	int associativityInt = atoi(associativity);
	int setsInt = atoi(sets);
	//calculate index offset and tag
	int indexSize = log10(setsInt)/log10(2);
	int offset = log10(atoi(blockSize))/log10(2);
	int tag = 32 - offset - indexSize;

	entryType writeThroughCache[setsInt][associativityInt];
	entryType writeBackCache[setsInt][associativityInt];

	//initializes invalid bits of both caches
	for (i = 0; i < setsInt; i++){
		int j;
		for(j = 0; j< associativityInt; j++){
			writeThroughCache[i][j].valid = 0;
			writeBackCache[i][j].valid = 0;
			writeBackCache[i][j].dirty = 0;
		}
	}

	//counters
	int totalReferences = 0;

	int throughHits = 0;
	int throughMisses = 0;
	int throughMems = 0;
	int throughLRUCounter = 0;

	int backHits = 0;
	int backMisses = 0;
	int backMems = 0;
	int backLRUCounter = 0;

	char line[130];
	while(fgets(line, 130, stdin)){
		//parses input
		const char s[2] = " ";
    char *token;
		token = strtok(line, s);
		int i = 0;
		char instr;
		int value;
		while( i < 2 ) {
			if (i == 0){
				instr = *token;
			}else{
				value = atoi(token);
			}
			token = strtok(NULL, s);
			i +=  1;
		}

		totalReferences++;

		//extracts tag and index
		int theTag = value >> (32 - tag);
		int theIndex = (value << tag) >> (tag + offset);

		if (instr == 'R'){

			//Handles write through
			int refWritten = 0;
			int i;
			for(i = 0; i < associativityInt; i++){
				if(writeThroughCache[theIndex][i].valid == 0){
					entryType thisEntry;
					thisEntry.valid = 1;
					thisEntry.tag = theTag;
					thisEntry.LRU = throughLRUCounter++;
					writeThroughCache[theIndex][i] = thisEntry;
					throughMisses++;
					throughMems++;
					refWritten = 1;
					break;
				}
				if(writeThroughCache[theIndex][i].tag == theTag && writeThroughCache[theIndex][i].valid == 1){
					throughHits++;
					//writeThroughCache[theIndex][i].LRU = throughLRUCounter++;
					refWritten=1;
					break;
				}
			}
			if (refWritten == 0){
				int lowestLRU = writeThroughCache[theIndex][0].LRU;
				int oldestEntry=0;
				int i;
				for(i = 0; i < associativityInt; i++){
					if (writeThroughCache[theIndex][i].LRU < lowestLRU){
							lowestLRU = writeThroughCache[theIndex][i].LRU;
							oldestEntry = i;
					}
				}
				entryType thisEntry;
				thisEntry.valid = 1;
				thisEntry.tag = theTag;
				thisEntry.LRU = throughLRUCounter++;
				writeThroughCache[theIndex][oldestEntry] = thisEntry;
				throughMisses++;
				throughMems++;
			}



			//Handles write-back
			int readHit = 0;
			//int i;
			for(i = 0; i < associativityInt; i++){
				if(writeBackCache[theIndex][i].tag == theTag && writeBackCache[theIndex][i].valid == 1){
					backHits++;
					readHit = 666;
					break;
				}
			}
			if (readHit == 0){

				int invalidFound = 0;
				int i;
				for(i = 0; i < associativityInt; i++){
						if(writeBackCache[theIndex][i].valid == 0){
							entryType thisEntry;
							thisEntry.valid = 1;
							thisEntry.tag = theTag;
							thisEntry.LRU = backLRUCounter++;
							thisEntry.dirty = 0;
							writeBackCache[theIndex][i] = thisEntry;
							backMisses++;
							backMems++;
							invalidFound = 1;
							break;
						}

				}
				if (invalidFound == 0){
					int lowestLRU = writeBackCache[theIndex][0].LRU;
					int oldestEntry=0;
					int i;
					for(i = 0; i < associativityInt; i++){
						if (writeBackCache[theIndex][i].LRU < lowestLRU){
								lowestLRU = writeBackCache[theIndex][i].LRU;
								oldestEntry = i;
						}
					}

					if(writeBackCache[theIndex][oldestEntry].dirty == 1){
						backMems++;
					}
					entryType thisEntry;
					thisEntry.valid = 1;
					thisEntry.tag = theTag;
					thisEntry.LRU = backLRUCounter++;
					thisEntry.dirty=0;
					writeBackCache[theIndex][oldestEntry] = thisEntry;
					backMisses++;
					backMems++;
				}
			}


		}else if(instr == 'W'){

			//Handles write-through
			int blockFound = 0;
			int i;
			for(i = 0; i < associativityInt; i++){

				if(writeThroughCache[theIndex][i].valid == 0){
					break;
				}
				if(writeThroughCache[theIndex][i].tag == theTag && writeThroughCache[theIndex][i].valid == 1){
						writeThroughCache[theIndex][i].LRU = throughLRUCounter++;
						blockFound = 1;
						throughHits++;
						throughMems++;
				}

			}
			if (blockFound == 0){
				throughMisses++;
				throughMems++;
			}

			//Handles write-back
			blockFound = 0;

			for(i = 0; i < associativityInt; i++){
				if(writeBackCache[theIndex][i].valid == 0){
					break;
				}
				if(writeBackCache[theIndex][i].tag == theTag && writeBackCache[theIndex][i].valid == 1){
						writeBackCache[theIndex][i].LRU = backLRUCounter++;
						writeBackCache[theIndex][i].dirty = 1;
						blockFound = 1;
						backHits++;

				}

			}
			if (blockFound == 0){
				int invalidFound = 0;
				int i;
				for(i = 0; i < associativityInt; i++){
					if(writeBackCache[theIndex][i].valid == 0){
						entryType thisEntry;
						thisEntry.valid = 1;
						thisEntry.tag = theTag;
						thisEntry.LRU = backLRUCounter++;
						thisEntry.dirty = 1;
						writeBackCache[theIndex][i] = thisEntry;
						backMisses++;
						backMems++;
						invalidFound = 1;
						break;
					}

				}
				if (invalidFound ==0){
					int lowestLRU = writeBackCache[theIndex][0].LRU;
					int oldestEntry=0;
					int i;
					for(i = 0; i < associativityInt; i++){
						if (writeBackCache[theIndex][i].LRU < lowestLRU){
								lowestLRU = writeBackCache[theIndex][i].LRU;
								oldestEntry = i;
						}
					}

					if( writeBackCache[theIndex][oldestEntry].dirty == 1){
						backMems++;
					}
					entryType thisEntry;
					thisEntry.valid = 1;
					thisEntry.tag = theTag;
					thisEntry.LRU = backLRUCounter++;
					thisEntry.dirty = 1;
					writeBackCache[theIndex][oldestEntry] = thisEntry;
					backMisses++;
					backMems++;
				}

			}

		}



	}
	printf("Number of offset bits: %d\n", offset);
	printf("Number of index bits: %d\n", indexSize);
	printf("Number of tag bits: %d\n\n", tag);
	printf("******************************************\n");
	printf("Write-through with No Write Allocate\n");
	printf("******************************************\n");
	printf("Total number of references: %i\n", totalReferences);
	printf("Hits: %i\n", throughHits);
	printf("Misses: %i\n", throughMisses);
	printf("Memory References: %i\n\n", throughMems);

	printf("******************************************\n");
	printf("Write-back with Write Allocate\n");
	printf("******************************************\n");
	printf("Total number of references: %i\n", totalReferences);
	printf("Hits: %i\n", backHits);
	printf("Misses: %i\n", backMisses);
	printf("Memory References: %i\n", backMems);

}
