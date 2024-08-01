#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>


#define  CONNECTION_REQUESTED    		1000
#define  CONNECTION_ACCEPTED     		1001
#define  COMMAND_REQUESTED       		1002
#define  COMMAND_PROCESSED       		1003
#define  CONNECTIONREMOVAL_REQUESTED 	1004
#define  CONNECTIONREMOVAL_ACCEPTED 	1005
#define  COMM_CHANNEL_OFFSET         	100

#define MAX_CLIENTS 10 			// Maximum number of clients that can register
#define MAX_NAME_LENGTH 10 		// Maximum length of client names
#define MAX_SHARED_MEM_SIZE 1024 // Maximum size of shared memory for each client

#define CONNECT_CHANNEL_KEY              1000
#define CONNECT_CHANNEL_SHM_SIZE         1024
#define CONNECT_COMM_CHANNEL_START_KEY   2000

enum FUNCTION_ID {ARITH,EVENODD,ISPRIME, ISNEGATIVE};
enum ARITH_PERATION{PLUS,MINUS,MULT,DIV};
enum CONN_RES{FAIL,PASS};

// Struct to store client information
typedef struct {
    int id; 					// Client ID
    char name[MAX_NAME_LENGTH]; // Client Name
    int comm_channel; 			// Communication Channel for client
	pthread_t ptid; 			// Thread ID of the client
	int shmid;					// SHM ID of the client
	int ClientCntr;				// Internal counte for Client
	int request_counter; 		// Request Counter for client
} CLIENT_INFO;

typedef struct  
{
   int id;						// Client ID
   char name[MAX_NAME_LENGTH];	// Client Name
   pthread_mutex_t Lock;		// Mutex to signal Server
   pthread_mutex_t RespLock;	// Mutex to signal Client (ACK)
   int Status;					// Status of the request
   enum FUNCTION_ID  funcid;	// Request Function ID
   enum ARITH_PERATION  arith;	// Request Operation Type
   int no_of_operands;			// No. of Operands
   int operand1;				
   int operand2;
   int operand3;
   int operand4;
   int operand5;
   int Result;					// Result of the Operation
   int client_response_sequence_no;	// Total requests from this client 
   int total_request_counter;	// Total Requests handled by the server
}CLIENT_REQUEST;

typedef struct  
{
   pthread_mutex_t Lock;		// Mutex to signal Server
   pthread_mutex_t Resp_Lock;	// Mutex to signal Client (ACK)
   int id; // Client ID			// Client ID
   char name[MAX_NAME_LENGTH];  // Client Name
   int shmid;					// SHM ID of the client
   int Status;					// Status of Connection Request
   int x;
}CONNECT_REQUEST;

//CLIENT_REQUEST Client_Req[MAX_CLIENTS];

volatile CLIENT_REQUEST *pClientRequest;
volatile CONNECT_REQUEST *ConnectRequestPtr;

