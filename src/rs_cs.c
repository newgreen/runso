#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "rs_msg.h"
#include "rs_cs.h"

int rs_execute_client(int argc, char **argv)
{
    if (rs_get_dst_pid() < 0)
    {
        printf("Error: *** not set dest pid.\n");
        return -1;
    }
    
    AUTO_MSG rs_msg_t *msg = rs_alloc_msg();
    if (!msg)
    {
        printf("Error: *** alloc msg fail.\n");
        return -1;
    }
    
    int ret = rs_send_cmd(argc, argv);
    if (ret < 0)
    {
        printf("Error: *** send cmd to server fail, errno %d, %s.\n", 
            errno, strerror(errno));
        return -1;
    }
    
    while ((ret = rs_recv(msg)) >= 0 && msg->msg_type == RS_MSG_LOG)
    {
        rs_log_t *log = (void*)msg;
        printf("%s", log->str);
    }
    
    if (ret < 0 || msg->msg_type != RS_MSG_DONE)
    {
        printf("Error: *** not recv done msg.\n");
        return -1;
    }
    
    rs_done_t *done = (void*)msg;
    if (done->rs_retcode)
    {
        printf("Error: *** runso execute so fail, retcode %d.\n",
            done->rs_retcode);
        return -1;
    }
    
    return done->usr_retcode;
}

static const char *g_server_state_str[RS_SERVER_BUTT] = 
{
    "initial",
    "start",
    "run",
    "stop"
};

typedef struct
{
    int state;
    int cmd_cnt;
} srv_state_t;

static srv_state_t g_srv_state = 
{
    .state   = RS_SERVER_INIT,
    .cmd_cnt = 0
};

static int rs_chg_srv_state(int *cur_state, int old_state, int new_state)
{
    int val = __sync_val_compare_and_swap(
        &g_srv_state.state, 
        old_state, 
        new_state);
    
    int chg_ok = (val == old_state);
    
    if (cur_state)
    {
        *cur_state = chg_ok ? new_state : old_state;
    }
    
    return chg_ok;
}

static int rs_get_srv_state(void)
{
    return g_srv_state.state;
}

static int rs_inc_cmd(void)
{
    return __sync_add_and_fetch(&g_srv_state.cmd_cnt, 1);
}

static int rs_dec_cmd(void)
{
    return __sync_sub_and_fetch(&g_srv_state.cmd_cnt, 1);
}

static int rs_get_cmd_cnt(void)
{
    return g_srv_state.cmd_cnt;
}

static int runso(int32_t argc, char **argv, int32_t *usr_retcode)
{
    rs_log("[server]recv cmd:\n");
    for (int i = 0; i < argc; i++)
    {
        rs_log("[server][%d]: %s\n", i, argv[i]);
    }
    
    *usr_retcode = 0;
    return 0;
}

static void* rs_server_handle_cmd_thread(void *arg)
{
    pthread_detach(pthread_self());
    
    rs_cmd_t *cmd = arg;
    AUTO_MSG rs_msg_t *msg = &cmd->msg_head;
    
    if (msg->src_pid > 0)
    {
        rs_set_dst_pid(msg->src_pid);
    }
    else
    {
        printf("Error: *** src pid is unknown.\n");
        return NULL;
    }
    
    int rs_retcode = 0;
    int usr_retcode = 0;
    
    if (msg->msg_type != RS_MSG_CMD)
    {
        rs_log("Error: *** it's not cmd msg, msg_type %d, func %s, line %d.\n",
            msg->msg_type, __func__, __LINE__);
        rs_retcode = -1;
    }
    else
    {
        rs_retcode = runso(cmd->argc, cmd->argv, &usr_retcode);
    }
    
    (void)rs_done(rs_retcode, usr_retcode);
    
    int cmd_cnt = rs_dec_cmd();
    if (!cmd_cnt && g_srv_state.state == RS_SERVER_STOP)
    {
        if (rs_chg_srv_state(NULL, RS_SERVER_STOP, RS_SERVER_INIT))
        {
            printf("Info: server is stopped at cmd thread.\n");
        }
    }
    
    return NULL;
}

static int rs_handle_cmd(const rs_cmd_t *cmd)
{
    rs_cmd_t *new_cmd = rs_clone_cmd(cmd);
    if (!new_cmd)
    {
        printf("Error: *** clone cmd %s fail.\n", cmd->argv[0]);
        return -1;
    }
    
    (void)rs_inc_cmd();
    
    pthread_t tid;
    int ret = pthread_create(&tid, NULL, rs_server_handle_cmd_thread, new_cmd);
    if (ret)
    {
        (void)rs_dec_cmd();
        printf("Error: *** create thread for handling cmd fail, errno %d, %s",
            errno, strerror(errno));
    }
    
    return ret;
}

static void* rs_server_thread(void *arg)
{
    pthread_detach(pthread_self());
    
    AUTO_MSG rs_msg_t *msg = rs_alloc_msg();
    if (!msg)
    {
        (void)rs_chg_srv_state(NULL, RS_SERVER_START, RS_SERVER_INIT);
        return NULL;
    }
    
    (void)rs_chg_srv_state(NULL, RS_SERVER_START, RS_SERVER_RUN);
    while (rs_get_srv_state() == RS_SERVER_RUN)
    {
        rs_cmd_t *cmd = (void*)msg;
        int ret = rs_recv_cmd(cmd);
        if (ret < 0)
        {
            usleep(300 * 1000); // 300 ms
            continue;
        }
        
        if (msg->msg_type == RS_MSG_DONE)
        {
            (void)rs_chg_srv_state(NULL, RS_SERVER_RUN, RS_SERVER_STOP);
            break;
        }
        
        if (msg->msg_type == RS_MSG_CMD)
        {
            rs_handle_cmd((rs_cmd_t*)msg);
        }
    }
    
    if (!rs_get_cmd_cnt())
    {
        if (rs_chg_srv_state(NULL, RS_SERVER_STOP, RS_SERVER_INIT))
        {
            printf("Info: server is stopped at server thread.\n");
        }
    }
    return NULL;
}

int rs_start_server(void)
{
    pthread_t tid;
    int ret;
    
    int old_state;
    if (!rs_chg_srv_state(&old_state, RS_SERVER_INIT, RS_SERVER_START))
    {
        printf("Error: *** change state(%d->%d) fail, old_state %d.\n",
            RS_SERVER_INIT, RS_SERVER_START, old_state);
        return -1;
    }
    
    ret = pthread_create(&tid, NULL, rs_server_thread, NULL);
    if (ret)
    {
        (void)rs_chg_srv_state(NULL, RS_SERVER_START, RS_SERVER_INIT);
        printf("Error: *** create server thread fail, errno %d, %s.\n", 
            errno, strerror(errno));
    }
    
    return ret;
}

int rs_stop_server(void)
{
    rs_set_dst_pid(getpid());
    (void)rs_done(0, 0);
    
    usleep(100 * 1000); // delay 100 ms
    
    if (rs_get_srv_state() != RS_SERVER_INIT)
    {
        printf("Error: *** server is not stopped, state %d, cmd_cnt %d.\n",
            rs_get_srv_state(), rs_get_cmd_cnt());
        return -1;
    }

    return 0;
}

int rs_get_server_state(void)
{
    return g_srv_state.state;
}

const char* rs_server_state_tostr(int state)
{
    return (state < RS_SERVER_BUTT) ? g_server_state_str[state] : "unknown";
}
