#include <mqueue.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>


static void
usage()
{
    fprintf(stderr, "usage: ./sender <queue_name> <num_messages>\n");
    fprintf(stderr, "example: ./sender /cs3210 50\n");
    fprintf(stderr, "please make sure the queue_name has a leading \"/\" character.\n");
}

#define MAX_MSG_SIZE 4096
#define MQ_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | \
		  S_IROTH | S_IWOTH)


int main (int argc, char **argv)
{
    if (argc < 3){
	usage();
	exit(1);
    }
    char *queue_name = argv[1];
    int num_msgs = atoi(argv[2]);
    int flags, opt;
    mqd_t queue;
    mode_t perms;
    struct mq_attr attr;

    flags = O_WRONLY;
    
    queue = mq_open(queue_name, flags, MQ_PERMS, NULL);
    while(errno == 2){
	queue = mq_open(queue_name, flags, MQ_PERMS, NULL);
    }
    
    int i;
    for(i = 0; i<num_msgs; i++){ 	
	char *msg = "Sending a test message. The same message over and over.";
	mq_send (queue, msg, strlen(msg) + 1, 1);
	printf("sent message number: %d\n", i);
    }
    mq_close(queue);
    return 0;
}
