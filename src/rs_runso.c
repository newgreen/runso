#include <dlfcn.h>
#include <string.h>
#include "rs_msg.h"
#include "rs_runso.h"

static void set_log_id(void *handle, int client_pid)
{
    int **log_id_ptr = dlsym(handle, "__rs_log_id_ptr__");
    if (!log_id_ptr)
    {
        return;
    }
    
    int *log_id = *log_id_ptr;
    if (!log_id)
    {
        return;
    }
    
    *log_id = client_pid;
}

enum
{
    RS_RUNSO_LOAD,
    RS_RUNSO_RUN,
    RS_RUNSO_UNLOAD,
    RS_RUNSO_BUTT
};

static int str2op(const char *str)
{
    if (!strcmp(str, "load"))
    {
        return RS_RUNSO_LOAD;
    }
    
    if (!strcmp(str, "runso"))
    {
        return RS_RUNSO_RUN;
    }
    
    if (!strcmp(str, "unload"))
    {
        return RS_RUNSO_UNLOAD;
    }
    
    return RS_RUNSO_BUTT;
}

static void* load_dlopen(int client_pid, const char *so_name)
{
    void *handle = dlopen(so_name, RTLD_NOW | RTLD_NOLOAD);
    if (handle)
    {
        rs_log(client_pid, "Error: *** %s is already loaded.\n", so_name);
        dlclose(handle);
        return NULL;
    }
    
    handle = dlopen(so_name, RTLD_NOW);
    if (!handle)
    {
        rs_log(client_pid, "Error: *** load %s fail, %s.\n", 
            so_name, dlerror());
    }
    
    return handle;
}

static void* runso_dlopen(int client_pid, const char *so_name)
{
    void *handle = dlopen(so_name, RTLD_NOW);
    if (!handle)
    {
        rs_log(client_pid, "Error: *** open %s fail, %s.\n", 
            so_name, dlerror());
    }
    
    return handle;
}

static void* unload_dlopen(int client_pid, const char *so_name)
{
    void *handle = dlopen(so_name, RTLD_NOW | RTLD_NOLOAD);
    if (!handle)
    {
        rs_log(client_pid, "Error: *** %s is not loaded.\n", so_name);
    }
    else
    {
        dlclose(handle);
    }
    
    return handle;
}

static void* rs_dlopen(int client_pid, const char *so_name, int op)
{
    switch (op)
    {
        case RS_RUNSO_LOAD:
            return load_dlopen(client_pid, so_name);
        
        case RS_RUNSO_RUN:
            return runso_dlopen(client_pid, so_name);
        
        case RS_RUNSO_UNLOAD:
            return unload_dlopen(client_pid, so_name);
        
        // not go here
        default:
            return NULL;
    }
}

static void rs_dlclose(void **phandle)
{
    if (*phandle)
    {
        dlclose(*phandle);
        *phandle = NULL;
    }
}

#define AUTO_DL_HANDLE __attribute__((cleanup(rs_dlclose)))

static void* rs_get_entry(int client_pid, void *handle, int op)
{
    switch (op)
    {
        case RS_RUNSO_LOAD:
            return dlsym(handle, "__init__");
        
        case RS_RUNSO_RUN:
            return dlsym(handle, "__main__");
        
        case RS_RUNSO_UNLOAD:
            return dlsym(handle, "__exit__");
        
        // not go here
        default:
            return NULL;
    }
}

/**
 *  usage:
 *          load   <so_name> [ arg0 arg1 ... ]
 *          runso  <so_name> [ arg0 arg1 ... ]
 *          unload <so_name> [ arg0 arg1 ... ]
 */
int rs_runso(int client_pid, int argc, char **argv, int *usr_retcode)
{
    if (argc < 2)
    {
        rs_log(client_pid, "Error: *** args is not enough, argc %d.\n", argc);
        return -1;
    }
    
    const char *cmd = argv[0];
    const char *so_name = argv[1];
    
    int op = str2op(argv[0]);
    if (op == RS_RUNSO_BUTT)
    {
        rs_log(client_pid, "Error: *** invalid operate %s.\n", cmd);
        return -1;
    }
    
    AUTO_DL_HANDLE void *handle = rs_dlopen(client_pid, so_name, op);
    if (!handle)
    {
        return -1;
    }
    
    set_log_id(handle, client_pid);
    
    int (*entry)(int, char**) = rs_get_entry(client_pid, handle, op);
    if (!entry)
    {
        return -1;
    }
    
    *usr_retcode = entry(argc, argv);
    return 0;
}
