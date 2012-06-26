
#ifndef _pmessages_private_h_

#include <pthread.h>
#include <semaphore.h>
#include "list.h"

/* Define the internal data structures for the message system */

typedef struct {
  sem_t block_for_receive;
  sem_t mailbox_lock;
  pthread_t owner;
  List_t mail;
} mailbox_t;

typedef struct {
  char *message;
  int  message_length;
  pthread_t sender;
} mail_item_t;

#endif

