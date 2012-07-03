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

extern int errno; /* for printing error messages */

int     count = 0;
pthread_mutex_t print_lock;
pthread_cond_t count_threshold_cv;
pthread_t server_thread_id;
pthread_t client_thread_id;
pthread_t ticket_thread_id;
pthread_t auth_thread_id;

struct account_t{
  char *username;
  char *password;
};

struct service_t{
    char *servicename;
    char *secret;
    char *users;
};

struct account_t *accounts[10];
struct service_t *services[3];

void
printoutput(char *msg)
{
    puts(msg);
    fflush(stdout);
}


/* Check the username password given as string parameter 
 * and check if they exist in the accounts array
 * interrupts.
 * RETURNS:  0:username:0               - if it doesnt exist 
 *           1:username:auth_string     - if it exist
 */
char*
checkIfUserPassExist(char *input)
{
    char username[30];
    char password[30];
    char *output = (char *)malloc(sizeof(char)*60);

    sscanf (input,"%[^:]:%s",username,password); 

    
    /*get the size of accounts array*/
    int sizeOfAccountsArray = sizeof(accounts)/sizeof(accounts[0]);
    int i;
    for(i=0;i<sizeOfAccountsArray;i++)
    {
        if( strcmp(accounts[i]->username, username) ==0 && strcmp(accounts[i]->password, password) ==0)
        {
            sprintf(output, "1:%s:%s",username,AUTH_STRING);
            return output;
            break;
        }
    }
    sprintf(output, "0:%s:0",username);
    return output;
}

/* Process the message sent to ticket
 * interrupts.
 * RETURNS:  0:username:servicename:0                           - if it doesn't exist
 *           1:username:servicename:service_secret_key          - if it exist
 */
char*
processTicketMessageReceived(char *input)
{
    char username[30];
    char servicename[30];
    char authstring[100];
    
    char *output = (char *)malloc(sizeof(char)*200);

    sscanf (input,"%[^:]:%[^:]:%s",username,servicename,authstring); 
    if(strcmp(authstring,AUTH_STRING)==0)
    {
        /*get the size of accounts array*/
        int sizeOfArray = sizeof(services)/sizeof(services[0]);
        int i;
        for(i=0;i<sizeOfArray;i++)
        {
            if( strcmp(services[i]->servicename, servicename) ==0 && strstr(services[i]->users,username) != NULL )
            {
                sprintf(output, "1:%s:%s:%s",username,services[i]->servicename,services[i]->secret);
                return output;
                break;
            }
        }
    }
    sprintf(output, "0:%s:%s:0",username,servicename);
    return output; 
}

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
    
    char *output = (char *)malloc(sizeof(char)*50);

    sscanf (input,"%[^:]:%[^:]:%d:%d:%d:%[^:]:%[^:]:%s",username,servicename,&parameter1,&parameter2,&flag,ticketusername,ticketservicename,ticketservicesecretkey); 
    

    /*get the size of accounts array*/
    int sizeOfArray = sizeof(services)/sizeof(services[0]);
    int i;
    for(i=0;i<sizeOfArray;i++)
    {
        if( strcmp(services[i]->servicename, servicename) ==0 )
        {
            strcpy(servicesecretkey,services[i]->secret);
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


void
*auth_thread()
{
    auth_thread_id = pthread_self();
    char *comeback;
    int size;
    while(1){
        printf("starting loop\n");
        if (receive_message( &auth_thread_id, &comeback, &size) == MSG_OK) {
                printf("Auth ticket received message: 1--%s--size %d\n", comeback, size);
                char *processedMessage = checkIfUserPassExist(comeback);
                printf("Auth processed the message: %s\n Sending it to client thread...\n", processedMessage );
                fflush(stdout);
                if (send_message_to_thread( &client_thread_id, processedMessage, strlen(processedMessage)+1) != MSG_OK) {
                        printf( "sending message from client to auth thread failed\n" );
                }
                /* should free the processedMessage*/
        }
    }
    
/*
    printf("%s\n", checkIfUserPassExist("hamid:tavakoli") );
    printf("%s\n", processTicketMessageReceived("hamid:add:thisistheauthenticationstring123456") );
    printf("%s\n", processServiceMessageReceived("hamid:add:1:2:1:hamid:add:lkslkdfjweijfpiwe") );
*/
        pthread_exit(NULL);
}

void
*ticket_thread()
{
    ticket_thread_id = pthread_self();
/*
    printf("%s\n", checkIfUserPassExist("hamid:tavakoli") );
    printf("%s\n", processTicketMessageReceived("hamid:add:thisistheauthenticationstring123456") );
    printf("%s\n", processServiceMessageReceived("hamid:add:1:2:1:hamid:add:lkslkdfjweijfpiwe") );
*/
        pthread_exit(NULL);
}

/*
struct account_t
*ConvertStringToAccountStruct(char *input)
{
    struct account_t account;
    sscanf (input,"%[^:]:%s",account.username,account.password);  
    return &account;
}
*/


void 
*client(void *t) 
{
  char *comeback, one[] = "I am client";
  client_thread_id = pthread_self();
/*
  pthread_t *auththreadidclone = (pthread_t *)malloc(sizeof(pthread_t));
*/
  int size;
  
 size_t length;

    for (;;) {
        char buf[1000];
        printf("Please enter username and password: e.g  username:password \n");
        
        fgets(buf, 1000, stdin);
        /* Send and receive from destination process 1 (without threads running, we'll receive on thread 1 in torch's pthread implementation. */
        printf( "sending message from client to auth thread: %s...\n",buf );
        if (send_message_to_thread( auth_thread_id, buf, strlen(buf)+1) != MSG_OK) {
                printf( "sending message from client to auth thread failed\n" );
        }
        if (receive_message( &client_thread_id, &comeback, &size) == MSG_OK) {
                pthread_mutex_lock(&print_lock);
                printf("received message from Auth thread--%s--size %d\n", comeback, size);
                fflush(stdout);
                pthread_mutex_unlock(&print_lock);
        }

    } /* end for */

    pthread_exit(NULL);
}


void 
*server(void *t) 
{
  char *comeback, one[] = "I am server";
  server_thread_id = pthread_self();
  
  int size;

    /* Send a single message for starters. */
/*
    while(1) {      
        if (send_message_to_thread( client_thread_id, one, strlen(one)+1) != MSG_OK) {
                printoutput( "first failed\n" );
        }
        if (receive_message( &server_thread_id, &comeback, &size) == MSG_OK) {
                pthread_mutex_lock(&print_lock);
                printf("received message 1--%s--size %d\n", comeback, size);
                fflush(stdout);
                pthread_mutex_unlock(&print_lock);
        } else {
                printoutput ("first receive failed\n");
        }
        sleep(1);
    }   
*/
    pthread_exit(NULL);
}

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
/*
                printf ("username: %s -> password: %s\n",username,password);
*/
                strcpy(accounts[count]->username,username);
                strcpy(accounts[count]->password,password);
/*
                printf ("username2: %s -> password2: %s\n",accounts[count]->username, accounts[count]->password);
*/
                count ++;
            }
            fclose (fp);
        }
        
        
        int sizeOfAccountsArray = sizeof(accounts)/sizeof(accounts[0]);
        int i;
        for(i=0;i<sizeOfAccountsArray;i++)
        {
/*
            printf("array username: %s --> array password: %s \n",accounts[i]->username,accounts[i]->password);
*/
            fflush(stdout);
        }
        
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
/*
                printf ("servicename: %s -> secret: %s ->users: %s\n",servicename,secret,users);
*/
                strcpy(services[count]->servicename, servicename);
                strcpy(services[count]->secret, secret);
                strcpy(services[count]->users , users);
