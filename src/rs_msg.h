#ifndef _RS_MSG_H_
#define _RS_MSG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MSGMAX
#define MSGMAX 8192
#endif

#ifndef RS_MAX_ARG_NUM
#define RS_MAX_ARG_NUM 32
#endif

typedef enum
{
    RS_MSG_CMD,
    RS_MSG_LOG,
    RS_MSG_DONE
} rs_msg_type_t;

typedef struct
{
    long        dst_pid;
    long        src_pid;
    uint32_t    msg_type;
    uint32_t    msg_size;
} rs_msg_t;

typedef struct
{
    rs_msg_t    msg_head;
    void*       base_addr;  // rs_cmd's addr, to adjust argv[]'s addr
    char*       argv[RS_MAX_ARG_NUM];
    int32_t     argc;
    char        str[0];     // save string for argv[]
} rs_cmd_t;

typedef struct
{
    rs_msg_t    msg_head;
    char        str[0];     // save string for log info
} rs_log_t;

typedef struct
{
    rs_msg_t    msg_head;
    int32_t     rs_retcode;
    int32_t     usr_retcode;
} rs_done_t;

rs_msg_t* rs_alloc_msg(void);
void rs_free_msg(rs_msg_t **pmsg);
#define AUTO_MSG __attribute__((cleanup(rs_free_msg)))

int rs_init_msgQ(void);
int rs_reinit_msgQ(void);
int rs_clr_msgQ(int pid); // clear all msg if pid == 0
int rs_destroy_msgQ(void);

int rs_send(const rs_msg_t *msg);
int rs_recv(rs_msg_t *msg, int msg_size);

int rs_send_cmd(int dst_pid, int32_t argc, char **argv);
int rs_recv_cmd(rs_cmd_t *cmd);
int rs_cmd_adjust(rs_cmd_t *cmd);
rs_cmd_t* rs_clone_cmd(const rs_cmd_t *cmd);

int rs_log(int dst_pid, const char *fmt, ...);
int rs_done(int dst_pid, int rs_rc, int usr_rc);

#ifdef __cplusplus
} // extern "C"
#endif
#endif // _RS_MSG_H_
