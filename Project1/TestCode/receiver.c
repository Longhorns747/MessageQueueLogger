#include <mqueue.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#define MAX_MSG_SIZE 4096
#define MQ_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | \
		  S_IROTH | S_IWOTH)

static void
usage()
{
    fprintf(stderr, "usage: ./receiver <queue_name> <expected_messages>\n");
    fprintf(stderr, "example: ./receiver /cs3210 50\n");
    fprintf(stderr, "please make sure the queue_name has a leading \"/\" character.\n");
}

static int
recv_message_blocking(mqd_t queue, void *msg)
{
    int err, ret_err;

loop:
    err = mq_receive(queue, (char*)msg, MAX_MSG_SIZE, NULL);
    if (err == -1) {
	if ((errno == EINTR) || (errno == EAGAIN)){
	    goto loop;
	}
	ret_err = -(errno);
	goto fail;
    }
    return 0;

fail:
    return ret_err;
}

static int
get_pending_msgs(mqd_t queue)
{
    struct mq_attr attr;
    mq_getattr(queue, &attr);
    return attr.mq_curmsgs;
}

void main (int argc, char **argv)
{
    if(argc < 3){
	usage();
	exit(1);
    }
    char *queue_name = argv[1];
    int exp_msgs = atoi(argv[2]);

    int flags, opt;
    mode_t perms;
    mqd_t queue;
    struct mq_attr attr;

    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    flags = O_RDONLY | O_CREAT;

    queue = mq_open(queue_name, flags, MQ_PERMS, &attr);

    int i;
    for(i = 0; i<exp_msgs; i++){

	printf("there are currently %d messages on the queue.\n",
	       get_pending_msgs(queue));
	void *msg = malloc(MAX_MSG_SIZE);
	memset(msg, 0, MAX_MSG_SIZE);
	int err = recv_message_blocking(queue, msg);
	free(msg);
    }
    mq_close(queue);
    mq_unlink(queue_name);
}
