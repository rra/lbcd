/*
 * Default weight functions and the interface to internal modules.
 *
 * Written by Larry Schwimmer
 * Copyright 1997, 1998, 2006, 2008, 2012
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

#include <lbcd.h>
#include <lbcdload.h>

/* Supported services list. */
static lbcd_func_tab_t service_table[] = {
    /* Default. */
    { "load",    &lbcd_load_weight,    LBCD_ARGLB },

    /* Internal built-ins. */
    { "cmd",     &lbcd_cmd_weight,     LBCD_ARGNONE },
    { "rr",      &lbcd_rr_weight,      LBCD_ARGNONE },
    { "unknown", &lbcd_unknown_weight, LBCD_ARGNONE },

    /* Modules. */
    { "ftp",     &lbcd_ftp_weight,     LBCD_ARGNONE },
    { "http",    &lbcd_http_weight,    LBCD_ARGPORT },
    { "imap",    &lbcd_imap_weight,    LBCD_ARGNONE },
#ifdef HAVE_LDAP
    { "ldap",    &lbcd_ldap_weight,    LBCD_ARGNONE },
#endif
    { "nntp",    &lbcd_nntp_weight,    LBCD_ARGNONE },
    { "ntp",     &lbcd_ntp_weight,     LBCD_ARGNONE },
    { "pop",     &lbcd_pop_weight,     LBCD_ARGNONE },
    { "smtp",    &lbcd_smtp_weight,    LBCD_ARGNONE },
    { "tcp",     &lbcd_tcp_weight,     LBCD_ARGPORT },

    /* Last element is NULLs */
    { "",        NULL,                 LBCD_ARGNONE }
};

/* Module globals. */
static const char *lbcd_command;
static const char *lbcd_service;
static u_int default_weight;
static u_int default_increment;
static lbcd_func_tab_t* lbcd_default_functab;
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
static lbcd_func_tab_t *
service_to_func(const char *service)
{
    lbcd_func_tab_t *stp;
    LBCD_SERVICE_REQ name;
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
    for (stp = service_table; *stp->service != '\0'; stp++)
        if (strcmp(name, stp->service) == 0)
            return stp;

    /* Return unknown service */
    for (stp = service_table; *stp->service != '\0'; stp++)
        if (strcmp("unknown", stp->service) == 0)
            return stp;

    /* FIXME: fatal condition: should terminate program */
    return NULL;
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
lbcd_setweight(P_LB_RESPONSE *lb, int offset, const char *service)
{
    u_int *weight_ptr, *incr_ptr;
    lbcd_func_tab_t *functab;
    const char *cp;

    weight_ptr = &lb->weights[offset].host_weight;
    incr_ptr = &lb->weights[offset].host_incr;
    *incr_ptr = default_increment;

    functab = service_to_func(service);
    switch (functab->argument) {
    case LBCD_ARGNONE:
        functab->function(weight_ptr, incr_ptr, lbcd_timeout, NULL, lb);
        break;
    case LBCD_ARGLB:
        functab->function(weight_ptr, incr_ptr, lbcd_timeout, NULL, lb);
        break;
    case LBCD_ARGPORT:
        cp = !strcmp(service,"default") ? lbcd_service : service;
        cp = strchr(cp, ':');
        if (cp != NULL)
            cp++;
        functab->function(weight_ptr, incr_ptr, lbcd_timeout, cp, lb);
        break;
    }
}


/*
 * The unknown weight function.  Return the maximum weight.
 */
int
lbcd_unknown_weight(u_int *weight_val, u_int *incr_val, int timeout UNUSED,
                    const char *portarg UNUSED, P_LB_RESPONSE *lb UNUSED)
{
    *weight_val = (u_int) -1;
    *incr_val = 0;
    return 0;
}


/*
 * The simple round-robin load function.
 */
int
lbcd_rr_weight(u_int *weight_val, u_int *incr_val, int timeout UNUSED,
               const char *portarg UNUSED, P_LB_RESPONSE *lb UNUSED)
{
    *weight_val = default_weight;
    *incr_val = default_increment;
    return 0;
}


/*
 * Run an external command and set the weight and increment from its output.
 */
int
lbcd_cmd_weight(u_int *weight_val, u_int *incr_val, int timeout,
                const char *portarg, P_LB_RESPONSE *lb)
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
