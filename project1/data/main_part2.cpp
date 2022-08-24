
#include "stdio.h"
#include "stdlib.h"

#include <time.h>
#include "string.h"
#include <iostream>
#include <limits.h>
#include <fstream>
#include <sstream>
#include "mpi.h"
using namespace std;
//Clock variables
double time_spent;

int total_rounds;									//number of rounds
int partition;								//number of partitions
long size;
int* edges;
int* degree;
double** credits;
int* partitionID;
//Storage Arrays


//MPI parameters
int world_size;
int ranks;
int rounds;
//file reading globals



char * buffer;
char * buffer_partition;

char * partitionFileName;
char * graphFileName;
long maximum = 0;
long minimum = INT_MAX;
clock_t begin, end;
void initialization(char * fileName){

	ifstream filestr(fileName);
	if (!filestr.is_open())
	{
		cerr << "Error Opening File Graph" << endl;
		exit(-1);
	}

	auto pbuf = filestr.rdbuf();
	size = pbuf->pubseekoff(0, ios::end, ios::in);
	pbuf->pubseekpos(0, ios::in);
	buffer = new char[size];
	pbuf->sgetn(buffer, size);
	auto temp = new char[size];
	strcpy(temp, buffer);
	char *p;
	p = strtok(temp, " \n\t");
	long count = 0;

	while (p){
		int i = atoi(p);
		p = strtok(NULL, " \n\t");
		++count;
		if (i > maximum)
		{
			maximum = i;
		}
		if (i < minimum)
		{
			minimum = i;
		}
	}
	strcpy(temp, buffer);
	p = strtok(temp, " \n\t");
	edges = new int[count]();
	degree = new int[maximum + 1]();
	credits = new double*[(maximum + 1)]();
	for (int i = minimum; i <= maximum; ++i)
	{
		credits[i] = new double[rounds]();
	}
	partitionID = new int[maximum + 1]();
	int c = 0;


	//char number[1024];
	//int i = 0;
	//int n = 0;

	//largest = 0;
	//len = 0;
	//fseek(fp, 0, SEEK_END);
	//fileSize = ftell(fp);

	//buffer = (char *)malloc(sizeof(char)* fileSize + 1);

	//long int count = 0;
	//fseek(fp, 0, SEEK_SET);
	//fread(buffer, 1, fileSize, fp);
	//while (count != fileSize){
	//	if ((char)buffer[count] == ' ' || (char)buffer[count] == '\t' || (char)buffer[count] == '\n'){
	//		number[i] = '\0';
	//		i = 0;

	//		n = atoi(number);
	//		if (n == 0){
	//			//	count++;
	//			continue;
	//		}

	//		if (n > largest){
	//			largest = n;
	//		}

	//		if (buffer[count] == '\n'){
	//			len++;
	//		}
	//		//i++;
	//		count++;
	//		continue;
	//	}
	//	number[i] = (char)buffer[count];
	//	i++;
	//	count++;
	//}

	//fseek(fp, 0, SEEK_SET);

	//degree = (int*)calloc(largest + 1, sizeof(int));
	//credit = (float *)calloc((total_rounds + 1)*largest + 1, sizeof(float));
	//pID = (int *)calloc(largest + 1, sizeof(int));
	//edges = (int *)calloc(len * 2, sizeof(int));

	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	if ((world_size != 2) && (world_size != 4)){

		MPI_Abort(MPI_COMM_WORLD, 1);
		exit(0);
	}
	MPI_Comm_rank(MPI_COMM_WORLD, &ranks);

	filestr.close();

}


//read file, parse input and store data into vector
void readFile_Graph(char * fileName)
{

	int c = 0;
	auto temp = new char[size];
	auto p = strtok(temp, " \n\t");
	long minimum = INT_maximum;
	while (p){
		int i = atoi(p);
		p = strtok(NULL, " \n\t");
		++c;
		edges[c] = i;
	}

}

