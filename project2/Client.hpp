// ring -c Config1.txt -i Input1.txt -o Output1.txt
// ring -c Config2.txt -i Input2.txt -o Output2.txt
// ring -c Config3.txt -i Input3.txt -o Output3.txt
#include "Head.h"

class Client
{
	
public:
	int s;
	int port;
    int joinTime;
    int stopTime;
    int PortNumber;
    int maxPortNum;
    int minPortNum;
    int portRange;
    int slen; 
    int recv_len;
    int NextHopPort = 0;
    int PreviousHopPort = 0;
    int tempPortNum;
    int electionRound =0;
    int coordinator = 0;
    int TokenNumber = -1;
    
    char buffer[1024];
    bool ringformed = false;
    bool Elected = false;
    bool tokenExist = false;
    bool postExist = false;
    bool PreviousHopPortCheck = true;
    bool token = false;

    string postcontent;
	string postport;
	string msg;
	struct sockaddr_in si_me,si_other;
    clock_t PreviousTime= 0;    
    long pastTime;
    float RingTimeout = 2.0;
    double currentTime;

    queue<Post> Posts;
    ofstream Output;
    chrono::system_clock::time_point StartTime;
    chrono::system_clock::time_point searchNextPortTime;
    chrono::system_clock::time_point lastTokenTime;
    chrono::system_clock::time_point lastElectionTime ;
    chrono::system_clock::time_point RingFormedTime;
    chrono::system_clock::time_point LastPostTime;
    string TokenString;

    Client(string config,string input,string outputfile)
	{
		readConfig(config);
		readInput(input);
		Output.open(outputfile);
		init(PortNumber);
		slen = sizeof(si_other);
		StartTime = chrono::high_resolution_clock::now();            
	
		while(true)
		{
			currentTime = (double)std::chrono::duration_cast<std::chrono::seconds>(chrono::high_resolution_clock::now()- StartTime).count();
			pastTime = (long)currentTime;	

			memset(buffer,0,1024);
			recv_len = recvfrom(s, buffer, 1024, 0,  (struct sockaddr*)&si_other, (socklen_t * )&slen);

			if(currentTime > joinTime) //Start to work at joinTime
			{
				if(recv_len != -1)
				{
		        	TransferMessage(); //Received message and transfer it
		    	}

		    	if(NextHopPort == 0) //Initialize before find next;
		    	{
		    		SearchForPort();
		    	}
		    	else if(NextHopPort != PortNumber) //The ring has more than one node
		    	{
		    		
		    		if(!ringformed) //If no ring exists
		    		{
		    			CheckRing();
		    		}
		    		else //If ring exists
		    		{
		    			ElectionInitialization();
	    			}
	    		}
			}

			if(currentTime > stopTime)
			{
				break;
			}
		}
		
	};

	void readConfig(string fileName)
	{
		ifstream infile(fileName);
		char line[256];
		char* p;
		int min,sec;
		infile.getline(line,256)  ;                    
		p = strtok (line," -\t");
		p = strtok (NULL, " -\t");
		minPortNum = atoi(p);
		p = strtok (NULL, " -\t");
		maxPortNum = atoi(p);
		infile.getline(line,256);                          
		p = strtok (line," -\t");
		p = strtok (NULL, " -\t");
		PortNumber = atoi(p);
		infile.getline(line,256);                          
		p = strtok (line," :\t");
		p = strtok (NULL, " :\t");
		min = atoi(p);
		p = strtok (NULL, " :\t");
		sec = atoi(p);
		joinTime = 60*min +sec;
		infile.getline(line,256);                            
		p = strtok (line," :\t");
		p = strtok (NULL, " :\t");
		min = atoi(p);
		p = strtok (NULL, " :\t");
		sec = atoi(p);
		stopTime = 60*min +sec;
		infile.close();
		portRange = maxPortNum - minPortNum +1;
	}

	void readInput(string fileName)
	{
		ifstream infile(fileName);
		char line[1024];
		char* p;
		int min,sec;
		while(infile.getline(line,1024))
		{
			p = strtok (line," :\t");
			min = atoi(p);
			p = strtok (NULL, " \t");
			sec = atoi(p);
			p = strtok(NULL,"");
			auto text  = string(p);
			Posts.push({60*min+sec , text});
		}
		
		
		infile.close();
	}

