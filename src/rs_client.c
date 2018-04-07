#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <signal.h>
#include "rs_msg.h"
#include "rs_cs.h"

struct rs_cmd_op
{
    const char *cmd;
    const char *help;
    int (*check)(const struct rs_cmd_op *op, int argc, char **argv);
    int (*exec)(const struct rs_cmd_op *op, int argc, char **argv);
};

static const char* app_name(void)
{
    return "runso.exe";
}

static int parse_pid(char *nptr, int *pid)
{
    char *endptr = NULL;
    
    errno = 0;
    *pid = (int)strtol(nptr, &endptr, 0);
    
    if (errno || endptr != nptr + strlen(nptr))
    {
        printf("Error: *** pid '%s' is invalid\n", nptr);
        return -1;
    }
    
    if (kill(*pid, 0))
    {
        printf("Error: *** check pid %d fail, %s\n", *pid, strerror(errno));
        return -1;
    }
    
    return 0;
}

static const char* strtrim(const char *str)
{
    if (!str)
    {
        return NULL;
    }
    
    while (*str != '\0' && isspace(*str))
    {
        str++;
    }
    
    return str;
}

static void free_str(char **pstr)
{
    if (*pstr)
    {
        free(*pstr);
        *pstr = NULL;
    }
}

#define AUTO_STR __attribute__((cleanup(free_str)))

static int convert_so_name(const char *so_name, char *buf, size_t size)
{
    so_name = strtrim(so_name);
    
    if (*so_name == '/')
    {
        return -1;
    }
    
    AUTO_STR char *path = getcwd(NULL, 0); // free path automatically
    if (!path)
    {
        return -1;
    }
    
    if (strlen(path) + strlen("/") + strlen(so_name) + 1 > size)
    {
        return -1;
    }
    
    snprintf(buf, size, "%s/%s", path, so_name);
    return access(buf, R_OK);
}

static int check_runso_args(const struct rs_cmd_op *op, int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Error: *** not specify pid of target process.\n");
        printf("Usage: %s %s\n", app_name(), op->help);
        return -1;
    }
    
    if (argc < 3)
    {
        printf("Error: *** not specify so_name.\n");
        printf("Usage: %s %s\n", app_name(), op->help);
        return -1;
    }
    
    return 0;
}

static int send_runso_cmd(const struct rs_cmd_op *op, int argc, char **argv)
{
    int dst_pid;
    
    if (parse_pid(argv[1], &dst_pid))
    {
        return -1;
    }
    
    char path[PATH_MAX];
    if (!convert_so_name(argv[2], path, PATH_MAX))
    {
        argv[2] = path;
    }
    
    // remove pid from argv
    argv[1] = argv[0];
    argc--;
    argv++;
    
    return rs_execute_client(dst_pid, argc, argv);
}

static const struct rs_cmd_op g_rs_cmd_op_table[] = 
{
    {
        "load", 
        "load <pid> <so_name> [ arg0 arg1 ... ]", 
        check_runso_args, 
        send_runso_cmd
    },
    
    {
        "runso", 
        "runso <pid> <so_name> [ arg0 arg1 ... ]", 
        check_runso_args, 
        send_runso_cmd
    },
    
    {
        "unload", 
        "unload <pid> <so_name> [ arg0 arg1 ... ]", 
        check_runso_args, 
        send_runso_cmd
    },
    
    {NULL}
};

static void usage(void)
{
    const struct rs_cmd_op *op = g_rs_cmd_op_table;
    
    printf("Usage: \n"); 
    while (op->cmd)
    {
        printf("\t%s %s\n", app_name(), op->help);
        op++;
    }
}

static const struct rs_cmd_op* get_op(const char *cmd)
{
    const struct rs_cmd_op *op = g_rs_cmd_op_table;
    
    while (op->cmd)
    {
        if (!strcmp(op->cmd, cmd))
        {
            return op;
        }
        op++;
    }
    
    return NULL;
}

int main(int argc, char **argv)
{
    // skip app name
    argc--;
    argv++;
    
    if (argc < 1)
    {
        usage();
        return 0;
    }
    
    const struct rs_cmd_op *op = get_op(argv[0]);
    if (!op)
    {
        printf("Error: *** invalid cmd %s.\n", argv[0]);
        usage();
        return -1;
    }
    
    int ret = op->check(op, argc, argv);
    if (ret)
    {
        return ret;
    }
    
    return op->exec(op, argc, argv);
}