//read partition file and store data.
void readFile_Partition(char * fileName){
	FILE * fp_Partition;
	fp_Partition = fopen(fileName, "r");
	if (fp_Partition == NULL){
		perror("Error Opening File Partition\n");
		//return(-1);
		exit(0);
	}
	ifstream f_Partition(fileName);
	if (!f_Partition.is_open())
	{
		cerr << "Error Opening File Graph" << endl;
		exit(-1);
	}

	auto pbuf = f_Partition.rdbuf();
	size = pbuf->pubseekoff(0, ios::end, ios::in);
	pbuf->pubseekpos(0, ios::in);
	buffer = new char[size];
	pbuf->sgetn(buffer, size);

	auto temp = new char[size];
	strcpy(temp, buffer);
	char *p;
	p = strtok(temp, " \n\t");
	long c = 0;
	int num_type = 0;
	int ID;
	while (p){
		int i = atoi(p);
		p = strtok(NULL, " \n\t");
		++c;
		switch (c%3)
		{
		case(0) :
			ID = i; break;
		case(1) :
			degree[ID] = i; break;
		case(2) :
			partitionID[ID] = i; break;
		default:
			break;
		}
	}
	f_Partition.close();

	//while (count != fileSize_Partition){
	//	if ((char)buffer_partition[count] == ' ' || (char)buffer_partition[count] == '\t' || (char)buffer_partition[count] == '\n'){
	//		number[i] = '\0';
	//		i = 0;

	//		n = atoi(number);
	//		if (n == 0){
	//			//continue;
	//		}
	//		if (num_type == 0){
	//			degree_id = n;
	//			num_type = 1;
	//		}
	//		else if (num_type == 1){
	//			degree[degree_id] = n;
	//			num_type = 2;
	//		}
	//		else{
	//			pID[degree_id] = n;
	//			num_type = 0;
	//		}

	//		count++;
	//		continue;
	//	}
	//	number[i] = (char)buffer_partition[count];
	//	i++;
	//	count++;

	//}

	//fclose(fp_Partition);
	//free(buffer_partition);

}


int mpi_send_credit(int round_number){
	float *send_credit;
	int init_size = maximum;

	send_credit = (float*)calloc(init_size, sizeof(float));

	int i = 0;
	int j = 0;
	for (i = minimum; i <= maximum; i++){
		if (credits[i][round_number] != (float)0){
			send_credit[j] = credits[i][round_number];
			j++;
		}
	}
	MPI_Request request[partition];
	MPI_Status status[partition];
	int m = 0;
	int n = 0;
	for (m = 0; m<partition; m++){
		if (ranks== m){
			for (n = 0; n<partition; n++){
				if (n != m){
					MPI_Isend(send_credit, init_size, MPI_FLOAT, n, round_number, MPI_COMM_WORLD, &request[m]);
					MPI_Wait(&request[m], &status[m]);
				}
			}
		}
		else{
			MPI_Status recv_status[partition];
			MPI_Request recv_request[partition];
			float *recv_credit;
			int count;
			MPI_Probe(m, round_number, MPI_COMM_WORLD, &recv_status[m]);
			MPI_Get_count(&recv_status[m], MPI_FLOAT, &count);
			recv_credit = (float*)malloc(sizeof(float)*count);

			MPI_Irecv(recv_credit, count, MPI_FLOAT, m, round_number, MPI_COMM_WORLD, &recv_request[m]);
			MPI_Wait(&recv_request[m], &recv_status[m]);

			//store them
			int x = 0;
			int y = 0;
			for (x = 1; x <= maximum; x++){
				if (partitionID[x] == m){
					credits[x][round_number] = recv_credit[y];
					y++;
				}
			}

		}
	}
	return 0;
}
void writePartition(ofstream &fp, int p){
	int i = 0;
	int j = 0;
	stringstream ss;
	for (i = minimum; i <= maximum; i++){
		if (degree[i] != 0){
			if (p == partitionID[i]){
				//fprintf(fp, "%i\t%i\t", i, degree[i]);
				ss << i << '\t' << degree[i];
				for (j = 1; j <= total_rounds; j++){
					//fprintf(fp, "%f\t", credits[i][j]);
					ss <<'\t'<< credits[i][j];
				}
				ss << endl;
			}
		}
	}
	fp << ss.str();
}