CLIENT_INFO Client_Info[MAX_CLIENTS];
static int ClientCntr=0;
// Total Requests handled by the Server
int total_request_counter;
char nameinit[MAX_NAME_LENGTH]={'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};

int Func_ArithOperation(CLIENT_REQUEST *pClientRequest)
{
	//printf("Recived Command %d %d %d %d %d \n",pClientRequest->funcid,pClientRequest->arith,pClientRequest->no_of_operands,pClientRequest->operand1,pClientRequest->operand2);
	
	if(pClientRequest->arith==PLUS)
	{
	    pClientRequest->Result = pClientRequest->operand1 + pClientRequest->operand2;
	}
	else if(pClientRequest->arith==MINUS)
	{
	    pClientRequest->Result = pClientRequest->operand1 - pClientRequest->operand2;
	}
	else if(pClientRequest->arith==MULT)
	{
	    pClientRequest->Result = pClientRequest->operand1 * pClientRequest->operand2;
	}
	else if(pClientRequest->arith==DIV)
	{
	    pClientRequest->Result = pClientRequest->operand1 / pClientRequest->operand2;
	}
	
	pClientRequest->Status = COMMAND_PROCESSED;
	printf("Result of Command %d \n",pClientRequest->Result );
	
	return 0;
}

int Func_EvenOdd(CLIENT_REQUEST *pClientRequest)
{
	pClientRequest->Result = (pClientRequest->operand1)%2;

	if(pClientRequest->Result==1)
	{
	    printf("Result of Command: ODD \n");
	}
	else
	{
	    printf("Result of Command: EVEN \n");
	}
	
	pClientRequest->Status = COMMAND_PROCESSED;

	return 0;
}

int Func_IsPrime(CLIENT_REQUEST *pClientRequest)
{
	
	
    int i;
    for(i = 2; i < pClientRequest->operand1; i++) {
        if(pClientRequest->operand1 % i == 0) {
            pClientRequest->Result = 0;
            break;
        }
    }
    if(i == pClientRequest->operand1) {
        pClientRequest->Result = 1;
    }
   
    if(pClientRequest->Result == 1) {
        printf("The given no. is PRIME \n");
    } else {
        printf("The given no. is COMPOSITE \n");
    }
	pClientRequest->Status = COMMAND_PROCESSED;
    return 0;
	
}

int Func_IsNegative(CLIENT_REQUEST *pClientRequest)
{
	
	if(pClientRequest->operand1<0)
	{
	    pClientRequest->Result=-1;
	    printf("The given no is negative \n");
	}
	else
	{
	    pClientRequest->Result=0;
	    printf("The given no is positive \n");
	}
	
	pClientRequest->Status = COMMAND_PROCESSED;
	
	return 0;
}

int PrintStatistics()
{
    int i;
    printf("The list of registered clients are: \n");
    for(i=0;i<=ClientCntr-1;i++)
    {
        if(Client_Info[i].ClientCntr!=0)
        {
        	printf("Client no . %d is %s \n",i+1,Client_Info[i].name);
        	printf("Total No of requests serviced for this client is :%d\n"	,Client_Info[i].request_counter);
        }
    }
    printf("Total no. of requests serviced for all clients is %d \n",total_request_counter);
    printf("*******************************\n");
}

void * ClientHandler(void *args)
{
	int *argptr = (int *) args;
	int client_id = *argptr;
	pthread_mutexattr_t attrmutex;
	
    CLIENT_REQUEST *ClientRequestPtr;
	pthread_t threadid = pthread_self();

	int pid=getpid();
	
	printf("PID %d THREAD %d  %s %s %d \n",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);
	ClientRequestPtr =  (CLIENT_REQUEST *)shmat(Client_Info[client_id].shmid,NULL,0);    
	
	if ((int) ClientRequestPtr == -1) {
          printf("PID %d THREAD %d  %s %s %d  shmat failed\n",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);
          exit(1);
    }
	
	pthread_mutexattr_init(&attrmutex);
	pthread_mutexattr_setpshared(&attrmutex,PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&ClientRequestPtr->Lock,&attrmutex);
	pthread_mutex_lock(&ClientRequestPtr->Lock);
    pthread_mutex_init(&ClientRequestPtr->RespLock,&attrmutex);
	pthread_mutex_lock(&ClientRequestPtr->RespLock);
	
	bool cond=true;      
	
	while(cond)
	{
		
		pthread_mutex_lock(&ClientRequestPtr->Lock);
		printf("*******************************\n");
		printf("PID %d THREAD %d  %s %s %d ",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);
    	printf ("Received new command %d from Client %d \n",ClientRequestPtr->funcid,client_id);
		
        Client_Info[client_id].request_counter++;
        total_request_counter++;
		
  		switch(ClientRequestPtr->funcid)
 		{
			case ARITH:
				Func_ArithOperation(ClientRequestPtr);			
				break;
			case EVENODD:
				Func_EvenOdd(ClientRequestPtr);
				break;
			case ISPRIME:
				Func_IsPrime(ClientRequestPtr);
				break;
			case ISNEGATIVE:
				Func_IsNegative(ClientRequestPtr);
				break;
			default:
				printf(")Unsupported Function\n");
				break;
  		}
		
		PrintStatistics();
		
		
		
		ClientRequestPtr->client_response_sequence_no =Client_Info[client_id].request_counter;
		ClientRequestPtr->total_request_counter =total_request_counter;
		
  		ClientRequestPtr->Status == COMMAND_PROCESSED;
  		pthread_mutex_unlock(&ClientRequestPtr->RespLock);
		
  	}
}

int SetupCommChannel(int Client_Counter)
{
    // Creating a new comm channel to handle the new client
	int shmid;
	int ShmKEY = ftok(".", Client_Counter+COMM_CHANNEL_OFFSET);
	//Creating a shared memory for every client
	shmid = shmget(ShmKEY,CONNECT_CHANNEL_SHM_SIZE, 0666|IPC_CREAT);
	
	if (shmid < 0) {
		printf("SetupCommChannel - shmget failed\n");
		exit(1);
	}	
	Client_Info[Client_Counter].shmid =shmid;
	Client_Info[Client_Counter].comm_channel = 	Client_Info[Client_Counter].shmid;  
}

//Creating Client Handler
int CreateClientHandler(int Client_Counter)
{
    
	int Res;
	//Setup Communication Channel
	SetupCommChannel(Client_Counter);
	Client_Info[Client_Counter].ClientCntr=Client_Counter;
	// Creating a new thread to handle the new client
	Res= pthread_create(&Client_Info[Client_Counter].ptid, NULL, &ClientHandler, &Client_Info[Client_Counter].ClientCntr);
	if(Res)
		printf("Failed to create communcation channel due to pthread create failure\n");
	ConnectRequestPtr->shmid=Client_Counter+COMM_CHANNEL_OFFSET;
	ConnectRequestPtr->Status=CONNECTION_ACCEPTED;
	printf("PID %d THREAD %d  %s %s %d \t",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);
	printf("Communication Channel Created for Client %s \n",Client_Info[Client_Counter].name);
}


int UnRegisterClient()
{
	int i;
	
    printf("UnRegisterClient\n");
    for(i=0;i<ClientCntr;i++)
    {
        if(ConnectRequestPtr->id=Client_Info[i].id)
        {
            printf("Found the client to be unregistered \n");
			
			Client_Info[i].comm_channel = 0;
			Client_Info[i].ClientCntr = 0;
			//strcpy(Client_Info[i].name,nameinit);
			Client_Info[i].request_counter= 0;
			
		#if 1
			int result = pthread_cancel(Client_Info[i].ptid );
			if (result == -1)
            {
            	printf("Error occurred while cancelling  Comm thread  for Client %d\n",i);
            	return -1;
            }
			Client_Info[i].ptid = 0;
			
            result = shmctl(Client_Info[i].shmid , IPC_RMID, NULL);
            if (result == -1)
            {
            	printf("Error occurred while freeing Comm channel memory for Client %d\n",i);
            	return -1;
            }
			Client_Info[i].shmid   = 0;
		#endif
			ConnectRequestPtr->shmid=Client_Info[i].ClientCntr+COMM_CHANNEL_OFFSET;
			ConnectRequestPtr->Status=CONNECTIONREMOVAL_ACCEPTED;
            return 0;
        }
    }
	return 0;
   
}

int ValidateClientName()
{
    //Checking if name is unique
    int i;
    
    for(i=0;i<ClientCntr;i++)
    {
        if(strcmp(ConnectRequestPtr->name,Client_Info[i].name)==0)
        {
            printf("Client Name already exists, hence not creating new one \n");
			ConnectRequestPtr->shmid=Client_Info[i].ClientCntr+COMM_CHANNEL_OFFSET;
			ConnectRequestPtr->Status=CONNECTION_ACCEPTED;
            return 0;
        }
    }
    //Storing new Client
    strcpy(Client_Info[ClientCntr].name,ConnectRequestPtr->name);
    return 1;
}

//Function to handle connection
int ConnectHandler()
{
	bool cond=true;  
	//printf("Inside Connect Handler ClientCntr%d\n",ClientCntr);
	
	while(cond)
	{
	    	int res;
		printf("Waiting for Client Connection \n");
		res=pthread_mutex_lock(&ConnectRequestPtr->Lock); 

		if(ConnectRequestPtr->Status == CONNECTION_REQUESTED)
		{
		printf("PID %d THREAD %d  %s %s %d \t",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);
		    printf("Connect Request received from  Clinet %s \n",ConnectRequestPtr->name);
		
		   res = ValidateClientName();
		    if(res==1)
		    {
		        //Creating Client Handler
		        CreateClientHandler(ClientCntr);
		        //Increace the no of clients
		        ClientCntr++;
	    	}
		}
		else if(ConnectRequestPtr->Status == CONNECTIONREMOVAL_REQUESTED)
		{
		printf("PID %d THREAD %d  %s %s %d \t",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);
		    printf("Unregister Request received from  Clinet %s \n",ConnectRequestPtr->name);
	    	UnRegisterClient();
			
		}
		
		res=pthread_mutex_unlock(&ConnectRequestPtr->Resp_Lock);
	}
}

//Function to Setup connection channel      
int SetupConnectionChannel()
{
    //Creating key
	int ShmKEY = ftok(".", 'x');
	int shmid;
	//declaring mutex attribute object
	pthread_mutexattr_t attrmutex;
	
    //Creating and attaching shared memory
	shmid=shmget(ShmKEY, CONNECT_CHANNEL_SHM_SIZE, 0666|IPC_CREAT); 
	if (shmid < 0) {
		printf("SetupConnectionChannel - shmget failed\n");
		exit(1);
	}	
	//printf("Connection Channel Created Sucessfully ID  is %d\n",shmid);  
	ConnectRequestPtr=(CONNECT_REQUEST*) shmat(shmid,NULL,0);  
	
	if ((int) ConnectRequestPtr == -1) {
          printf("SetupConnectionChannel - shmat failed\n");
          exit(1);
        }  
	 
	ConnectRequestPtr->Status = -99;
	//declaring mutex attribute object
	pthread_mutexattr_init(&attrmutex);
	//Sets the mutex to shared among processes
	pthread_mutexattr_setpshared(&attrmutex,PTHREAD_PROCESS_SHARED);
	//initializing mutex attribute object
	pthread_mutex_init(&ConnectRequestPtr->Lock,&attrmutex);
	//printf("Connect Channel Mutex Inited %d \n",ConnectRequestPtr->Lock);
	//Locking the mutex 
	pthread_mutex_lock(&ConnectRequestPtr->Lock);
	//printf("Connect Channel  Mutext Locked %d \n",ConnectRequestPtr->Lock);
	
	pthread_mutex_init(&ConnectRequestPtr->Resp_Lock,&attrmutex);
	pthread_mutex_lock(&ConnectRequestPtr->Resp_Lock);
	printf("PID %d THREAD %d  %s %s %d \t",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);
	printf("Connection Channel Created Sucessfully \n"); 
	return 0;
}

int WaitforClient()
{
	int i=0;
	for (i=0;i<ClientCntr;i++)
	{
		//Waiting for the created thread to terminate
		pthread_join(Client_Info[ClientCntr].ptid, NULL);
		printf("Closing thread for Client %d \n",ClientCntr);   
		pthread_exit(NULL);
	}
}

int main()  
{  
	//Function to setup connection handler
    SetupConnectionChannel();
	//Function to handle connection
	ConnectHandler();
    //Function to wait for client
	WaitforClient();
	//Function to Remove a client
}


