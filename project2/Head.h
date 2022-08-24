#include <stdio.h> 
#include <stdlib.h> 
#include <iostream>
#include <unistd.h>
#include <string>
#include <string.h>
#include <vector>
#include <fstream>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <queue>
#include <chrono>
using namespace std;

typedef chrono::high_resolution_clock Clock;

typedef struct
{
	int Time;
	string message;
}Post;