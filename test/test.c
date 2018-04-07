#include <runso.h>
#include <unistd.h>

RS_STATE_LOG_ID(g_logid);

int __main__(int argc, char **argv)
{
    rs_log(g_logid, "target-pid : %d\n", getpid());
    
    rs_log(g_logid, "arg-list   :");
    for (int i = 0; i < argc; i++)
    {
        rs_log(g_logid, " %s", argv[i]);
    }
    rs_log(g_logid, "\n");
    
    return argc;
}
