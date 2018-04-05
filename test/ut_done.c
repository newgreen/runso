#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "rs_msg.h"

int main(int argc, char **argv)
{
    rs_set_dst_pid(getpid());
    
#ifdef RS_SEND
    int ret = rs_done(1, 2);
    printf("send done, ret %d\n", ret);
    return ret;
#endif

#ifdef RS_RECV
    AUTO_MSG rs_msg_t *msg = rs_alloc_msg();
    if (!msg)
    {
        printf("Error: *** alloc msg fail.\n");
        return -1;
    }
    
    rs_done_t *done = (void*)msg;
    int ret = rs_recv(msg);
    if (ret < 0)
    {
        printf("Error: *** recv done msg fail.\n");
        return -1;
    }
    
    if (msg->msg_type != RS_MSG_DONE)
    {
        printf("Error: *** recv msg, but not done.\n");
        return -1;
    }
    
    printf("recv done: rs %d, usr %d\n", done->rs_retcode, done->usr_retcode);
    return 0;
#endif
}