	void TransferMessage()
	{
		msg = string(buffer);
    	if(msg == "SearchNextPort")
		{
			FindNextClientPort();
		}
			
		else if(msg == "AnswerNext")   
		{
			FindNextClientPortReply();
		}
		
		if(PreviousHopPort == ntohs(si_other.sin_port))
		{
			if(PortNumber == NextHopPort)
			{
				NextHopPort = 0;
				tempPortNum = PortNumber;
				tokenExist = false;
				postExist = false;
				token = false;
			}
    		if(string(msg.begin(),msg.begin()+11) == "RingFormed?")
    		{
    			if(msg == string("RingFormed?")+to_string(PortNumber))
    			{
    				ringformed = true;
    			}
    			else
    			{
    				if(NextHopPort!= 0 && NextHopPort != PortNumber)
    				{
    					SendMsgToPort(NextHopPort,buffer);
    				}
    			}
    		}
    		else if(string(msg.begin(),msg.begin()+8) == "Election")
    		{
    			token = false;
    			if(string(msg.begin(),msg.begin()+12) == "ElectionDown")
				{
					ElectionCompleted();
				}
				else
				{
        			ElectionComplete();
    			}

    		}
    		else if(string(msg.begin() , msg.begin() + 5) == "Token")
    		{
    			TransferToken();
    		}
    		else if(string(msg.begin(),msg.begin() + 4) == "Post")
    		{
    			TransferPost();
    		}
		}
	}

