//mpirun -n 2 ./pro fl_compact.tab  fl_compact_part.2 5 2
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "mpi.h"

#define VERTEXID 0
#define DEGREE 1
#define PARTITIONID 2
//Clock variables
clock_t read_start, read_finish;
clock_t write_start, write_finish;
clock_t round_start, round_finish;
clock_t round_part_start, round_part_finish;
double exe_time;

long int file_size;
int num_line;				
int max_id;					
int *nodeDegree;
int *edges;

int num_of_rounds;			
int partition;				
int num_of_proc;
int rank_of_proc;
int *pID;

double *credit;
double *update;
char *buffer;

FILE *fp;
MPI_Request request[partition];
MPI_Status status[partition];
MPI_Status status_temp[partition];
MPI_Request request_temp[partition];

void construct_Graph(char * fileName){
	int i = 0;
	long int index;
	char toNumberOne[64];
	char toNumberTwo[64];
	int node_id = 0;
	int edge_id = 0;

	max_id = 0;
	num_line = 0;

	fp = fopen(fileName, "r");
	if(fp == NULL){
		perror("Opening graph file failed! Try again! \n");
		exit(0);
	}

	fseek(fp, 0, SEEK_END);
	//Total size of fl_compact.tab
	file_size = ftell(fp);

	buffer = (char *)malloc(sizeof(char)* file_size+1);
	
	fseek(fp, 0, SEEK_SET);
	//Read all data to the buffer
	fread(buffer, 1, file_size, fp);

	//Find max id and initialize the storage
	for(index = 0; index < file_size; index++){
		if(buffer[index] == '\t' || buffer[index] == '\n'){
			toNumberOne[i] = '\0';
			i = 0;

			node_id = atoi(toNumberOne);

			if(node_id > max_id){
				max_id = node_id;
			}

			if(buffer[index] == '\n'){
				num_line++;
			}
		} else{
			toNumberOne[i] = buffer[index];
			i++;
		}
	}

	// Edge is represend as two adjacent nodes
	edges = (int *)calloc(num_line * 2, sizeof(int));

	// Credit for each round is stored in the array
	credit = (double *)calloc((num_of_rounds + 1) * max_id +1, sizeof(double));

	i = 0;
	node_id = 0;

	for(index = 0; index < file_size; index++){
		if(buffer[index] == '\t' || buffer[index] == '\n'){
			toNumberTwo[i] = '\0';
			i = 0;

			node_id = atoi(toNumberTwo);
			// Initialize credit for each node
			credit[node_id] = 1;
			// Edge is represend as two adjacent nodes
			edges[edge_id] = node_id;
			edge_id++;
		} else{
			toNumberTwo[i] = buffer[index];
			i++;
		}
	}

	free(buffer);
	fclose(fp);
}

void construct_Partition(char * fileName){
	int i = 0;	
	long int index = 0;
	int input = 0;
	int degree_id = 0;
	char toNumber[64];							
	int node_id = 0;

	fp = fopen(fileName, "r");
	if(fp == NULL){
		perror("Opening graph file failed! Try again! \n");
		exit(0);
	}

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);		//Total size of fl_compact_part.*
	buffer = (char *)malloc(sizeof(char)* file_size+1);
	
	fseek(fp, 0, SEEK_SET);
	fread(buffer, 1, file_size, fp);
		
	for(index = 0; index < file_size; index++) {
		if(buffer[index] == '\t' || buffer[index] == '\n'){
			toNumber[i] = '\0';
			i = 0;

			if(buffer[index] == '\n'){
				input = VERTEXID;
			}

			node_id = atoi(toNumber);
			if(input == VERTEXID){
				degree_id = node_id;
				input = DEGREE;
			}else if(input == DEGREE){
				nodeDegree[degree_id] = node_id;
				input = PARTITIONID;
			}else{
				pID[degree_id] = node_id;
				input = VERTEXID;
			}
		} else {
			toNumber[i] = buffer[index];
			i++;
		}
	}

	free(buffer);
	fclose(fp);	
}

void write_file(){
	FILE *fp;
	char outputFile[] = "fl_pageRanki_s17\0";
	int i;
	int j;
	int k = 0;
	
	for(i = 0; i < partition; i++){
		if(i == rank_of_proc){
			outputFile[11] = i + '0';

			fp = fopen(outputFile, "w");
			if(fp == NULL){
				perror("Write file failed!\n");
				exit(0);
			}

			for(j = 1; j <= max_id; j++){
				if(nodeDegree[j] != 0){
					if(i == pID[j]){
						fprintf(fp, "%d\t%d\t", j, nodeDegree[j]);
						for(k = 1; k <= num_of_rounds; k++){
							fprintf(fp, "%f\t", credit[k * max_id + j]);
						}
						fprintf(fp, "\n");
					}
				}
			}

			fclose(fp);
		}
	}
}

