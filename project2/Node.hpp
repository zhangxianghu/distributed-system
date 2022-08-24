#include "Head.h"

class Client
{
public:
    int JoinTime;
    int LeaveTime;
    int PortNumber;
    int MaxClientPort;
    int MinClientPort;
    queue<Post> Posts;
    struct sockaddr_in si_me,si_other;
    int s, i, slen = sizeof(si_other) , recv_len;
    char buf[BUFLEN];
    bool Token;
    bool InRing;
    queue<string> Msgs;
    chrono_t BeginTime;
    bool IsLive = true;
    bool recvstoped = false;
    int NextPort = 0;
    int PreviousPort = 0;
    chrono_t FindNextPortClock;
    int LastPortSearch;
    bool ringformed = false;
    chrono_t LastTokenClock;
    bool Elected = false;
    int ElectSendRound =0;
    chrono_t LastElectionMsgClock ;
    int CandidateLeader = 0;
    bool IfAlreadyGotToken = false;
    bool IfAlreadyGotPost = false;
    clock_t PreviousTime= 0;
    chrono_t RingFormedTime;
    bool PreviousPortCanUseLessNumber = true;
    int TokenNumber = -1;
    bool InToken = false;
    float TokenRingBrokenTime = 2.0f;
    chrono_t LastPostTime;
    string TokenString;
    ofstream OutFile;
    int PortRange;
    long PastTime;