/*
                printf ("servicename2: %s -> secret2: %s ->users2: %s\n",services[count]->servicename,services[count]->secret,services[count]->users);
*/
                count ++;
            }
            fclose (fp);
        }
 
        return 1;
}

int 
main(int argc, char *argv[])
{
    int i;
    long t1=1, t2=2;
    pthread_t threads[4];
    pthread_attr_t attr;
    
    /* Initialize accounts global static variable*/
    int sizeOfArray = sizeof(accounts)/sizeof(accounts[0]);
    for(i=0;i<sizeOfArray;i++)
    {
        accounts[i] = (struct account_t *) malloc( sizeof(struct account_t ) );
        accounts[i]->username = (char *) malloc( sizeof( char ) ); 
        accounts[i]->password = (char *) malloc( sizeof( char ) );
    }
    i = 0;
    sizeOfArray = sizeof(services)/sizeof(services[0]);
    for(i=0;i<sizeOfArray;i++)
    {
        services[i] = (struct account_t *) malloc( sizeof(struct account_t ) );
        services[i]->servicename = (char *) malloc( sizeof( char ) ); 
        services[i]->secret = (char *) malloc( sizeof( char ) );
        services[i]->users = (char *) malloc( sizeof( char ) );
    }
    i = 0;
    
    
    read_auth_file("users.txt");
    read_ticket_file("tickets.txt");

/*
    auth_thread();
*/
    /* Don't start if we can't get the message system working. */
    if (messages_init() == MSG_OK) {
        /* Initialize mutex and condition variable objects */
        pthread_mutex_init(&print_lock, NULL);
        pthread_cond_init (&count_threshold_cv, NULL);

        /* For portability, explicitly create threads in a joinable state */
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

        /* Creating threads in joinable state*/

        pthread_create(&threads[0], &attr, client, (void *)t1); 
        pthread_create(&threads[1], &attr, server, (void *)t2); 
        pthread_create(&threads[2], &attr, auth_thread, (void *)t1); 
        pthread_create(&threads[3], &attr, ticket_thread, (void *)t2);   
    /*
        auth_thread();
    */
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