void synchronization(int round_number){
	int i;
	int j = 0;

	for(i = 1; i <= max_id; i++){
		if(credit[round_number * max_id + i] != 0){
			update[j] = credit[round_number * max_id + i];
			j++;
		}
	}

	for(i = 0; i < partition; i++){
		if(rank_of_proc == i){
			for(j = 0; j < partition; j++){
				if(j != i){
					MPI_Isend(update, max_id, MPI_DOUBLE, j, round_number, MPI_COMM_WORLD, &request[i]);
					MPI_Wait(&request[i], &status[i]);
				}
			}
		}else{
			double *recv_credit;
			int count;
			MPI_Probe(i, round_number, MPI_COMM_WORLD, &status_temp[i]);
			MPI_Get_count(&status_temp[i], MPI_DOUBLE, &count);
			recv_credit = (double*) malloc(sizeof(double)*count);

			MPI_Irecv(recv_credit, count, MPI_DOUBLE, i, round_number, MPI_COMM_WORLD, &request_temp[i]);
			MPI_Wait(&request_temp[i], &status_temp[i]);

			//store them
			int x = 0;
			int y = 0;
			for(x = 1; x <= max_id; x++){
				if(pID[x] == i){
					credit[round_number*max_id+x] = recv_credit[y];
					y++;
				}
			}

		}
	}
	// return 0;
}

void pageRank(int rounds){
	synchronization(0);

	int i;
	int j;
	int first_node;
	int second_node;
	int first_node_index = 0;
	int edge_second_node = 1;

	// Compute pageRank for rounds i
	for(i = 1; i <= rounds; i++){
		round_start = clock();
		round_part_start = clock();
		printf("Start round %d: ", i);
		first_node_index = 0;
		edge_second_node = 1;
	
		for(j = 1; j <= num_line; j++){
			first_node = edges[first_node_index];
			second_node = edges[edge_second_node];

			if((pID[first_node] == rank_of_proc) && (pID[second_node] == rank_of_proc)){
				credit[max_id * i + first_node] += credit[max_id * (i - 1) + second_node] / nodeDegree[second_node];
				credit[max_id * i + second_node] += credit[max_id * (i - 1) + first_node] / nodeDegree[first_node];		
			}else if((pID[first_node] == rank_of_proc) && (pID[second_node] != rank_of_proc)){
				credit[max_id*i+first_node] += credit[max_id * (i - 1) + second_node] / nodeDegree[second_node];
			
			}else if((pID[first_node]!=rank_of_proc)&&(pID[second_node] == rank_of_proc)){
				credit[max_id*i+second_node]+=credit[max_id*(i-1)+first_node] / nodeDegree[first_node];
			
			}
		
			first_node_index +=2;
			edge_second_node +=2;
		}

		round_part_finish = clock();
		exe_time = (double)(round_part_finish - round_part_start) / CLOCKS_PER_SEC;
		
		printf("time for round %d, partition %d = %.2fsec\n",i ,rank_of_proc, exe_time);

		synchronization(i);

		round_finish = clock();
		exe_time = (double)(round_finish - round_start) / CLOCKS_PER_SEC;

		if(rank_of_proc == 0){
			printf("total time for round %d: %.2fsec\n", i, exe_time);
			printf("Round %d ends!----------------------\n", i);
		}
	}
}


int main(int argc, char ** argv){

	if(argc != 5){
    	perror("Arguments error: prog GraphFile Partitionfile rounds Partitions!");
		exit(-1);
    }

	num_of_rounds = atoi(argv[3]);
    partition = atoi(argv[4]);

	if(num_of_rounds < 1 || partition < 2){
		perror("Rounds must be greater than 0 and Partitions must be greater than 1!");
		exit(0);
	}

	MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &num_of_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_of_proc);

	read_start = clock();			//Start time for reading files
	
	construct_Graph(argv[1]);

	nodeDegree = (int*)calloc(max_id+1, sizeof(int));
    pID = (int *)calloc(max_id+1, sizeof(int));
    update = (double*)calloc(max_id, sizeof(double));

	construct_Partition(argv[2]);

	read_finish = clock();				//Finish time for reading files

	exe_time = (double)(read_finish - read_start)/ CLOCKS_PER_SEC;

	printf("time to read input files, partition %d = %.2fsec\n",rank_of_proc, exe_time);

	pageRank(num_of_rounds);

	write_start = clock();		//Start time for writing file
	write_file();
	write_finish = clock();		//Finish time for writing file
	exe_time = (double)(write_finish - write_start)/ CLOCKS_PER_SEC;
	printf("time to write the output files, partition %d = %.2fsec\n", rank_of_proc, exe_time);

	free(nodeDegree);
	free(edges);
	free(credit);
	free(update);
	free(pID);
	free(fp);

	MPI_Finalize();
	return 0;
}
