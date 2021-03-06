/*
 * Default weight functions and the interface to internal modules.
 *
 * Written by Larry Schwimmer
 * Copyright 1997, 1998, 2006, 2008, 2012, 2013
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/system.h>

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <server/internal.h>
#include <util/macros.h>
#include <util/messages.h>

/* Supported services list and a mapping from service to weight function. */
struct service_mapping {
    lbcd_name_type service;
    weight_func_type *function;
} service_table[] = {
    /* Default. */
    { "load",    &lbcd_load_weight    },

    /* Internal built-ins. */
    { "cmd",     &lbcd_cmd_weight     },
    { "rr",      &lbcd_rr_weight      },
    { "unknown", &lbcd_unknown_weight },

    /* Modules. */
    { "ftp",     &lbcd_ftp_weight     },
    { "http",    &lbcd_http_weight    },
    { "imap",    &lbcd_imap_weight    },
#ifdef HAVE_LDAP
    { "ldap",    &lbcd_ldap_weight    },
#endif
    { "nntp",    &lbcd_nntp_weight    },
    { "ntp",     &lbcd_ntp_weight     },
    { "pop",     &lbcd_pop_weight     },
    { "smtp",    &lbcd_smtp_weight    },
    { "tcp",     &lbcd_tcp_weight     },

    /* Last element is NULLs. */
    { "",      NULL                   }
};

/* Module globals. */
static const char *lbcd_command;
static const char *lbcd_service;
static uint32_t default_weight;
static uint32_t default_increment;
static const struct service_mapping *lbcd_default_functab;
static int lbcd_timeout;


/*
 * Look at an argument that may be a service or may be weights and determine
 * whether it's a weight and an increment.
 *
 * To be a weight and an increment, it must be of the form:
 *
 *     <weight>:<increment>
 *
 * where both <weight> and <increment> are entirely numeric.
 */
static int
is_weights(const char *service)
{
    const char *cp;
    int sawcolon;

    /* NULL string -- not a weight. */
    if (service == NULL)
        return -1;

    /* Must begin and end with a number */
    if (!isdigit((int) *service))
        return -1;
    if (!isdigit((int) service[strlen(service) - 1]))
        return -1;

    /* Must only consist of digits and a colon */
    sawcolon = 0;
    for (cp = service; *cp; cp++) {
        if (*cp != ':' && !isdigit(*cp))
            return -1;
        sawcolon += (*cp == ':');
    }
    if (sawcolon != 1)
        return -1;

    /* All okay. */
    return 0;
}


/*
 * Given the name of a service, return the function table entry for it or
 * NULL on failure.
 */
static const struct service_mapping *
service_to_func(const char *service)
{
    const struct service_mapping *stp;
    lbcd_name_type name;
    char *cp;

    /* Return default if service is NULL or "default". */
    if (service == NULL || strcmp("default", service) == 0)
        return lbcd_default_functab;

    /* Obtain service name portion (service:port). */
    strlcpy(name, service, sizeof(name));
    cp = strchr(name, ':');
    if (cp != NULL)
        *cp ='\0';

    /* Check table for exact match on service */
    for (stp = service_table; stp->service[0] != '\0'; stp++)
        if (strcmp(name, stp->service) == 0)
            return stp;

    /* Return unknown service */
    for (stp = service_table; stp->service[0] != '\0'; stp++)
        if (strcmp("unknown", stp->service) == 0)
            return stp;
    die("internal error: cannot locate unknown service");
}


/*
 * Initialize our globals from the command-line options.
 */
int
lbcd_weight_init(const char *cmd, const char *service, int timeout)
{
    lbcd_command = cmd;
    lbcd_service = service;
    lbcd_timeout = timeout;

    /* Round robin with default specified. */
    if (service != NULL && is_weights(service) == 0) {
        char *cp;

        cp = strchr(service,':');
        *cp++ = '\0';
        default_weight = atoi(service);
        default_increment = atoi(cp);
        lbcd_default_functab = service_to_func("rr");
    }
    /* External command */
    else if (cmd) {
        lbcd_default_functab = service_to_func("cmd");
    }
    /* Specified module */
    else {
        /* Specify simple defaults for round robin */
        default_weight = 1;
        default_increment = 1;

        /* Specify default load module */
        lbcd_default_functab = service_to_func(service ? service : "load");
    }
    return 0;
}


