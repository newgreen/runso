#include <stdio.h>
#include <string.h>
#include "rs_cs.h"

int main(int argc, char **argv)
{
    int ret = rs_start_server();
    if (ret)
    {
        return ret;
    }
    else
    {
        printf("Info: start server success.\n");
    }
    
    char cmd[4096] = {0};
    
    do
    {
        printf("admin:/>");
        
        cmd[0] = 0;
        if (scanf("%s", cmd) != 1)
        {
            continue;
        }
        
        if (!strcmp(cmd, "stat"))
        {
            printf("server state: %s\n", 
                rs_server_state_tostr(rs_get_server_state()));
            continue;
        }
        
        if (!strcmp(cmd, "stop"))
        {
            int ret = rs_stop_server();
            printf("stop server: %s\n", (ret) ? "error" : "ok");
            
            if (ret)
            {
                continue;
            }
            else
            {
                break;
            }
        }
        
    } while (rs_get_server_state() != RS_SERVER_INIT);
    
    return 0;
}
