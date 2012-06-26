
#include "pmessages.h"
#include <string.h>
#include <pthread.h>
#include <stdio.h>

int
main( int argc, char **argv )
{
  char *comeback, one[] = "the quick brown fox", two[]="jumps over the lazy dog";
  pthread_t receive = pthread_self();
  int size;

  /* Don't start if we can't get the message system working. */

  if (messages_init() == MSG_OK) {

    /* Send and receive from destination process 1 (without threads running, we'll receive on thread 1 in torch's pthread implementation. */

    /* Send a single message for starters. */

    if (send_message_to_thread( receive, one, strlen(one)+1) != MSG_OK) {
      printf( "first failed\n" );
    }

    if (receive_message( &receive, &comeback, &size) == MSG_OK) {
      printf ("received message 1--%s--size %d\n", comeback, size );
    } else {
      printf ("first receive failed\n");
    }

    /* Ensure that we can have some capacity in our message system. */

    if (send_message_to_thread( receive, two, strlen(two)+1) != MSG_OK) {
      printf( "second 1 failed\n" );
    }
    if (send_message_to_thread( receive, one, strlen(one)+1) != MSG_OK) {
      printf( "second 2 failed\n" );
    }

    if (receive_message( &receive, &comeback, &size) == MSG_OK) {
      printf ("received message 2--%s--size %d\n", comeback, size );
    } else {
      printf ("second 1 receive failed\n");
    }

    if (receive_message( &receive, &comeback, &size) == MSG_OK) {
      printf ("received message 3--%s--size %d\n", comeback, size );
    } else {
      printf ("second 2 receive failed\n");
    }

    /* Clean up the message system. */

    messages_end();
  }

  return 1;
}

