#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "rs_msg.h"

int main(int argc, char **argv)
{
    int ret;
    
    rs_set_dst_pid(getpid());
    
#ifdef RS_SEND
    ret = rs_send_cmd(argc, argv);
    if (ret)
    {
        printf("Error: *** send args fail.\n");
        return ret;
    }
    
    printf("send cmd ok.\n");
#endif

#ifdef RS_RECV
    AUTO_MSG rs_msg_t *msg = rs_alloc_msg();
    if (!msg)
    {
        printf("Error: *** alloc msg fail.\n");
        return -1;
    }
    
#ifdef RS_RECV_LOOP
    while (1)
    {
#endif

    rs_cmd_t *cmd = (void*)msg;
    ret = rs_recv_cmd(cmd);
    if (ret < 0)
    {
        printf("Error: *** recv args fail, ret %d\n", ret);
#ifdef RS_RECV_LOOP
        sleep(1);
        continue;
#else
        return -1;
#endif
    }
    
    for (int i = 0; i < cmd->argc; i++)
    {
        printf("[%d]%s\n", i, cmd->argv[i]);
    }
    
#ifdef RS_RECV_LOOP
    }
#endif

#endif

#ifdef RS_CLRMSG
    ret = rs_clr_msgQ(0);
    if (ret < 0)
    {
        printf("Error: *** clear msg fail.\n");
        return ret;
    }
    
    printf("clr msg ok.\n");
#endif
    
    return 0;
}
