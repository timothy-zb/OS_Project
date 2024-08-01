#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

// Define constants for maximum client name length and shared memory size
#define MAX_NAME_LENGTH     10
#define MAX_SHARED_MEM_SIZE 1024

// Define keys for IPC objects
#define CONNECT_CHANNEL_KEY              1000
#define CONNECT_CHANNEL_SHM_SIZE         1024
#define CONNECT_COMM_CHANNEL_START_KEY   2000

// Define constants for various request types and functions
#define CONNECTION_REQUESTED   			1000
#define CONNECTION_ACCEPTED     		1001
#define COMMAND_REQUESTED       		1002
#define COMMAND_PROCESSED       		1003
#define CONNECTIONREMOVAL_REQUESTED 	1004
#define CONNECTIONREMOVAL_ACCEPTED 		1005
#define COMM_CHANNEL_OFFSET         100

enum FUNCTION_ID {ARITH,EVENODD,ISPRIME,ISNEGATIVE};
enum ARITH_PERATION{PLUS,MINUS,MULT,DIV};
enum CONN_RES{FAIL,PASS};


// Define a struct for client requests
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
} CLIENT_REQUEST;

static int ClientCntr=0;

// Define a struct for connection requests
typedef struct  
{
   pthread_mutex_t Lock;		// Mutex to signal Server
   pthread_mutex_t Resp_Lock;	// Mutex to signal Client (ACK)
   int id; // Client ID			// Client ID
   char name[MAX_NAME_LENGTH];  // Client Name
   int shmid;					// SHM ID of the client
   int Status;					// Status of Connection Request
   int x;
   
} CONNECT_REQUEST;

volatile CLIENT_REQUEST  *ClientRequestPtr; // Pointer to shared memory for client requests
volatile CONNECT_REQUEST *ConnectRequestPtr; // Connection request object and pointer to it

// Function to set up communication channel with client
int SetupCommChannel()
{
	int shmid; 
	// Use client's shared memory ID to generate a key for the communication channel
	int ShmKEY = ftok(".", ConnectRequestPtr->shmid);
	// Get the shared memory ID for the communication channel
	shmid=shmget(ShmKEY, CONNECT_CHANNEL_SHM_SIZE, 0666);  
	
	if (shmid < 0) {
		printf("SetupCommChannel - shmget failed\n");
		exit(1);
	}	
	// Attach to the shared memory for the communication channel
	ClientRequestPtr= (CLIENT_REQUEST *)shmat(shmid,NULL,0);    
	
	if ((int) ClientRequestPtr == -1) {
          printf("SetupCommChannel - shmat failed\n");
          exit(1);
    }
	
	printf("Comm Channel setup sucessfully for Client \n");  
}

// Function to handle arithmetic operation requests
int Func_ArithOperation()
{
	int Res;
	char sign[10];
	printf("PID %d THREAD %d  %s %s %d \n",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);
	
	// Set the function ID for the request to arithmetic operation
	ClientRequestPtr->funcid=ARITH;
	
	// Prompt the user for the arithmetic operation to perform
	printf("Enter the Arithmetic operation to perform on the operands in the form + - / *: \n");
	scanf("%s", sign);
	//Feeding in the values arith
	if(sign[0] == '+')
	{
	    ClientRequestPtr->arith=PLUS;
	}
	else if(sign[0] =='-')
	{
	    ClientRequestPtr->arith=MINUS;
	}
	else if(sign[0] =='/')
	{
	    ClientRequestPtr->arith=DIV;
	}
	else if(sign[0] =='*')
	{
	    ClientRequestPtr->arith=MULT;
	}
	else
	{
	    printf("Error \n");
	    exit(0);
	}
	ClientRequestPtr->no_of_operands=2;
	printf("Enter the first operand : \n");
	scanf("%d",&ClientRequestPtr->operand1);
	printf("Enter the second operand : \n");
	scanf("%d",&ClientRequestPtr->operand2);
	
	ClientRequestPtr->Status = COMMAND_REQUESTED;
	
	Res= pthread_mutex_unlock(&ClientRequestPtr->Lock);
        Res= pthread_mutex_lock(&ClientRequestPtr->RespLock); 
    
	printf("Server  Processed  Command\n");
	printf("Result of Command %d \n",ClientRequestPtr->Result );
	
	printf("Sequnce No. of Client Command  %d \n",ClientRequestPtr->client_response_sequence_no );
	printf("Total Commands procesed by Server  %d \n",ClientRequestPtr->total_request_counter );
	
	return 0;
}

int Func_EvenOdd()
{
	int Res;
	printf("PID %d THREAD %d  %s %s %d \n",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);
	
	ClientRequestPtr->funcid=EVENODD;
	ClientRequestPtr->arith=PLUS;
	ClientRequestPtr->no_of_operands=1;
	
	printf("Enter the first operand : \n");
	scanf("%d",&ClientRequestPtr->operand1);
	
	ClientRequestPtr->Status = COMMAND_REQUESTED;
	Res= pthread_mutex_unlock(&ClientRequestPtr->Lock);
	
   	Res= pthread_mutex_lock(&ClientRequestPtr->RespLock);
    	
	printf("Server  Processed  Command\n");
	printf("Result of Command %d \n",ClientRequestPtr->Result );
	
	printf("Sequnce No. of Client Command  %d \n",ClientRequestPtr->client_response_sequence_no );
	printf("Total Commands procesed by Server  %d \n",ClientRequestPtr->total_request_counter );
	
	return 0;
}