    Client(string configfile,string inputfile,string outputfile)
	{
		BeginTime = chrono::high_resolution_clock::now();               //as second
		InRing = false;

		readConfig(configfile);
		readInput(inputfile);
		OutFile.open(outputfile);
		init(PortNumber);
		cout<<JoinTime<<" "<<LeaveTime<<" "<<PortNumber <<endl;
		// init(myport);
		// thread t1(&Client::Receive,this);
		// thread t2(&Client::Act,this);
		// t1.join();
		// t2.join();
		while(1)
		{
			//cout<< TokenRingBrokenTime<<endl;
			//cout<<ringformed <<"   "<< Elected <<"    "<< IfAlreadyGotToken <<endl;

			auto PastTimed = (double)std::chrono::duration_cast<std::chrono::seconds>(chrono::high_resolution_clock::now()- BeginTime).count();
			PastTime = (long)PastTimed;		
			memset(buf,0,BUFLEN);
			recv_len = recvfrom(s, buf, BUFLEN, 0,  (struct sockaddr*)&si_other, (socklen_t * )&slen);

	        	// if(InRing)
	        	// {
			if(/*(double)(Clock::now() - BeginTime) / */PastTimed > JoinTime)
			{
				if(recv_len != -1)
				{
					//printf("Port %d ,Data : %s, RingFormed %d, Elected %d , NextPort %d ,PreviousPort %d\n",PortNumber, buf, ringformed, Elected,NextPort,PreviousPort);
		        	auto msg = string(buf);
		        	if(msg == "FindNextPort")
	        		{
						FindNextClientPort();
	        		}
	        			
	        		else if(msg == "ReplyFindNextPort")   
	        		{
	        			ReplyFindNextClientPort();
	        		}
	        		
	        		if(PreviousPort == ntohs(si_other.sin_port))
	        		{
	        			//LastTokenClock = Clock::now();
	        			if(PortNumber == NextPort)
	        			{
							NextPort = 0;
							LastPortSearch = PortNumber;
							IfAlreadyGotToken = false;
							IfAlreadyGotPost = false;
							InToken = false;
	        			}
		        		if(string(msg.begin(),msg.begin()+11) == "RingFormed?")
		        		{
		        			if(msg == string("RingFormed?")+to_string(PortNumber))
		        			{
		        				//PreviousTime = Clock::now();
		        				ringformed = true;
		        			}
		        			else
		        			{
		        				if(NextPort!= 0 && NextPort != PortNumber)
		        				{
		        					SendMsgToPort(NextPort,buf);
		        				}
		        			}
		        		}
		        		else if(string(msg.begin(),msg.begin()+8) == "Election")
		        		{
		        			InToken = false;
		        			if(string(msg.begin(),msg.begin()+12) == "ElectionDown")
							{
								ElectSendRound = 0;
								auto port = atoi(string(msg.begin()+12,msg.end()).c_str());
								if(port != PortNumber)
								{
									CandidateLeader = port;
									Elected = true;
									LastTokenClock = Clock::now();
									SendMsgToPort(NextPort,msg);
									printf("%d:%d : relayed election message, leader: client %d \n",PastTime/60,PastTime%60,port);
									OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": relayed election message, leader : client "<<port<<endl;
								}
								else
								{
									if(!IfAlreadyGotToken)
									{
										Elected = true;
										TokenString = "Token" + to_string(rand()%10001);
										InToken = true;
										/* send out the messages */


										SendMsgToPort(NextPort,TokenString);
					        			InToken = false;
					        			IfAlreadyGotToken = true;
					        			LastTokenClock = Clock::now();
					        			printf("%d:%d : New token %s generated\n",PastTime/60,PastTime%60 , string(TokenString.begin()+5,TokenString.end()).c_str());
					        			OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": New token "<<string(TokenString.begin()+5,TokenString.end())<< " generated"<<endl;
									}
								}
							}
							else
							{
								
			        			auto port = atoi(string(msg.begin()+8,msg.end()).c_str());
			        			if(port == PortNumber)
			        			{
			        				Elected = true;
			        				SendMsgToPort(NextPort,"ElectionDown"+ to_string(PortNumber));
			        				printf("%d:%d : leader selected\n",PastTime/60,PastTime%60);
			        				printf("%d:%d : relayed election message, leader: client %d\n",PastTime/60,PastTime%60,port);
			        				OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": relayed election message, leader: client "<<port<<endl;
			        				OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": leader selected"<<endl;
			        				
			        				//LastTokenClock = Clock::now();
			        			}
			        			else
			        			{
			        				if(!Elected)
									{
					        			if(port < PortNumber)
					        			{
					        				CandidateLeader = PortNumber;
					        				SendMsgToPort(NextPort,"Election"+to_string(PortNumber)+":"+to_string(ElectSendRound));
					        				printf("%d:%d : relayed election message, replaced leader\n",PastTime/60,PastTime%60);
					        				OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": relayed election message, replaced leader"<<endl;
					        			}
					        			else
					        			{
					        				CandidateLeader = port;
					        				SendMsgToPort(NextPort,buf);
					        			}
				        			}
				        			else
				        			{
				        				SendMsgToPort(NextPort,"ElectionDown"+to_string(CandidateLeader));
				        			}
			        			}
			        			
		        			}

		        		}
		        		else if(string(msg.begin() , msg.begin() + 5) == "Token")
		        		{
		        			IfAlreadyGotToken = true;

		        			InToken = true;
		        			TokenString = msg;
		        			auto t_number = atoi(string(msg.begin()+5,msg.end()).c_str());
		        			// if(TokenNumber != t_number)
		        			// {
		        			// 	cout<<"t_number "<<t_number<<endl;
		        			// }
		        			if(t_number != TokenNumber)
		        			{
		        				printf("%d:%d : Token %d was received\n",PastTime/60,PastTime%60,t_number);
		        				OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": Token"<<t_number<<" was received"<<endl;
		        			}

		        			if(ringformed && Elected)
		        			{
			        			LastTokenClock = Clock::now();
			        			/*

								do the things it needs to

			        			*/
			        			
			        			// while(Posts.size())
			        			// {
			        			// 	if(Posts.front().Time < PastTime)
			        			// 	{
			        			// 		//auto t = Posts.front().Time;
			        			// 		printf("%d:%d : %s\n",PastTime/60,PastTime%60,Posts.front().message.c_str());
			        			// 		Posts.pop();
			        			// 	}
			        			// 	else
			        			// 	{
			        			// 		break;
			        			// 	}
			        			// }
			        			// IfAlreadyGotToken =true;
			        			// SendMsgToPort(NextPort,msg);
			        			// InToken = false;

			        			//printf("%d:%d : Token %s was send to client %d\n" ,PastTime /60 , PastTime%60, string(msg.begin()+5, msg.end()).c_str(),NextPort);
		        				if(Posts.size()&&Posts.front().Time < PastTime)
		        				{
		        					SendMsgToPort(NextPort,"Post:"+to_string(PortNumber)+":" + Posts.front().message);
		        					printf("%d:%d : Post %s was sent\n" , PastTime/60,PastTime%60, Posts.front().message.c_str(),NextPort);
		        					OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": Post "<<Posts.front().message<<" was sent"<<endl;
		        					LastPostTime = Clock::now();
		        					IfAlreadyGotPost = true;
		        				}
		        				else
		        				{
		        					SendMsgToPort(NextPort,TokenString);
		        					if(t_number != TokenNumber)
		        					{	
		        						printf("%d:%d : Token %s was sent\n" , PastTime/60,PastTime%60, string(TokenString.begin()+5,TokenString.end()).c_str());
		        						OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": Token "<< string(TokenString.begin()+5,TokenString.end()) <<" was sent"<<endl;
		        					}
		        					TokenNumber = t_number;
		        					//printf("%d:%d : Token %s sent to %d\n" , PastTime/60,PastTime%60, string(TokenString.begin()+5,TokenString.end()).c_str(),NextPort);
		        					InToken = false;
		        				}
		        			}
		        			
		        		}
		        		else if(string(msg.begin(),msg.begin() + 4) == "Post")
		        		{
		        			//auto t_number = atoi(string(msg.begin()+5,msg.end()).c_str());
		        			//cout<<"t_number "<<t_number<<endl;
		        			if(msg == "Post:"+ to_string(PortNumber) +":" +Posts.front().message)
		        			{
		        				printf("%d:%d : Post %s was delivered to all succuessfully\n",PastTime/60,PastTime%60,Posts.front().message.c_str());
		        				OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": Post "<< Posts.front().message<<" was delivered to all succuessfully"<<endl;
		        				TokenRingBrokenTime = 2* (double)std::chrono::duration_cast<std::chrono::nanoseconds>(chrono::high_resolution_clock::now()- LastPostTime).count() / BILLION;
		        				Posts.pop();
		        				if(Posts.size()&&Posts.front().Time < PastTime)
		        				{
		        					SendMsgToPort(NextPort,"Post:"+to_string(PortNumber)+":" + Posts.front().message);
		        					printf("%d:%d : Post %s was sent\n" , PastTime/60,PastTime%60, msg.c_str(),Posts.front().message.c_str());
		        					OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": Post "<<Posts.front().message<<" was sent"<<endl;
		        					LastPostTime = Clock::now();
		        					IfAlreadyGotPost = true;
		        				}
		        				else
		        				{
		        					if(InToken)
		        					{
		        						SendMsgToPort(NextPort,TokenString);
		        						auto t_number = atoi(string(TokenString.begin()+5,TokenString.end()).c_str());
		        						if(t_number != TokenNumber)
		        						{	
		        							printf("%d:%d : Token %s was sent\n" , PastTime/60,PastTime%60, string(TokenString.begin()+5,TokenString.end()).c_str());
		        							OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": Token "<< string(TokenString.begin()+5,TokenString.end()) <<"was sent"<<endl;
		        							
		        						}
		        						TokenNumber = t_number;
		        						InToken = false;
		        					}
		        				}
		        				

		        			}
		        			else
		        			{
		        				if(!InToken)
		        				{
		        					SendMsgToPort(NextPort,msg);
		        					//printf("message %s sent to port %d\n",msg.c_str(),NextPort);
		        					auto postcontent = string(msg.begin()+ IndexOfChar(msg,':',2) , msg.end());
		        					auto postport = string(msg.begin() + 5 , msg.begin()+ IndexOfChar(msg,':',2) -1);
		        					printf("%d:%d : post %s from client %s was relayed\n" , PastTime/60,PastTime%60, postcontent.c_str(),postport.c_str());
		        					OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": post "<<postcontent << " from client "<<postport << " was relayed"<<endl;
		        					IfAlreadyGotPost = true;
		        				}
		        			}
		        		}
	        		}

		    	}








/* then determine some thing to send */





		    	if(NextPort == 0) //Initialize before find next;
		    	{
		    		if((double)std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - FindNextPortClock).count()> 0.5f)
		    		{
		    			//cout<<" now send to port "<<LastPortSearch<<endl;
		    			++LastPortSearch;
		    			if(LastPortSearch == PortNumber)
		    			{
		    				NextPort = PortNumber;
		    				continue;
		    			}
		    			LastPortSearch = (LastPortSearch - MinClientPort ) % (MaxClientPort + 1 - MinClientPort) + MinClientPort;
		    		 	SendMsgToPort(LastPortSearch,"FindNextPort");
		    			FindNextPortClock = Clock::now();
		    		}
		    		
		    	}
		    	else if(NextPort != PortNumber) //The ring has more than one Client
		    	{
		    		
		    		if(!ringformed)
		    		{
		    			if((double)std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - RingFormedTime).count() > PortRange)
		    			{
		    				//printf("%d:%d : NextPort set to 0\n",PastTime/60, PastTime%60);
		    				printf("ring is boken\n");
		    				OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": ring is broken"<<endl;
							NextPort = 0;
							LastPortSearch = PortNumber;
							IfAlreadyGotToken = false;
							IfAlreadyGotPost = false;
							InToken = false;

		    			}
		    			else //Join ask, send msg to next Client
		    			{
		    				SendMsgToPort(NextPort,string("RingFormed?")+to_string(PortNumber));
		    			}
		    		}
		    		else
		    		{
		    			// if((double)(Clock::now() - PreviousTime)/CLOCKS_PER_SEC > 2.0f)
		    			// {
		    			// 	cout<<"runovertheprevioustime"<<endl;
		    			// 	NextPort = 0; // it is too long time didn't get message after ringformed
		    			// 	PreviousPortCanUseLessNumber = true;
		    			// 	ringformed = false;
		    			// 	Elected = false;
		    			// }
		    			if(!Elected)
		    			{
		    				if((double)std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - LastElectionMsgClock ).count() > 0.75f)
		    				{
		    					SendMsgToPort(NextPort,"Election"+to_string(PortNumber)+":"+to_string(ElectSendRound));
		    					++ElectSendRound;
		    					LastElectionMsgClock = Clock::now();
		    					printf("%d:%d : started election ,send election message to client %d\n",PastTime/60,PastTime%60,NextPort);
		    					OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": started election, send election message to client"<< NextPort<<endl;
		   						if(ElectSendRound > 8)
		   						{
		   							printf("ring is boken\n");
		   							OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": ring is broken"<<endl;
									NextPort = 0;
									LastPortSearch = PortNumber;
									IfAlreadyGotToken = false;
									IfAlreadyGotPost = false;
									InToken = false;
				    				PreviousPortCanUseLessNumber = true;
				    			 	ringformed = false;
				    			 	Elected = false;
				    			 	ElectSendRound = 0;
		   						}
		    				}
		    				//Elected = true;
		    			}
		    			else
		    			{
		    				if(((double)std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - LastTokenClock).count()  > TokenRingBrokenTime && IfAlreadyGotToken)||((double)std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - LastPostTime).count()  > TokenRingBrokenTime&& InToken))
		    				{
		    					//cout<< std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - LastTokenClock).count()<<endl;
		    					//cout<<PortNumber<<endl;
		    					// new Client inserted
		    					printf("ring is boken\n");
		    					OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": ring is broken"<<endl;
								NextPort = 0;
								LastPortSearch = PortNumber;
								IfAlreadyGotToken = false;
								IfAlreadyGotPost = false;
								InToken = false;
		    					PreviousPortCanUseLessNumber = true;

		    					Elected = false;
		    					ringformed = false;
		    					IfAlreadyGotToken = false;
		    				}
		    			}



	    			}
	    		}
			}

			if(PastTimed > LeaveTime)
			{
				break;
			}
		}
		
	};

	void ReplyFindNextClientPort() {
		NextPort = ntohs(si_other.sin_port);
		printf("%d:%d : next hop is changed to client %d\n",PastTime/60,PastTime%60,NextPort);
		OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": next hop is changed to client "<<NextPort<<endl;
		RingFormedTime = Clock::now();
	}

	void FindNextClientPort() {
		auto portfrom = ntohs(si_other.sin_port);
						if(!PreviousPort)
						{
							PreviousPort = portfrom;
							SendMsgToPort(PreviousPort,"ReplyFindNextPort");
							printf("%d:%d : previous hop is changed to client %d\n",PastTime/60,PastTime%60,PreviousPort);
							OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": previous hop is changed to "<<PreviousPort<<endl;
							PreviousPortCanUseLessNumber = false;
							InToken = false;
						}
						else if(PreviousPortCanUseLessNumber)
						{
							PreviousPort = portfrom;
							SendMsgToPort(PreviousPort,"ReplyFindNextPort");
							printf("%d:%d : previous hop is changed to client %d\n",PastTime/60,PastTime%60,PreviousPort);
							OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": previous hop is changed to client "<<PreviousPort<<endl;
							ringformed = false;
	        				Elected = false;
	        				InToken = false;
							PreviousPortCanUseLessNumber = false;
						}
						else
						{
							if(((PortNumber - portfrom)%(MaxClientPort+1 -MinClientPort)) <= ((PortNumber - PreviousPort) % (MaxClientPort - MinClientPort +1)))
		        			{
		        				
		        				PreviousPort = portfrom;
								SendMsgToPort(PreviousPort,"ReplyFindNextPort");
		        				ringformed = false;
		        				InToken  = false;
		        				Elected = false;
		        				printf("%d:%d : previous hop is changed to client %d\n",PastTime/60,PastTime%60,PreviousPort);
		        				OutFile<<PastTime/60<<":"<<To2DigitTime(PastTime%60)<<": prevoius hop is changed to client "<<PreviousPort<<endl;
		            		}
						}
	        			
	        			
	        			if(NextPort == PortNumber)
	        			{
	        				NextPort = 0;
							LastPortSearch = PortNumber;
							IfAlreadyGotToken = false;
							IfAlreadyGotPost = false;
							InToken = false;
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


	string To2DigitTime(int number)
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
		LastPortSearch = PortNumber;
		srand (time(NULL));
	}

	void readConfig(string filename)
	{
		ifstream infile(filename);
		char line[256];
		char* p;
		int min,sec;
		infile.getline(line,256)  ;                    // port range
		p = strtok (line," -\t");
		p = strtok (NULL, " -\t");
		MinClientPort = atoi(p);
		p = strtok (NULL, " -\t");
		MaxClientPort = atoi(p);
		infile.getline(line,256);                          //my port
		p = strtok (line," -\t");
		p = strtok (NULL, " -\t");
		PortNumber = atoi(p);
		infile.getline(line,256);                          // join time
		p = strtok (line," :\t");
		p = strtok (NULL, " :\t");
		min = atoi(p);
		p = strtok (NULL, " :\t");
		sec = atoi(p);
		JoinTime = 60*min +sec;
		infile.getline(line,256);                            // leave time
		p = strtok (line," :\t");
		p = strtok (NULL, " :\t");
		min = atoi(p);
		p = strtok (NULL, " :\t");
		sec = atoi(p);
		LeaveTime = 60*min +sec;
		//cout<< MinClientPort <<" "<<MaxClientPort<<" "<<PortNumber<<" "<<JoinTime<<" " <<LeaveTime<<endl;
		infile.close();
		PortRange = MaxClientPort - MinClientPort +1;
	}



	void readInput(string filename)
	{
		ifstream infile(filename);
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
		// for(auto &i : Posts)
		// {
		// 	cout<<i.Time <<" "<<i.message<<endl;
		// }
	}

	~Client()
	{
		close(s);
		OutFile.close();
	}
};