void writeFile_partition(){
	//FILE *outFile;
	char outFileName[] = "output_p.txt";
	ofstream output;
	//int tmp = 7;
	//MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int i = 0;
	begin = clock();
	for (i = 0; i< partition; i++){

		if (i == ranks){
			outFileName[7] = i + '0';

			output.open(outFileName);
			if (!output.is_open()){
				perror("Error Opening File\n");
				exit(0);
			}
			writePartition(output, i);
			output.close();
		}
	}
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("time to write the output files, partition %i = %.2fsec\n", rank, time_spent);

}

void calculate_round_zero(){
	begin = clock();

	int tnode1 = 0;
	int tnode2 = 1;
	int i = 0;
	for (i = 1; i <= len; i++){
		int first, second;
		first = edges[tnode1];
		second = edges[tnode2];
		if ((pID[first] == ranks) && (pID[second] == ranks)){
			credit[first] = 1;
			credit[second] = 1;
		}
		else if ((pID[first] == ranks) && (pID[second] != ranks)){
			credit[first] = 1;
		}
		else if ((pID[first] != ranks) && (pID[second] == ranks)){
			credit[second] = 1;
		}
		tnode1 += 2;
		tnode2 += 2;

	}

	//end = clock();
	//time_spent = (double)(end - begin)/ CLOCKS_PER_SEC;
	//zero round
	//printf("time for round 1 , partition %i = %.2fsec\n",rank, time_spent);
}



void calculate_round(int r){
	calculate_round_zero();
	mpi_send_credit(0);
	int tnode1 = 0;
	int tnode2 = 1;
	int i = 0;
	int j = 0;
	for (i = 1; i <= r; i++){
		begin = clock();
		tnode1 = 0;
		tnode2 = 1;

		for (j = 1; j <= len; j++){

			int first, second;
			first = edges[tnode1];
			second = edges[tnode2];

			if ((pID[first] == rank) && (pID[second] == rank)){
				credit[largest*i + first] += credit[largest*(i - 1) + second] / degree[second];
				credit[largest*i + second] += credit[largest*(i - 1) + first] / degree[first];

			}
			else if ((pID[first] == rank) && (pID[second] != rank)){
				credit[largest*i + first] += credit[largest*(i - 1) + second] / degree[second];

			}
			else if ((pID[first] != ranks) && (pID[second] == ranks)){
				credit[largest*i + second] += credit[largest*(i - 1) + first] / degree[first];

			}

			tnode1 += 2;
			tnode2 += 2;
		}
		//calculate the time for calculation.
		end = clock();
		time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

		printf("time for round %i, partition %i = %.2fsec\n", (i), ranks, time_spent);
		//write();
		mpi_send_credit(i);
		end = clock();
		time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
		if (ranks == 0){
			printf("total time for round %i: %.2fsec\n", (i), time_spent);
		}

	}
	writeFile_partition();
}


int main(int argc, char ** argv){
	//check parameters
	if (argc<5){
		perror("To Run:'programName graphFile partitionFile rounds'\nExiting");
		exit(-1);
	}

	total_rounds = atoi(argv[3]);
	if (total_rounds<1){
		perror("The round number should be bigger than 0\n Exiting");
		exit(0);
	}

	partition = atoi(argv[4]);
	if (partition<2){
		perror("The partition number should be bigger than 1\n Exiting");
		exit(0);
	}

	begin = clock();
	initialization(argv[1]);

	readFile_Graph(argv[1]); //in_filename

	readFile_Partition(argv[2]);

	end = clock();

	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	//if(rank == 0){
	printf("time to read input files, partition %i = %.2fsec\n", ranks, time_spent);
	//}

	calculate_round(total_rounds);



	MPI_Finalize();
	return 0;
}
