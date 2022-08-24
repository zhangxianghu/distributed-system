
# include "Client.hpp"


int main(int argc, char** argv)
{
	string configFile;
	string inputFile;
	string outputFile;

	int paraOpt = 0;

	while ((paraOpt = getopt(argc, argv,"c:i:o:")) != -1) {
		switch (paraOpt) {
			case 'c' : configFile = string(strdup(optarg)); 
				break;
			case 'i' : inputFile = string(strdup(optarg)); 
				break;
			case 'o' : outputFile = string(strdup(optarg)); 
				break;
		}
	}

	Client(configFile, inputFile, outputFile);
	return 0;
}