	void SearchForPort()
	{
	    if((double)std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - searchNextPortTime).count()> 0.5f)
		{
			//cout<<" now send to port "<<tempPortNum<<endl;
			++tempPortNum;
			if(tempPortNum == PortNumber)
			{
				NextHopPort = PortNumber;
				// continue;
			} else {
				tempPortNum = (tempPortNum - minPortNum ) % (maxPortNum + 1 - minPortNum) + minPortNum;
		 		SendMsgToPort(tempPortNum,"SearchNextPort");
				searchNextPortTime = Clock::now();
			}
		}
	}

	void FindNextClientPortReply() 
	{
    	NextHopPort = ntohs(si_other.sin_port);
		printf("[%d:%d]: next hop is changed to client [%d]\n",pastTime/60,pastTime%60,NextHopPort);
		Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: next hop is changed to client "<<"["<<NextHopPort<<"]"<<endl;
		RingFormedTime = Clock::now();
	}

	void FindNextClientPort() 
	{
		auto portfrom = ntohs(si_other.sin_port);
		if(!PreviousHopPort)
		{
			PreviousHopPort = portfrom;
			SendMsgToPort(PreviousHopPort,"AnswerNext");
			printf("[%d:%d]: previous hop is changed to client [%d]\n",pastTime/60,pastTime%60,PreviousHopPort);
			Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: previous hop is changed to client ["<<PreviousHopPort<<"]"<<endl;
			PreviousHopPortCheck = false;
			token = false;
		}
		else if(PreviousHopPortCheck)
		{
			PreviousHopPort = portfrom;
			SendMsgToPort(PreviousHopPort,"AnswerNext");
			printf("[%d:%d]: previous hop is changed to client %d\n",pastTime/60,pastTime%60,PreviousHopPort);
			Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: previous hop is changed to client ["<<PreviousHopPort<<"]"<<endl;
			ringformed = false;
			Elected = false;
			token = false;
			PreviousHopPortCheck = false;
		}
		else
		{
			if(((PortNumber - portfrom)%(maxPortNum+1 -minPortNum)) <= ((PortNumber - PreviousHopPort) % (maxPortNum - minPortNum +1)))
			{
				
				PreviousHopPort = portfrom;
				SendMsgToPort(PreviousHopPort,"AnswerNext");
				ringformed = false;
				token  = false;
				Elected = false;
				printf("[%d:%d]: previous hop is changed to client %d\n",pastTime/60,pastTime%60,PreviousHopPort);
				Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: prevoius hop is changed to client "<<PreviousHopPort<<endl;
    		}
		}
		
		
		if(NextHopPort == PortNumber)
		{
			NextHopPort = 0;
			tempPortNum = PortNumber;
			tokenExist = false;
			postExist = false;
			token = false;
		}
	}

	void ElectionComplete() 
	{
		auto port = atoi(string(msg.begin()+8,msg.end()).c_str());
		if(port == PortNumber)
		{
			Elected = true;
			SendMsgToPort(NextHopPort,"ElectionDown"+ to_string(PortNumber));
			printf("[%d:%d]: leader selected\n",pastTime/60,pastTime%60);
			printf("[%d:%d]: relayed election message, leader: client [%d]\n",pastTime/60,pastTime%60,port);
			Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: relayed election message, leader: client ["<<port<<"]"<<endl;
			Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: leader selected"<<endl;
		}
		else
		{
			if(!Elected)
			{
    			if(port < PortNumber)
    			{
    				coordinator = PortNumber;
    				SendMsgToPort(NextHopPort, "Election" + to_string(PortNumber) + ":" + to_string(electionRound));
    				printf("[%d:%d]: relayed election message, replaced leader\n",pastTime/60,pastTime%60);
    				Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: relayed election message, replaced leader"<<endl;
    			}
    			else
    			{
    				coordinator = port;
    				SendMsgToPort(NextHopPort,buffer);
    			}
			}
			else
			{
				SendMsgToPort(NextHopPort,"ElectionDown"+to_string(coordinator));
			}
		}
	}

	void ElectionCompleted()
	{
		electionRound = 0;
		port = atoi(string(msg.begin()+12,msg.end()).c_str());
		if(port != PortNumber)
		{
			coordinator = port;
			Elected = true;
			lastTokenTime = Clock::now();
			SendMsgToPort(NextHopPort,msg);
			printf("[%d:%d]: relayed election message, leader: client [%d] \n",pastTime/60,pastTime%60,port);
			Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: relayed election message, leader: client ["<<port<<"]"<<endl;
		}
		else
		{
			if(!tokenExist)
			{
				Elected = true;
				TokenString = "Token" + to_string(rand()%10001);
				token = true;

				SendMsgToPort(NextHopPort,TokenString);
    			token = false;
    			tokenExist = true;
    			lastTokenTime = Clock::now();
    			printf("[%d:%d]: new token %s generated\n",pastTime/60,pastTime%60 , string(TokenString.begin()+5,TokenString.end()).c_str());
    			Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: new token "<<string(TokenString.begin()+5,TokenString.end())<< " generated"<<endl;
			}
		}
	}

	void CheckRing()
	{
		if((double)std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - RingFormedTime).count() > portRange)
		{
			printf("ring is broken\n");
			Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: ring is broken"<<endl;
			NextHopPort = 0;
			tempPortNum = PortNumber;
			tokenExist = false;
			postExist = false;
			token = false;

		}
		else //Join ask, send msg to next node
		{
			SendMsgToPort(NextHopPort,string("RingFormed?")+to_string(PortNumber));
		}
	}

	void TransferToken() 
	{
    	tokenExist = true;

		token = true;
		TokenString = msg;
		auto t_number = atoi(string(msg.begin()+5,msg.end()).c_str());
		
		if(t_number != TokenNumber)
		{
			printf("[%d:%d]: token [%d] was received\n",pastTime/60,pastTime%60,t_number);
			Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: token ["<<t_number<<"] was received"<<endl;
		}

		if(ringformed && Elected)
		{
			lastTokenTime = Clock::now();

			if(Posts.size()&&Posts.front().Time < pastTime)
			{
				SendMsgToPort(NextHopPort,"Post:"+to_string(PortNumber)+":" + Posts.front().message);
				printf("[%d:%d]: post \"%s\" was sent \n" , pastTime/60,pastTime%60, Posts.front().message.c_str(),NextHopPort);
				Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: post \""<<Posts.front().message<<"\" was sent"<<endl;
				LastPostTime = Clock::now();
				postExist = true;
			}
			else
			{
				SendMsgToPort(NextHopPort,TokenString);
				if(t_number != TokenNumber)
				{	
					printf("[%d:%d]: token %s was sent to client [%d]\n" , pastTime/60,pastTime%60, string(TokenString.begin()+5,TokenString.end()).c_str(),NextHopPort);
					Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: token ["<< string(TokenString.begin()+5,TokenString.end()) <<"] was sent to client ["<<NextHopPort<<"]"<<endl;
				}
				TokenNumber = t_number;

				token = false;
			}
		}
	}

	void TransferPost()
	{
		if(msg == "Post:"+ to_string(PortNumber) +":" +Posts.front().message)
		{
			printf("[%d:%d]: post \"%s\" was delivered to all succuessfully\n",pastTime/60,pastTime%60,Posts.front().message.c_str());
			Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: post \""<< Posts.front().message<<"\" was delivered to all succuessfully"<<endl;
			RingTimeout = 2* (double)std::chrono::duration_cast<std::chrono::seconds>(chrono::high_resolution_clock::now()- LastPostTime).count();
			Posts.pop();
			if(Posts.size()&&Posts.front().Time < pastTime)
			{
				SendMsgToPort(NextHopPort,"Post:"+to_string(PortNumber)+":" + Posts.front().message);
				printf("%d:%d : post %s was sent\n" , pastTime/60,pastTime%60, msg.c_str(),Posts.front().message.c_str());
				Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: post \""<<Posts.front().message<<"\" was sent"<<endl;
				LastPostTime = Clock::now();
				postExist = true;
			}
			else
			{
				if(token)
				{
					SendMsgToPort(NextHopPort,TokenString);
					auto t_number = atoi(string(TokenString.begin()+5,TokenString.end()).c_str());
					if(t_number != TokenNumber)
					{	
						printf("[%d:%d]: token [%s] was sent to client [%d]\n" , pastTime/60,pastTime%60, string(TokenString.begin()+5,TokenString.end()).c_str(), NextHopPort);
						Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: token ["<< string(TokenString.begin()+5,TokenString.end()) <<"] was sent to client ["<<NextHopPort<<"]"<<endl;
						
					}
					TokenNumber = t_number;
					token = false;
				}
			}
		}
		else
		{
			if(!token)
			{
				SendMsgToPort(NextHopPort,msg);
				auto postcontent = string(msg.begin()+ IndexOfChar(msg,':',2) , msg.end());
				auto postport = string(msg.begin() + 5 , msg.begin()+ IndexOfChar(msg,':',2) -1);
				printf("[%d:%d]: post \"%s\" from client [%s] was relayed\n" , pastTime/60,pastTime%60, postcontent.c_str(),postport.c_str());
				Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: post \""<<postcontent<<"\" from client "<<"["<<postport << "] was relayed"<<endl;
				postExist = true;
			}
		}
	}

	int IndexOfChar(string input , char c , int ind) // find the index of the indth char c of input
	{
		int p = 0;
		for(int i = 0; i<input.size(); ++i)
		{
			if(input[i] == c)
			{
				++p;
			}
			if(p == ind)
			{
				return i+1;
			}
		}
		return 0;
	}


	string GetPostContent(string msg)
	{
		return string(msg.begin()+ IndexOfChar(msg,':',2) , msg.end());
	}

	void SendMsgToPort(int TargetPort,char* Msg)
	{
		si_other.sin_family = AF_INET;
    	si_other.sin_port = htons(TargetPort);
    	si_other.sin_addr.s_addr = htonl(INADDR_ANY);
    	auto sent = sendto(s, Msg, strlen(Msg), 0, (struct sockaddr*) &si_other, slen);
    	if(sent  == -1)
    	{
    		cerr<<"failed to send to "<<TargetPort << " from "<<PortNumber<<" with msg: " <<Msg<<endl;
    	}
	}


	string ConvertTime(int number)
	{
		if(number < 10)
		{
			return "0"+to_string(number);
		}
		else
		{
			return to_string(number);
		}
	}

	void SendMsgToPort(int TargetPort,string Msg)
	{
		si_other.sin_family = AF_INET;
    	si_other.sin_port = htons(TargetPort);
    	si_other.sin_addr.s_addr = htonl(INADDR_ANY);
    	auto sent = sendto(s, Msg.c_str(), strlen(Msg.c_str()), 0, (struct sockaddr*) &si_other, slen);
    	if(sent  == -1)
    	{
    		cerr<<"failed to send to "<<TargetPort << " from "<<PortNumber<<" with msg: " <<Msg<<endl;
    	}
	}

	void init(int port) //---------------------------
	{
		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if(s == -1)
		{
			cerr<< "cannot create socket at port"<<port<<endl;
			// die("socket");
			perror("error");
		    exit(-1);
		}
		memset((char *) &si_me, 0, sizeof(si_me));
    	si_me.sin_family = AF_INET;
    	si_me.sin_port = htons(port);
    	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    //bind socket to port
	    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
	    {
	    	cerr<<"cannot bind to port "<<port<<endl;
	        // die("bind");
			perror("bind");
		    exit(-1);	        
	    }
	    struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) 
		{
		    // die("error");
			perror("error");
		    exit(-1);		    
		}
		tempPortNum = PortNumber;
		srand (time(NULL));
	}

	void ElectionInitialization()
	{
		if(!Elected)
		{
			if((double)std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - lastElectionTime ).count() > 0.75f)
			{
				SendMsgToPort(NextHopPort,"Election"+to_string(PortNumber)+":"+to_string(electionRound));
				++electionRound;
				lastElectionTime = Clock::now();
				printf("[%d:%d]: started election, send election message to client [%d]\n",pastTime/60,pastTime%60,NextHopPort);
				Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: started election, send election message to client ["<< NextHopPort<<"]"<<endl;
				if(electionRound > 8)
				{
					printf("ring is broken\n");
					Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: ring is broken"<<endl;
					NextHopPort = 0;
					tempPortNum = PortNumber;
					tokenExist = false;
					postExist = false;
					token = false;
					PreviousHopPortCheck = true;
				 	ringformed = false;
				 	Elected = false;
				 	electionRound = 0;
				}
			}
		}
		else
		{
			if(((double)std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - lastTokenTime).count()  > RingTimeout && tokenExist)||((double)std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - LastPostTime).count()  > RingTimeout&& token))
			{
				printf("ring is broken\n");
				Output<<"["<<pastTime/60<<":"<<ConvertTime(pastTime%60)<<"]: ring is broken"<<endl;
				NextHopPort = 0;
				tempPortNum = PortNumber;
				tokenExist = false;
				postExist = false;
				token = false;
				PreviousHopPortCheck = true;

				Elected = false;
				ringformed = false;
				tokenExist = false;
			}
		}
	}



	~Client()
	{
		close(s);
		Output.close();
	}
};