/*
 * Given a response, the number of the service, and the name of the service,
 * get the weight and increment for that service and fill it into the
 * response.
 */
void
lbcd_setweight(struct lbcd_reply *lb, int offset, const char *service)
{
    uint32_t *weight_ptr, *incr_ptr;
    const struct service_mapping *functab;
    const char *cp = NULL;

    weight_ptr = &lb->weights[offset].host_weight;
    incr_ptr = &lb->weights[offset].host_incr;
    *incr_ptr = default_increment;

    functab = service_to_func(service);
    cp = strcmp(service, "default") == 0 ? functab->service : service;
    cp = strchr(cp, ':');
    if (cp != NULL)
        cp++;
    functab->function(weight_ptr, incr_ptr, lbcd_timeout, cp, lb);
}


/*
 * The unknown weight function.  Return the maximum weight.
 */
int
lbcd_unknown_weight(uint32_t *weight_val, uint32_t *incr_val,
                    int timeout UNUSED, const char *portarg UNUSED,
                    struct lbcd_reply *lb UNUSED)
{
    *weight_val = (uint32_t) -1;
    *incr_val = 0;
    return 0;
}


/*
 * The simple round-robin load function.
 */
int
lbcd_rr_weight(uint32_t *weight_val, uint32_t *incr_val, int timeout UNUSED,
               const char *portarg UNUSED, struct lbcd_reply *lb UNUSED)
{
    *weight_val = default_weight;
    *incr_val = default_increment;
    return 0;
}


/*
 * Run an external command and set the weight and increment from its output.
 */
int
lbcd_cmd_weight(uint32_t *weight_val, uint32_t *incr_val, int timeout,
                const char *portarg, struct lbcd_reply *lb)
{
    int fd[2];

    /*
     * The following code is essentially popen(3) with a timeout and without
     * using /bin/sh.
     */
    if (pipe(fd) == 0) {
        pid_t child;

        child = fork();
        if (child < 0) {
            return lbcd_unknown_weight(weight_val, incr_val, timeout, portarg,
                                       lb);
        } else if (child == 0) {
            close(fd[0]);
            close(2);
            if (fd[1] != 1) {
                dup2(fd[1], 1);
                close(fd[1]);
            }
            execl(lbcd_command, lbcd_command, (char *) 0);
            exit(1);
        } else {
            int stat_loc;
            FILE *fp;
            char buf[128];

            close(fd[1]);
            fp = fdopen(fd[0], "r");
            if (fp == NULL) {
                kill(SIGTERM, child);
                waitpid(child, NULL, 0);
                return lbcd_unknown_weight(weight_val, incr_val, timeout,
                                           portarg, lb);
            }
            while (waitpid(child, &stat_loc, 0) < 0) {
                if (errno != EINTR) {
                    fclose(fp);
                    if (kill(SIGTERM, child) == -1)
                        kill(SIGKILL, child);
                    waitpid(child, NULL, 0);
                    return lbcd_unknown_weight(weight_val, incr_val, timeout,
                                               portarg, lb);
                }
            }
            if (WIFEXITED(stat_loc)) {
                if (WEXITSTATUS(stat_loc) != 0) {
                    fclose(fp);
                    return lbcd_unknown_weight(weight_val, incr_val, timeout,
                                               portarg, lb);
                }
            } else {
                if (kill(SIGTERM, child) == -1)
                    kill(SIGKILL, child);
                fclose(fp);
                return lbcd_unknown_weight(weight_val, incr_val, timeout,
                                           portarg, lb);
            }

            if (fgets(buf, sizeof(buf), fp) != NULL) {
                fclose(fp);
                if (sscanf(buf, "%d%d", weight_val, incr_val) != 2)
                    return lbcd_unknown_weight(weight_val, incr_val, timeout,
                                               portarg, lb);
            } else {
                fclose(fp);
                return lbcd_unknown_weight(weight_val, incr_val, timeout,
                                           portarg, lb);
            }
        }
    } else {
        return lbcd_unknown_weight(weight_val, incr_val, timeout, portarg, lb);
    }
    return 0;
}
