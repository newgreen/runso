#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "rs_msg.h"

int main(int argc, char **argv)
{
    rs_set_dst_pid(getpid());
    
#ifdef RS_SEND
    int ret = rs_log("Hello, my name is %s, age is %d.\n", "zhengyp", 25);
    printf("send log, ret %d\n", ret);
    return ret;
#endif

#ifdef RS_RECV
    AUTO_MSG rs_msg_t *msg = rs_alloc_msg();
    if (!msg)
    {
        printf("Error: *** alloc msg fail.\n");
        return -1;
    }
    
    rs_log_t *log = (void*)msg;
    int ret = rs_recv(msg);
    if (ret < 0)
    {
        printf("Error: *** recv log msg fail.\n");
        return -1;
    }
    
    if (msg->msg_type != RS_MSG_LOG)
    {
        printf("Error: *** recv msg, but not log.\n");
        return -1;
    }
    
    printf("recv log: %s\n", log->str);
    return 0;
#endif
}
