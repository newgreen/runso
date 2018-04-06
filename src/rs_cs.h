#ifndef _RS_CS_H_
#define _RS_CS_H_

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    RS_SERVER_INIT,
    RS_SERVER_START,
    RS_SERVER_RUN,
    RS_SERVER_STOP,
    RS_SERVER_BUTT
};

int rs_get_server_state(void);
const char* rs_server_state_tostr(int state);

int rs_start_server(void);
int rs_stop_server(void);

int rs_execute_client(int argc, char **argv);

#ifdef __cplusplus
} // extern "C"
#endif
#endif // _RS_CS_H_
