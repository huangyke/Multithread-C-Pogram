#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "pmessages.h"
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


#define NUM_THREADS  4
#define TCOUNT 10
#define COUNT_LIMIT 12
#define AUTH_STRING "thisistheauthenticationstring123456"
#define LINESZ 1024
#define SIZE_USERNAME (30)
#define SIZE_PASSWORD (30)
#define SIZE_SERVICENAME (30)
#define SIZE_SECRET (100)
#define SIZE_USERS (200)
#define SIZE_ACCOUNTS (10)
#define SIZE_SERVICES (3)

extern int errno; /* for printing error messages */

pthread_mutex_t print_lock;
pthread_cond_t count_threshold_cv;



struct account_t{
  char username[SIZE_USERNAME+1];
  char password[SIZE_PASSWORD+1];
};

struct service_t{
    char servicename[SIZE_SERVICENAME+1];
    char secret[SIZE_SECRET+1];
    char users[SIZE_USERS+1];
};

typedef struct {
    pthread_t server_thread_id;
    pthread_t ticket_thread_id;
    pthread_t auth_thread_id;
} thread_ids_t;

struct account_t accounts[SIZE_ACCOUNTS+1];
struct service_t services[SIZE_SERVICES+1];

//============================= OPERATIONS ===================================

/* Read the authentication file
 * interrupts.
 * RETURNS:  <0 - if an error occurs 
 *          >=0 - no error 
 */
int 
read_auth_file (char *filename ) {
        char buff[LINESZ];
	FILE *fp;
	const char *fname;
        char username[20];
        char password[20];
        int count=0;
        
	fname = filename;
	fp = fopen(fname, "r");
	if (!fp){
		fprintf(stderr, "reload_config: Failed to open file: %s\n", fname);
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return -1;
	}

        if (fp != NULL) {
            while (fgets (buff, LINESZ, fp)) {
                /* %[^:] is the same as %s meaning that read character until it gets to : delimiter*/
                sscanf (buff,"%[^:]:%s",username,password);                
                strcpy(accounts[count].username,username);
                strcpy(accounts[count].password,password);
                count ++;
            }
            fclose (fp);
        }   
/*
        int sizeOfAccountsArray = sizeof(accounts)/sizeof(accounts[0]);
        int i;
        for(i=0;i<sizeOfAccountsArray;i++)
        {
            fflush(stdout);
        }
*/
        
        return 1;
}

/* Read the authentication file
 * interrupts.
 * RETURNS:  <0 - if an error occurs 
 *          >=0 - no error 
 */
int 
read_ticket_file (char *filename ) {
        char buff[LINESZ];
	FILE *fp;
	const char *fname;
        char servicename[5];
        char secret[100];
        char users[200];
        int count=0;
        
	fname = filename;
	fp = fopen(fname, "r");
	if (!fp){
		fprintf(stderr, "reload_config: Failed to open file: %s\n", fname);
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return -1;
	}

        if (fp != NULL) {
            while (fgets (buff, LINESZ, fp)) {
                /* %[^:] is the same as %s meaning that read character until it gets to : delimiter*/
                sscanf (buff,"%[^:]:%[^:]:%s",servicename,secret,users);                

                strcpy(services[count].servicename, servicename);
                strcpy(services[count].secret, secret);
                strcpy(services[count].users , users);

                count ++;
            }
            fclose (fp);
        }
 
        return 1;
}

/* Check the username password given as string parameter 
 * and check if they exist in the accounts array
 * 
 * RETURNS:  0:username:0               - if it doesnt exist 
 *           1:username:auth_string     - if it exist
 */
char*
checkIfUserPassExist(char *input)
{
    char username[30];
    char password[30];
    static char output[65];//*output = (char *)malloc(sizeof(char)*60);

    sscanf (input,"%[^:]:%s",username,password); 

    
    /*get the size of accounts array*/
    int sizeOfAccountsArray = sizeof(accounts)/sizeof(accounts[0]);
    int i;
    for(i=0;i<sizeOfAccountsArray;i++)
    {
        if( strcmp(accounts[i].username, username) ==0 && strcmp(accounts[i].password, password) ==0)
        {
            sprintf(output, "1:%s:%s",username,AUTH_STRING);
            return output;
            break;
        }
    }
    sprintf(output, "0:%s:0",username);
    return output;
}

/* Process the message sent to ticket.
 * RETURNS:  0:username:servicename:0                           - if it doesn't exist
 *           1:username:servicename:service_secret_key          - if it exist
 */
char*
processTicketMessageReceived(char *input)
{
    char username[30];
    char servicename[30];
    char authstring[100];
    
    static char output[200]; //*output = (char *)malloc(sizeof(char)*200);

    sscanf (input,"%[^:]:%[^:]:%s",username,servicename,authstring); 
    if(strcmp(authstring,AUTH_STRING)==0)
    {
        /*get the size of accounts array*/
        int sizeOfArray = sizeof(services)/sizeof(services[0]);
        int i;
        for(i=0;i<sizeOfArray;i++)
        {
            if( strcmp(services[i].servicename, servicename) ==0 && strstr(services[i].users,username) != NULL )
            {
                sprintf(output, "1:%s:%s:%s",username,services[i].servicename,services[i].secret);
                return output;
                break;
            }
        }
    }
    sprintf(output, "0:%s:%s:0",username,servicename);
    return output; 
}

