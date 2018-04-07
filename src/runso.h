#ifndef _RUNSO_H_
#define _RUNSO_H_

#ifdef __cplusplus
extern "C" {
#endif

// state log_id
#define RS_STATE_LOG_ID(log_id) \
    int log_id = -1;            \
    int *__rs_log_id_ptr__ = &log_id

// extern log_id
#define RS_EXTERN_LOG_ID(log_id) extern int log_id

int rs_log(int log_id, const char *fmt, ...);

#ifdef __cplusplus
} // extern "C"
#endif
#endif // _RUNSO_H_
