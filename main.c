#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "pmessages.h"
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define NUM_THREADS  3
#define TCOUNT 10
#define COUNT_LIMIT 12
/*
#define USERNAME_MAX 30
#define PASSWORD_MAX 30
*/
#define LINESZ 1024

extern int errno; /* for printing error messages */

int     count = 0;
pthread_mutex_t print_lock;
pthread_cond_t count_threshold_cv;
pthread_t server_thread_id;
pthread_t client_thread_id;

typedef struct {
  char *username;
  char *password;
} account_t;

typedef struct {
    char *operation;
    char *secret;
    char *users;
} service_t;

static account_t acounts[10];
static service_t services[3];

void
printoutput(char *msg)
{
    puts(msg);
    fflush(stdout);
}

void 
*client(void *t) 
{
  char *comeback, one[] = "I am client";
  client_thread_id = pthread_self();
  
  int size;
  
  /* Don't start if we can't get the message system working. */
  if (messages_init() == MSG_OK) {

    /* Send and receive from destination process 1 (without threads running, we'll receive on thread 1 in torch's pthread implementation. */

    /* Send a single message for starters. */

    while(1){
        if (send_message_to_thread( server_thread_id, one, strlen(one)+1) != MSG_OK) {
                printoutput( "first failed\n" );
        }       
        if (receive_message( &client_thread_id, &comeback, &size) == MSG_OK) {
                pthread_mutex_lock(&print_lock);
/*
                printoutput ("Recieved client \n" );
*/
                printf("received message 1--%s--size %d\n", comeback, size);
                fflush(stdout);
                pthread_mutex_unlock(&print_lock);
        } else {
                printoutput ("first receive failed\n");
        }
        sleep(1);
    }  



    /* Clean up the message system. */

    messages_end();
  }
    pthread_exit(NULL);
}

void 
*server(void *t) 
{
  char *comeback, one[] = "I am server";
  server_thread_id = pthread_self();
  
  int size;
  
  /* Don't start if we can't get the message system working. */
  if (messages_init() == MSG_OK) {

    /* Send and receive from destination process 1 (without threads running, we'll receive on thread 1 in torch's pthread implementation. */

    /* Send a single message for starters. */
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


    /* Clean up the message system. */

    messages_end();
  }    
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
                printf ("username: %s -> password: %s\n",username,password);
                acounts[count].username =username;
                acounts[count].password =password;
                printf ("username2: %s -> password2: %s\n",acounts[count].username, acounts[count].password);
                count ++;
            }
            fclose (fp);
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
        char operation[5];
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
                sscanf (buff,"%[^:]:%[^:]:%s",operation,secret,users);                
                printf ("operation: %s -> secret: %s ->users: %s\n",operation,secret,users);
                services[count].operation = operation;
                services[count].secret = secret;
                services[count].users = users;
                printf ("operation2: %s -> secret2: %s ->users2: %s\n",services[count].operation,services[count].secret,services[count].users);
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
  pthread_t threads[3];
  pthread_attr_t attr;

  /* Initialize mutex and condition variable objects */
  pthread_mutex_init(&print_lock, NULL);
  pthread_cond_init (&count_threshold_cv, NULL);

  /* For portability, explicitly create threads in a joinable state */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  
  /* Creating threads in joinable state*/
/*
  pthread_create(&threads[0], &attr, client, (void *)t1); 
  pthread_create(&threads[1], &attr, server, (void *)t2);   
*/

  read_auth_file("users.txt");
  read_ticket_file("tickets.txt");
  
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

}