/* Processes the message that the service thread receives from the client.
 * PARAMETER: 
 * RETURNS:  0:username:servicename:0                           - if it doesn't exist
 *           1:username:servicename:service_secret_key          - if it exist
 */
char*
processServiceMessageReceived(char *input)
{
    char username[30];
    char servicename[30];
    char servicesecretkey[100];
    
    int parameter1;
    int parameter2;
    
    int flag;
    char ticketusername[30];
    char ticketservicename[30];
    char ticketservicesecretkey[100];
    
    static char output[50]; // *output = (char *)malloc(sizeof(char)*50);

    sscanf (input,"%[^:]:%[^:]:%d:%d:%d:%[^:]:%[^:]:%s",username,servicename,&parameter1,&parameter2,&flag,ticketusername,ticketservicename,ticketservicesecretkey); 
    

    /*get the size of accounts array*/
    int sizeOfArray = sizeof(services)/sizeof(services[0]);
    int i;
    for(i=0;i<sizeOfArray;i++)
    {
        if( strcmp(services[i].servicename, servicename) ==0 )
        {
            strcpy(servicesecretkey,services[i].secret);
            break;
        } 
    }

    if(strlen(servicesecretkey)>0 && 
        strcmp(username,ticketusername) == 0 &&
        strcmp(servicename,ticketservicename) == 0 &&
        strcmp(servicesecretkey,ticketservicesecretkey) == 0)
    {
        
        sprintf(output, "1:%s:%d %s %d",servicename,parameter1,servicename,parameter2);
        return output;
    }
    sprintf(output, "0:%s:0",servicename);
    return output; 
}
//============================= END OF OPERATIONS ===================================

//============================= THREADS ===================================

void
*auth_thread()
{
    pthread_t auth_thread_id;
    static char *processedMessage;
    static char *comeback;
    int size;
    while(1){
        auth_thread_id = pthread_self();
        if (receive_message( &auth_thread_id, &comeback, &size) == MSG_OK) {
                printf("Auth thread received    : %s\n", comeback);
                
                processedMessage = checkIfUserPassExist(comeback);
                free(comeback);
                printf("Auth --> Client         : %s\n", processedMessage );
                fflush(stdout);
                if (send_message_to_thread( auth_thread_id, processedMessage, strlen(processedMessage)+1) != MSG_OK) {
                        printf( "sending message from client to auth thread failed\n" );
                }
        }
    }
    
    pthread_exit(NULL);
}

void
*ticket_thread()
{
    pthread_t ticket_thread_id;
    static char *processedMessage;
    static char *comeback;
    int size;
    while(1){
        ticket_thread_id = pthread_self();
        if (receive_message( &ticket_thread_id, &comeback, &size) == MSG_OK) {
                printf("Ticket thread received    : %s\n", comeback);
                processedMessage = processTicketMessageReceived(comeback);
                free(comeback);
                printf("Ticket --> Client         : %s\n", processedMessage );
                fflush(stdout);
                if (send_message_to_thread( ticket_thread_id, processedMessage, strlen(processedMessage)+1) != MSG_OK) {
                        printf( "sending message from client to auth thread failed\n" );
                }
        }
    }
    pthread_exit(NULL);
}


void 
*server(void *t) 
{
    pthread_t server_thread_id;
    static char *processedMessage;
    static char *comeback;
    int size;
    while(1){ 
        server_thread_id = pthread_self();
        if (receive_message( &server_thread_id, &comeback, &size) == MSG_OK) {
                printf("Server thread received    : %s\n", comeback);
                processedMessage = processServiceMessageReceived(comeback);
                free(comeback);
                printf("Server --> Client         : %s\n", processedMessage );
                fflush(stdout);
                if (send_message_to_thread( server_thread_id, processedMessage, strlen(processedMessage)+1) != MSG_OK) {
                        printf( "sending message from client to auth thread failed\n" );
                }
        }
    }
    pthread_exit(NULL);
}