int Func_IsPrime()
{
	int Res;
	printf("PID %d THREAD %d  %s %s %d \n",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);

	ClientRequestPtr->funcid=ISPRIME;
	ClientRequestPtr->arith=PLUS;
	ClientRequestPtr->no_of_operands=1;
	
	printf("Enter the first operand : \n");
	scanf("%d",&ClientRequestPtr->operand1);
	ClientRequestPtr->Status = COMMAND_REQUESTED;
	
	Res= pthread_mutex_unlock(&ClientRequestPtr->Lock);
	
   	Res= pthread_mutex_lock(&ClientRequestPtr->RespLock);
   	
	printf("Server  Processed  Command\n");
	printf("Result of Command %d \n",ClientRequestPtr->Result );
	
	printf("Sequnce No. of Client Command  %d \n",ClientRequestPtr->client_response_sequence_no );
	printf("Total Commands procesed by Server  %d \n",ClientRequestPtr->total_request_counter );
	return 0;
}

int Func_IsNegative()
{
	int Res;
	
	printf("PID %d THREAD %d  %s %s %d \n",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);

	ClientRequestPtr->funcid=ISNEGATIVE;
	ClientRequestPtr->arith=PLUS;
	ClientRequestPtr->no_of_operands=1;
	
	printf("Enter the first operand : \n");
	scanf("%d",&ClientRequestPtr->operand1);
	ClientRequestPtr->Status = COMMAND_REQUESTED;
	
	Res= pthread_mutex_unlock(&ClientRequestPtr->Lock);
	
   	Res= pthread_mutex_lock(&ClientRequestPtr->RespLock);
 	
	printf("Server  Processed  Command\n");
	printf("Result of Command %d \n",ClientRequestPtr->Result );
	
	printf("Sequnce No. of Client Command  %d \n",ClientRequestPtr->client_response_sequence_no );
	printf("Total Commands procesed by Server  %d \n",ClientRequestPtr->total_request_counter );
	
	return 0;
}

int RegisterClient()
{
	int shmid;  
	int Res;
	ClientCntr++;
	
	int ShmKEY = ftok(".", 'x');
	
	shmid=shmget(ShmKEY, CONNECT_CHANNEL_SHM_SIZE, 0666);  
	if (shmid < 0) {
		printf("RegisterClient - shmget failed\n");
		exit(1);
	}	
	printf("PID %d THREAD %d  %s %s %d \t",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);
	printf("Connect Channel Creation Sucessfull ID  is %d\n",shmid); 
	
	ConnectRequestPtr = (CONNECT_REQUEST*) shmat(shmid,NULL,0); 
	if ((int) ConnectRequestPtr == -1) {
          printf("RegisterClient - shmat failed\n");
          exit(1);
        }   

	ConnectRequestPtr->id=shmid;
	ConnectRequestPtr->Status = -99;
	
	//Inputting unique name for Client
	printf("Enter unique name for client \n");
	scanf("%s",ConnectRequestPtr->name);
	//strcpy(ConnectRequestPtr->name ,"Client");
	ConnectRequestPtr->Status = CONNECTION_REQUESTED;
	
	
	Res= pthread_mutex_unlock(&ConnectRequestPtr->Lock);
	
	Res= pthread_mutex_lock(&ConnectRequestPtr->Resp_Lock);
	while(1)
	{
		if (ConnectRequestPtr->Status == CONNECTION_ACCEPTED)
		{
			printf("PID %d THREAD %d  %s %s %d \t",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);
			printf(" Server accepted Connection \n");
			ConnectRequestPtr->Status = -99;
			break;
		}
		sleep(1);
	}
	SetupCommChannel();

	return 0;
}

// Function to unregister a client
int UnregisterClient()
{
    int Res;
	
    ConnectRequestPtr->Status = CONNECTIONREMOVAL_REQUESTED;
    Res= pthread_mutex_unlock(&ConnectRequestPtr->Lock);
	
	Res= pthread_mutex_lock(&ConnectRequestPtr->Resp_Lock);
	
  
	if(ConnectRequestPtr->Status==CONNECTIONREMOVAL_ACCEPTED)
	{
		// Free the shared memory associated with the client
		int shmid = ConnectRequestPtr->id;
		printf("PID %d THREAD %d  %s %s %d \t",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);
		printf("CONNECTIONREMOVAL_ACCEPTED \n");
		int result = shmctl(shmid, IPC_RMID, NULL);
		if (result == -1)
		{
			// Error occurred while freeing shared memory
			return -1;
		}
	}
    
    printf("PID %d THREAD %d  %s %s %d \t",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);
    printf("Unregistering Sucessfull \n");
    return 0;
}

int main()  
{  
	int choice;  
	printf("PID %d THREAD %d  %s %s %d \t",getpid(),pthread_self(), __FILE__,__FUNCTION__,__LINE__);
	printf(" Before Registeration\n");
	RegisterClient();
	printf(" After Registeration\n");
	
	while(1)
	{
		printf (" Enter the Operation\n");
		printf (" 1:  ARITH\n");
		printf (" 2:  EVENODD\n");
		printf (" 3:  ISPRIME\n");
		printf (" 4:  ISNEGATIVE\n");
		printf (" 5:  UNREGISTER and EXIT\n");
		scanf("%d", &choice);
		switch (choice)
		{
			case 1:
			Func_ArithOperation();
			break;
			case 2:
			Func_EvenOdd();
			break;
			case 3:
			Func_IsPrime();
			break;
			case 4:
			Func_IsNegative();
			break;
		}
		if(choice ==5)
		{
			UnregisterClient();
			break;
		}
	}		
}


