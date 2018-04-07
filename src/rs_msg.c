#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "rs_msg.h"

rs_msg_t* rs_alloc_msg(void)
{
    return malloc(MSGMAX);
}

void rs_free_msg(rs_msg_t **pmsg)
{
    if (*pmsg)
    {
        free(*pmsg);
        *pmsg = NULL;
    }
}

static const int g_msg_key = 0x545352; // 'RST' means run so tool
static int g_msg_Qid = -1;

int rs_init_msgQ(void)
{
    if (g_msg_Qid >= 0)
    {
        return 0;
    }
    
    g_msg_Qid = msgget(g_msg_key, IPC_CREAT | 0666);
    if (g_msg_Qid < 0)
    {
        return -1;
    }
    
    return 0;
}

int rs_reinit_msgQ(void)
{
    g_msg_Qid = -1;
    return rs_init_msgQ();
}

int rs_clr_msgQ(int pid)
{
    if (g_msg_Qid < 0)
    {
        errno = EIDRM;
        return -1;
    }
    
    AUTO_MSG rs_msg_t *msg = rs_alloc_msg();
    if (!msg)
    {
        errno = ENOMEM;
        return -1;
    }
    
    errno = 0;
    do 
    {
        // recv all msg if pid == 0
        msgrcv(g_msg_Qid, msg, MSGMAX, pid, MSG_NOERROR | IPC_NOWAIT);
    } while (!errno);
    
    return 0;
}

int rs_destroy_msgQ(void)
{
    if (g_msg_Qid < 0)
    {
        errno = EIDRM;
        return -1;
    }
    
    int ret = msgctl(g_msg_Qid, IPC_RMID, NULL);
    if (ret < 0)
    {
        return ret;
    }
    
    g_msg_Qid = -1;
    return 0;
}

static inline void init_msg(
        rs_msg_t *msg, 
        int       dst_pid,
        uint32_t  msg_type, 
        uint32_t  msg_size)
{
    msg->dst_pid  = dst_pid;
    msg->src_pid  = getpid();
    msg->msg_type = msg_type;
    msg->msg_size = msg_size;
}

int rs_send(const rs_msg_t *msg)
{
    if (msg->dst_pid < 0)
    {
        errno = EADDRNOTAVAIL;
        return -1;
    }
    
    if (msg->msg_size < sizeof(*msg))
    {
        errno = EINVAL;
        return -1;
    }
    
    // init msgQ automatically
    int ret = rs_init_msgQ();
    if (ret < 0)
    {
        return ret;
    }
    
    return msgsnd(g_msg_Qid, msg, msg->msg_size - sizeof(long), IPC_NOWAIT);
}

static inline int max(int x, int y)
{
    return x > y ? x : y;
}

int rs_recv(rs_msg_t *msg, int msg_size)
{
    if (msg_size < sizeof(*msg))
    {
        errno = EINVAL;
        return -1;
    }
    
    int max_buf_size = msg_size - sizeof(long);
    
    // init msgQ automatically
    int ret = rs_init_msgQ();
    if (ret < 0)
    {
        return ret;
    }
    
    msg->msg_size = 0;
    ret = msgrcv(g_msg_Qid, msg, max_buf_size, getpid(), MSG_NOERROR);
    if (ret < 0)
    {
        // init msgQ for next recv
        if (errno == EIDRM)
        {
            rs_reinit_msgQ();
            errno = EIDRM; // don't change errno
        }
        
        return -1;
    }
    
    int size = max(sizeof(*msg), msg->msg_size) - sizeof(long);
    if (ret < size)
    {
        errno = ERANGE;
        return -1;
    }
    
    if (msg->msg_size < sizeof(*msg))
    {
        errno = EINVAL;
        return -1;
    }
    
    return msg->msg_size;
}

static int rs_cmd_init(rs_cmd_t *cmd, int dst_pid, int32_t argc, char **argv)
{
    if (argc >= RS_MAX_ARG_NUM)
    {
        errno = E2BIG;
        return -1;
    }
    
    uint32_t cmd_len = sizeof(*cmd);
    char *tail = cmd->str;
    
    cmd->base_addr = cmd;
    for (cmd->argc = 0; cmd->argc < argc; cmd->argc++)
    {
        int str_len = strlen(argv[cmd->argc]) + 1;
        
        cmd_len += str_len;
        if (cmd_len > MSGMAX)
        {
            errno = ERANGE;
            return -1;
        }
        
        cmd->argv[cmd->argc] = memcpy(tail, argv[cmd->argc], str_len);
        tail += str_len;
    }
    
    init_msg(&cmd->msg_head, dst_pid, RS_MSG_CMD, cmd_len);
    
    return 0;
}

int rs_send_cmd(int dst_pid, int32_t argc, char **argv)
{
    AUTO_MSG rs_msg_t *msg = rs_alloc_msg();
    if (!msg)
    {
        errno = ENOMEM;
        return -1;
    }
    
    int ret = rs_cmd_init((void*)msg, dst_pid, argc, argv);
    if (ret < 0)
    {
        return -1;
    }
    
    return rs_send(msg);
}

int rs_cmd_adjust(rs_cmd_t *cmd)
{
    if (cmd->argc > RS_MAX_ARG_NUM)
    {
        errno = EINVAL;
        return -1;
    }
    
    for (int32_t i = 0; i < cmd->argc; i++)
    {
        cmd->argv[i] = (char*)cmd + (cmd->argv[i] - (char*)cmd->base_addr);
    }
    cmd->base_addr = cmd;
    
    return 0;
}

int rs_recv_cmd(rs_cmd_t *cmd)
{
    int ret = rs_recv(&cmd->msg_head, MSGMAX);
    if (ret < 0)
    {
        return -1;
    }
    
    if (rs_cmd_adjust(cmd))
    {
        return -1;
    }
    
    return ret;
}

rs_cmd_t* rs_clone_cmd(const rs_cmd_t *cmd)
{
    rs_msg_t *msg = rs_alloc_msg();
    if (!msg)
    {
        errno = ENOMEM;
        return NULL;
    }
    
    memcpy(msg, &cmd->msg_head, cmd->msg_head.msg_size);
    
    rs_cmd_t *clone_cmd = (void*)msg;
    if (rs_cmd_adjust(clone_cmd))
    {
        rs_free_msg(&msg);
        return NULL;
    }
    
    return clone_cmd;
}

int rs_log(int dst_pid, const char *fmt, ...)
{
    AUTO_MSG rs_msg_t *msg = rs_alloc_msg();
    if (!msg)
    {
        errno = ENOMEM;
        return -1;
    }
    
    rs_log_t *log = (void*)msg;
    int max_len = MSGMAX - sizeof(*log) - 1;
    log->str[max_len] = 0;
    
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(log->str, max_len, fmt, ap) + 1;
    va_end(ap);
    
    if (len <= 0)
    {
        return -1;
    }
    
    init_msg(msg, dst_pid, RS_MSG_LOG, sizeof(*log) + len);
    return rs_send(msg);
}

int rs_done(int dst_pid, int rs_rc, int usr_rc)
{
    rs_done_t done =
    {
        .rs_retcode  = rs_rc,
        .usr_retcode = usr_rc
    };
    
    init_msg(&done.msg_head, dst_pid, RS_MSG_DONE, sizeof(done));
    return rs_send(&done.msg_head);
}