void 
*client(void *t) 
{
    char message[300];
    pthread_t client_thread_id = pthread_self();
    char flag[2];
    char username[SIZE_USERNAME+1];
    char authstring[200];
    int param1, param2;
    char servicename[SIZE_SERVICENAME+1];

    thread_ids_t *threadids = (thread_ids_t*)t;

    int size;

    for (;;) { 
        char buf[1000];
        char *comebackFromAuth, *comebackFromTicket, *comebackFromServer;
        comebackFromTicket = (char *)malloc(sizeof(char)*400);
        comebackFromServer = (char *)malloc(sizeof(char)*400);
        printf("Please enter username and password: e.g  username:password \n");

        fgets(buf, 1000, stdin);
        /* Send and receive from destination process 1 (without threads running, we'll receive on thread 1 in torch's pthread implementation. */
        printf( "Client --> Auth        : %s\n",buf );
        if (send_message_to_thread( threadids->auth_thread_id, buf, strlen(buf)+1) != MSG_OK) {
                printf( "sending message from client to auth thread failed\n" );
        }
        if (receive_message( &client_thread_id, &comebackFromAuth, &size) == MSG_OK) {
                pthread_mutex_lock(&print_lock);
                printf("Client thread received    : %s\n", comebackFromAuth);
                fflush(stdout);
                pthread_mutex_unlock(&print_lock);

                sscanf (comebackFromAuth,"%[^:]:%[^:]:%s",flag,username,authstring);
                free(comebackFromAuth);
/*
                printf("flag: %s   username: %s    authstring: %s \n", flag,username,authstring);
*/
                if(!strcmp(flag,"0")==0)
                { 
                    /* Get input from user again : operation parameter1 parameter2*/
                    printf("Please enter the operation: e.g  add 1 2 \n");
                    fgets(buf, 1000, stdin);
                    sscanf (buf,"%s %d %d",servicename,&param1,&param2);
/*
                    printf("servicename: %s   param1: %d   param2: %d \n",servicename,param1,param2);                  
*/
                }
                else{
                    /* if the flag is 0 then dont continue*/
                    continue;
                }

                sprintf(comebackFromTicket, "%s:%s:%s",username,servicename, AUTH_STRING );
                printf( "Client --> Ticket        : %s\n",comebackFromTicket );
                if (send_message_to_thread( threadids->ticket_thread_id, comebackFromTicket, strlen(comebackFromTicket)+1) != MSG_OK) {
                        printf( "sending message from client to ticket thread failed\n" );
                }
                if (receive_message( &client_thread_id, &comebackFromTicket, &size) == MSG_OK) {

                        pthread_mutex_lock(&print_lock);
                        printf("Client thread received    : %s\n", comebackFromTicket);
                        fflush(stdout);
                        pthread_mutex_unlock(&print_lock);

                        strcpy(message, comebackFromTicket);
                        free(comebackFromTicket);
                        
                        sprintf(comebackFromServer, "%s:%s:%d:%d:%s", username, servicename, param1, param2, message );

                        printf( "Client --> Server        : %s\n",comebackFromServer );
                        if (send_message_to_thread( threadids->server_thread_id, comebackFromServer, strlen(comebackFromServer)+1) != MSG_OK) {
                                printf( "sending message from client to server thread failed\n" );
                        }
                        if (receive_message( &client_thread_id, &comebackFromServer, &size) == MSG_OK) {
                                pthread_mutex_lock(&print_lock);
                                printf("Client thread received    : %s\n", comebackFromServer);
                                fflush(stdout);
                                pthread_mutex_unlock(&print_lock);
                                free(comebackFromServer);
                        }
                }
        }
    } /* end for */
    pthread_exit(NULL);
}
//============================= END OF THREADS =================================== 



int 
main(int argc, char *argv[])
{
    /* If the arguments are not entered correctly then exit the program with proper instruction */
    if ( argc != 3 )
    {
        printf("Please enter the proper argument i.e: %s users.txt tickets.txt \n", argv[0] );
    }
    else{
        int i;
        pthread_t threads[4];
        pthread_attr_t attr;
        thread_ids_t threadids;  /* creating threadids to store ids of all threads and pass it to the client thread*/
        int     count = 0;

        read_auth_file(argv[1]);
        read_ticket_file(argv[2]);


        /* Don't start if we can't get the message system working. */
        if (messages_init() == MSG_OK) {
            /* Initialize mutex and condition variable objects */
            pthread_mutex_init(&print_lock, NULL);
            pthread_cond_init (&count_threshold_cv, NULL);

            /* For portability, explicitly create threads in a joinable state */
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

            /* Creating threads in joinable state*/

            pthread_create(&threads[1], &attr, server, NULL); 
            threadids.server_thread_id = threads[1];
            pthread_create(&threads[2], &attr, auth_thread, NULL);
            threadids.auth_thread_id = threads[2];
            pthread_create(&threads[3], &attr, ticket_thread, NULL);   
            threadids.ticket_thread_id = threads[3];
            pthread_create(&threads[0], &attr, client, (void *)&threadids); 

            /* Wait for all threads to complete */
            for (i = 0; i < NUM_THREADS; i++) {
            pthread_join(threads[i], NULL);
            }
            printf ("Main(): Waited on %d threads. Final value of count = %d. Done.\n", 
                    NUM_THREADS, count);

            /* Clean up and exit */
            pthread_attr_destroy(&attr);
            pthread_mutex_destroy(&print_lock);
            pthread_cond_destroy(&count_threshold_cv);
            pthread_exit (NULL);
            /* Clean up the message system. */

            messages_end();
        }
        else
        {
            printf ("Message system doesn't work!\n");
        }
    }
    
    printf("Exiting the main program...\n");
    return 0;
}