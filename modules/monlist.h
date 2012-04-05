/*
 * NTP defines required to do a monlist query.
 *
 * Extracted from:
 *
 *     ntp_request.h
 *     ntp_types.h
 *     ntp_fp.h
 *
 * Modified fom xntpd source by Larry Schwimmer
 * Copyright 1992-1997 University of Delaware
 * Copyright 1997, 2008, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and that
 * both the copyright notice and this permission notice appear in supporting
 * documentation, and that the name University of Delaware not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission. The University of Delaware makes no
 * representations about the suitability this software for any purpose. It is
 * provided "as is" without express or implied warranty.
 */

#ifndef MODULES_MONLIST_H
#define MODULES_MONLIST_H 1

#include <portable/macros.h>

typedef unsigned int u_int32;
typedef int int32;

typedef struct {
    union {
        u_int Xl_ui;
        int Xl_i;
    } Ul_i;
    union {
        u_int Xl_uf;
        int Xl_f;
    } Ul_f;
} l_fp;

/*
 * A request packet.  These are almost a fixed length. */
struct req_pkt {
    u_char rm_vn_mode;          /* Response, more, version, mode */
    u_char auth_seq;            /* Key, sequence number */
    u_char implementation;      /* Implementation number */
    u_char request;             /* Request number */
    u_short err_nitems;         /* Error code/number of data items */
    u_short mbz_itemsize;       /* Item size */
    char data[32];              /* Data area */
    l_fp tstamp;                /* Time stamp, for authentication */
    u_int32 keyid;              /* Encryption key */
};

/* Input packet lengths.  One with the mac, one without. */
#define REQ_LEN_MAC     (sizeof(struct req_pkt))
#define REQ_LEN_NOMAC   REQ_LEN_MAC

/*
 * A response packet.  The length here is variable, this is a maximally sized
 * one.  Note that this implementation doesn't authenticate responses.
 */
#define RESP_HEADER_SIZE        (8)
#define RESP_DATA_SIZE          (500)

struct resp_pkt {
    u_char rm_vn_mode;          /* Response, more, version, mode */
    u_char auth_seq;            /* Key, sequence number */
    u_char implementation;      /* Implementation number */
    u_char request;             /* Request number */
    u_short err_nitems;         /* Error code/number of data items */
    u_short mbz_itemsize;       /* Item size */
    char data[RESP_DATA_SIZE];  /* Data area */
};

/* Information error codes. */
#define INFO_OKAY       0
#define INFO_ERR_IMPL   1       /* Incompatable implementation */
#define INFO_ERR_REQ    2       /* Unknown request code */
#define INFO_ERR_FMT    3       /* Format error */
#define INFO_ERR_NODATA 4       /* No data for this request */
#define INFO_ERR_AUTH   7       /* Authentication failure */

/* Bit setting macros for multifield items. */
#define RESP_BIT        0x80
#define MORE_BIT        0x40

#define ISRESPONSE(rm_vn_mode)   (((rm_vn_mode) & RESP_BIT) != 0)
#define ISMORE(rm_vn_mode)       (((rm_vn_mode) & MORE_BIT) != 0)
#define INFO_VERSION(rm_vn_mode) ((u_char)(((rm_vn_mode) >> 3) & 0x7))
#define INFO_MODE(rm_vn_mode)    ((rm_vn_mode) & 0x7)

#define RM_VN_MODE(resp, more) \
    ((u_char)(((resp) ? RESP_BIT : 0) | ((more) ? MORE_BIT : 0) \
              | ((NTP_VERSION) << 3) | (MODE_PRIVATE)))

#define INFO_IS_AUTH(auth_seq)   (((auth_seq) & 0x80) != 0)
#define INFO_SEQ(auth_seq)       ((auth_seq) & 0x7f)
#define AUTH_SEQ(auth, seq) \
    ((u_char)((((auth) != 0) ? 0x80 : 0) | ((seq) & 0x7f)))

#define INFO_ERR(err_nitems)     ((u_short)((ntohs(err_nitems) >> 12) & 0xf))
#define INFO_NITEMS(err_nitems)  ((u_short)(ntohs(err_nitems) & 0xfff))
#define ERR_NITEMS(err, nitems) \
    (htons((u_short)((((u_short)(err) << 12) & 0xf000) \
                     | ((u_short)(nitems) & 0xfff))))

#define INFO_MBZ(mbz_itemsize)      ((ntohs(mbz_itemsize) >> 12) & 0xf)
#define INFO_ITEMSIZE(mbz_itemsize) (ntohs(mbz_itemsize) & 0xfff)
#define MBZ_ITEMSIZE(itemsize)      (htons((u_short)(itemsize)))

/* XNTPD request codes. */
#define REQ_MON_GETLIST 20      /* Return data collected by monitor */

/* Other defines. */
#define IMPL_XNTPD   2
#define NTP_VERSION  ((u_char) 3)       /* Current version number */
#define MODE_PRIVATE 7                  /* Implementation defined function */

BEGIN_DECLS

extern int monlist(int sd, int timeout);

END_DECLS

#endif /* !MODULES_MONLIST_H */
