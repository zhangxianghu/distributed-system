//mpirun -n 2 ./pro fl_compact.tab  fl_compact_part.2 5 2
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "mpi.h"

//Clock variables
clock_t begin, end;
double time_spent;

long int file_size;
int num_line;				//number of lines in file
int max_id;					//max_id node ID
int *degree;
int *edges;

//MPI parameters
int total_rounds;			//number of rounds
int partition;				//number of partitions
int num_of_proc;
int rank_of_proc;

//file reading globals

double *credit;
int *pID;
char *buffer;
FILE *fp;

void construct_Graph(char * fileName){
	long int index;
	char toNumberOne[64];
	char toNumberTwo[64];
	int i = 0;
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
		if((char)buffer[index] == '\t' || (char)buffer[index] == '\n'){
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
			toNumberOne[i] = (char)buffer[index];
			i++;
		}
	}

	// Edge is represend as two adjacent nodes
	edges = (int *)calloc(num_line*2, sizeof(int));
	credit = (double *)calloc((total_rounds+1)*max_id +1, sizeof(double));

	i = 0;
	node_id = 0;

	for(index = 0; index < file_size; index++){
		if((char)buffer[index] == '\t' || (char)buffer[index] == '\n'){
			toNumberTwo[i] = '\0';
			i = 0;

			node_id = atoi(toNumberTwo);
			// Initialize credit for each node
			credit[node_id] = 1;
			// Edge is represend as two adjacent nodes
			edges[edge_id] = node_id;
			edge_id++;
		} else{
			toNumberTwo[i] = (char)buffer[index];
			i++;
		}
	}

	free(buffer);
	fclose(fp);
}

void construct_Partition(char * fileName){
	int input = 0;
	int degree_id = 0;
	char toNumber[64];			
	int i = 0;					
	int node_id = 0;
	long int index = 0;

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
		if((char)buffer[index] == '\t' || (char)buffer[index] == '\n'){
			toNumber[i] = '\0';
			i = 0;

			node_id = atoi(toNumber);
			if(input == 0){
				degree_id = node_id;
				input = 1;
			}else if(input == 1){
				degree[degree_id] = node_id;
				input = 2;
			}else{
				pID[degree_id] = node_id;
				input = 0;
			}
		} else {
			toNumber[i] = (char)buffer[index];
			i++;
		}
	}

	free(buffer);
	fclose(fp);	
}

void writePartition(FILE * fp, int p){
	int i = 0;
	int j = 0;
	for(i = 1; i<=max_id; i++){
		if(degree[i] != 0){
			if(p == pID[i]){
				fprintf(fp, "%i\t%i\t", i, degree[i]);
				for(j = 1; j<=total_rounds; j++){
					fprintf(fp, "%f\t", credit[j*max_id + i]);
				}
				fprintf(fp, "\n");
			}
		}
	}
}

void writeFile_partition(){
	FILE *outFile;
	char outFileName[] = "output_p.txt\0";
	int i = 0;
	begin = clock();
	for(i = 0; i< partition; i++){

		if(i == rank_of_proc){
			outFileName[7] = i + '0';

			outFile = fopen(outFileName, "w");
			if(outFile == NULL){
				perror("Error Opening File\n");
				exit(0);
			}
			writePartition(outFile, i);
			fclose(outFile);
		}
	}
	end = clock();
	time_spent = (double)(end - begin)/ CLOCKS_PER_SEC;
	printf("time to write the output files, partition %i = %.2fsec\n", rank_of_proc, time_spent);

}

