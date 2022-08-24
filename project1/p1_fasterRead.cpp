#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <cstring>
#include <vector>
#include <time.h>


//Class for object information storing
class Node{
	int ID;
	int degree;
	std::vector<float> credits;
	//std::vector<Node *> adjList;
	std::vector<int> neighbors;

	public:
		Node(int x, int y): ID(x), degree(y){}
		void initset(int id, float deg){
			ID = id;
			degree = deg;
			credits.push_back(1);
			//neighbors.resize(z); 
		}
		void incDegree(){
			degree += 1;
		}

		//find method to check if member is in neighbor list
		int findNeighbors(int x){
			for( unsigned int i = 0; i < neighbors.size(); i++ )
			{
				//printf("Element[%i] = %i \n",i ,neighbors[i]);
				if(neighbors[i] == x){
					return 1;
				}
			}
			return 0;
		}

		//set methods for adding to the credit & neighbor vector
		void storeCredit(float x){
			credits.push_back(x);
		}

		void storeNeighbor(int x){
			neighbors.push_back(x);
		}

		//Getter methods to retrieve information
		int getID(){
			return ID;
		}
		int getDegree(){
			return degree;
		}
		std::vector<float> getVectorCredits(){
			return credits;
		}
		std::vector<int> getVectorNeighbors(){
			return neighbors;
		}
		
		float getCredit(int i){
			return credits[i];
		}
		int getNeighbor(int i){
			return neighbors[i];
		}

		//print methods for check.
		void printVectorCredits(){
			for( unsigned int i = 0; i < credits.size(); i++ )
			{
				printf("Element[%i] = %f \n",i ,credits[i]);
			}
		}
		void printVectorNeighbors(){
			for( unsigned int i = 0; i < neighbors.size(); i++ )
			{
				printf("Element[%i] = %i \n",i ,neighbors[i]);
			}
		}
};

//global variable declaration
std::vector<Node *> vList;
	
clock_t begin, end;
double time_spent;
int * edges;
int * degree;
float * credits;
int maxId;
int nLine;



//read file, parse input and store data into vector
void readFile(char * fileName){
	FILE *fp;
	//unsigned char buffer[1024];

	fp = fopen(fileName, "r");
	if(fp == NULL){
		perror("Error Opening File\n");
		//return(-1);
		exit(0);
	}

	char number[1024];
	int i = 0;
	int n = 0;
	int e1 = 0;
	int round = 5;

	maxId = 0;
	nLine = 0;
	fseek(fp, 0, SEEK_END);
	long int fileSize = ftell(fp);
	//printf("file size = %li\n", fileSize);
	char * buffer = (char *)malloc(sizeof(char)* fileSize+1);
//read file and allocate space
	//begin = clock();
	long int count = 0;
	fseek(fp, 0, SEEK_SET);
	fread(buffer, 1, fileSize, fp);
	while(count != fileSize){
		if((char)buffer[count] == ' ' || (char)buffer[count] == '\t' || (char)buffer[count] == '\n'){
			number[i] = '\0';
			i = 0;

			n = atoi(number);
			//printf("number is %i \n", n);
			if(n == 0){
				continue;
			}

			if(n > maxId){
				maxId = n;
			}

			if(buffer[count] == '\n'){
				nLine++;
			}
			//i++;
			count++;
			continue;
		}
		number[i] = (char)buffer[count];
		i++;
		count++;
	}

	//end = clock();

	//time_spent = (double)(end - begin)/ CLOCKS_PER_SEC;
	//printf("time to read input file = %.2fsec\n", time_spent);

	//printf("max Id %i\n", maxId);
	//printf("number of lines %i\n", (nLine));
	fseek(fp, 0, SEEK_SET);

	int edgeSize = nLine *2 ;
	int degreeSize = maxId +1;
	int creditSize = (maxId*(round+1)) + 1;


	edges = (int *)calloc(edgeSize, sizeof(int));
	degree = (int *)calloc(degreeSize, sizeof(int));
	credits = (float *)calloc(creditSize, sizeof(float));


	i = 0;
	count = 0;
	while(count != fileSize){
		if((char)buffer[count] == ' ' || (char)buffer[count] == '\t' || (char)buffer[count] == '\n'){
			number[i] = '\0';
			i = 0;

			n = atoi(number);
			if(n == 0){
				continue;
			}

			edges[e1] = n;
			e1++;
			
			degree[n] = degree[n] + 1; //increase degree for that number
			//printf("%i of degree %i\n", n, degree[n]);
			//i++;
			count++;
			continue;
		}
		number[i] = (char)buffer[count];
		i++;
		count++;

	}

	for(int z =1; z<=maxId; z++){
		credits[z] = 1.0;
 	}


     fclose(fp);
}

int calc(int round_number){
	//credit calculations
	int tnode1 =0;
	int tnode2 =1;
	for(int round = 1; round <= round_number; round++){
		begin = clock();
		tnode1 = 0;
		tnode2 = 1;
		for(int b = 0; b < nLine; b++){

			credits[maxId*round+edges[tnode1]] += credits[maxId*(round-1)+edges[tnode2]]/degree[edges[tnode2]]; 
			credits[maxId*round+edges[tnode2]] += credits[maxId*(round-1)+edges[tnode1]]/degree[edges[tnode1]]; 

			tnode1 +=2;
			tnode2 +=2;
			
		}
		end = clock();
		time_spent = (double)(end - begin)/ CLOCKS_PER_SEC;
		printf("time for round %i = %.2fsec\n",(round), time_spent);
	}
}

void writeFile(char * fileName, int round){
	begin = clock();
	FILE * fp;

	fp = fopen(fileName, "w");
	if(fp == NULL){
		perror("Error Opening File\n");
		//return(-1);
		exit(0);
	}
	// for( unsigned int i = 0; i < vList.size(); i++)
	// {
	// 	if(vList[i] != NULL){
	// 		fprintf(fp, "%i\t%i", vList[i]->getID(), vList[i]->getDegree());
	// 		for(int j = 0; j < round; j++){
	// 			fprintf(fp, "\t%.6f", vList[i]->getCredit(j+1));
	// 		}
	// 		fprintf(fp, "\n");
	// 	}
	// }

	for(int i = 1; i<= maxId; i++){
		if(degree[i] != 0){
			fprintf(fp, "%i\t%i\t", i, degree[i]);
			for(int j = 1; j<=round; j++){
				fprintf(fp, "%f\t", credits[j*maxId + i]);
			}
			fprintf(fp, "\n");
		}
	}

	fclose(fp);

	end = clock();
	time_spent = (double)(end - begin)/ CLOCKS_PER_SEC;
	printf("time to write the output file = %.2fsec\n", time_spent);
}

int main(int argc, char* argv[]){
	if(argc != 4){
		perror("To Run:'programName inputFile outputFile rounds'\nExiting");
		exit(-1);
	}
	begin = clock();
	readFile(argv[1]); //in_filename
	end = clock();
	//readFile_Partition();
	time_spent = (double)(end - begin)/ CLOCKS_PER_SEC;
	printf("time to read input file = %.2fsec\n", time_spent);
	calc(atoi(argv[3])); //rounds
	writeFile(argv[2], atoi(argv[3])); //out_filename, rounds
	//test();

	free(edges);

	free(degree);

	free(credits);
	

	return 0;
}