int mpi_send_credit(int round_number){
	double *send_credit;
	int init_size = max_id;

	send_credit = (double*)calloc(init_size, sizeof(double));

	int i = 0;
	int j = 0;
	for(i=1;i<=max_id;i++){
		if(credit[round_number*max_id+i]!=(double)0){
			send_credit[j] = credit[round_number*max_id+i];
			j++;
		}
	}
	MPI_Request request[partition];
	MPI_Status status[partition];
	int m = 0;
	int n = 0;
	for(m=0;m<partition;m++){
		if(rank_of_proc==m){
			for(n=0;n<partition;n++){
				if(n!=m){
					MPI_Isend(send_credit, init_size, MPI_DOUBLE, n, round_number, MPI_COMM_WORLD, &request[m]);
					MPI_Wait(&request[m], &status[m]);
				}
			}
		}else{
			MPI_Status recv_status[partition];
			MPI_Request recv_request[partition];
			double *recv_credit;
			int count;
			MPI_Probe(m, round_number, MPI_COMM_WORLD, &recv_status[m]);
			MPI_Get_count(&recv_status[m], MPI_DOUBLE, &count);
			recv_credit = (double*) malloc(sizeof(double)*count);

			MPI_Irecv(recv_credit, count, MPI_DOUBLE, m, round_number, MPI_COMM_WORLD, &recv_request[m]);
			MPI_Wait(&recv_request[m], &recv_status[m]);

			//store them
			int x=0;
			int y = 0;
			for(x=1;x<=max_id;x++){
				if(pID[x]==m){
					credit[round_number*max_id+x] = recv_credit[y];
					y++;
				}
			}

		}
	}
	return 0;
}

void calculate_round(int r){
	begin = clock();
	mpi_send_credit(0);
	int tnode1 = 0;
	int tnode2 = 1;
	int i = 0;
	int j = 0;
	for(i=1;i<=r;i++){
		begin = clock();
		tnode1 = 0;
		tnode2 = 1;
	
		for(j=1;j<=num_line;j++){

			int first,second;
			first= edges[tnode1];
			second= edges[tnode2];

			if((pID[first]==rank_of_proc)&&(pID[second]==rank_of_proc)){
				credit[max_id*i+first]+=credit[max_id*(i-1)+second]/degree[second];
				credit[max_id*i+second]+=credit[max_id*(i-1)+first]/degree[first];
			
			}else if((pID[first]==rank_of_proc)&&(pID[second]!=rank_of_proc)){
				credit[max_id*i+first]+=credit[max_id*(i-1)+second]/degree[second];
			
			}else if((pID[first]!=rank_of_proc)&&(pID[second]==rank_of_proc)){
				credit[max_id*i+second]+=credit[max_id*(i-1)+first]/degree[first];
			
			}
		
			tnode1 +=2;
			tnode2 +=2;
		}
		//calculate the time for calculation.
		end = clock();
		time_spent = (double)(end - begin)/ CLOCKS_PER_SEC;
		
		printf("time for round %i, partition %i = %.2fsec\n",(i),rank_of_proc, time_spent);
		//write();
		mpi_send_credit(i);
		end = clock();
		time_spent = (double)(end - begin)/ CLOCKS_PER_SEC;
		if(rank_of_proc == 0){
			printf("total time for round %i: %.2fsec\n",(i), time_spent);
		}

	}
	writeFile_partition();
}


int main(int argc, char ** argv){

    if(argc != 5){
    	perror("Arguments error: prog GraphFile Partitionfile rounds Partitions!");
		exit(-1);
    }

    total_rounds = atoi(argv[3]);
	if(total_rounds < 1){
		perror("Rounds must be greater than 0!");
		exit(0);
	}

    partition = atoi(argv[4]);
	if(partition < 2){
		perror("Partitions must be greater than 1!");
		exit(0);
	}

	MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &num_of_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_of_proc);

	begin = clock();			//Start time
	
	construct_Graph(argv[1]);

	degree = (int*)calloc(max_id+1, sizeof(int));
    
	pID = (int *)calloc(max_id+1, sizeof(int));

	// readFile_Graph(argv[1]); //in_filename

	construct_Partition(argv[2]);
	// initialize_credit();

	end = clock();				//Finish time

	time_spent = (double)(end - begin)/ CLOCKS_PER_SEC;
	//if(rank_of_proc == 0){
	printf("time to read input files, partition %i = %.2fsec\n",rank_of_proc, time_spent);
	//}

	calculate_round(total_rounds);

	free(degree);
	free(credit);
	free(edges);
	MPI_Finalize();
	return 0;
}
