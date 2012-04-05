/*
 * Utility code to return the number of NTP peers of a system.
 *
 * This code is xntpdc's monlist command butchered and packaged in a single
 * source file.
 *
 * Modified fom xntpd source by Larry Schwimmer
 * Copyright 1992-1997 University of Delaware
 * Copyright 1997, 1998, 2008, 2012
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

#include <config.h>
#include <portable/socket.h>
#include <portable/system.h>

#include <ctype.h>
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#include <sys/time.h>

#include <modules/monlist.h>

/* The maximum packets of rely to read from the server. */
#define MAXPACKETS 100


/*
 * Format and send a request packet.
 */
static int
sendrequest(int sd, int implcode, int reqcode)
{
    struct req_pkt qpkt;

    memset(&qpkt, 0, sizeof qpkt);
    qpkt.rm_vn_mode = RM_VN_MODE(0, 0);
    qpkt.implementation = (u_char) implcode;
    qpkt.request = (u_char) reqcode;
    qpkt.err_nitems = ERR_NITEMS(0, 0);
    qpkt.mbz_itemsize = MBZ_ITEMSIZE(0);
    qpkt.auth_seq = AUTH_SEQ(0, 0);

    if (send(sd, &qpkt, REQ_LEN_NOMAC, 0) == -1)
        return -1;
    return 0;
}


/*
 * Get number of entries in the first packet.
 */
static int
getresponse(int sd, int implcode, int reqcode, int *ritems, int timeout)
{
    struct resp_pkt rpkt;
    struct timeval tv = { timeout, 0 };
    int seq;
    fd_set fds;
    int n;
    int seenpacket[MAXPACKETS];
    int lastseq = 999;
    int numrecv = 0;

    *ritems = 0;
    memset(seenpacket, 0, sizeof(seenpacket));

    /* Receive reply packet. */
    while (1) {
        FD_ZERO(&fds);
        FD_SET(sd, &fds);
        n = select(sd+1, &fds, NULL, NULL, &tv);
        if (n < 1)
            return -1;
        n = recv(sd, (char *)&rpkt, sizeof(rpkt), 0);
        if (n == -1)
            return -1;

        /* Check for format errors. */
        if ((n < RESP_HEADER_SIZE)
            || (INFO_VERSION(rpkt.rm_vn_mode) != NTP_VERSION)
            || (INFO_MODE(rpkt.rm_vn_mode) != MODE_PRIVATE)
            || (INFO_IS_AUTH(rpkt.auth_seq))
            || (!ISRESPONSE(rpkt.rm_vn_mode))
            || (INFO_MBZ(rpkt.mbz_itemsize) != 0))
            continue;

        /* Check implementation/request.  Could be old data getting to us. */
        if (rpkt.implementation != implcode || rpkt.request != reqcode)
            continue;

        /* Check the error code.  If non-zero, return it. */
        if (INFO_ERR(rpkt.err_nitems) != INFO_OKAY)
            return INFO_ERR(rpkt.err_nitems);

        /* If it isn't the first packet, we don't care. */
        seq = INFO_SEQ(rpkt.auth_seq);
        if (seq >= MAXPACKETS)
            continue;
        if (seenpacket[seq])
            continue;
        seenpacket[seq]++;

        /* Collect items and size.  Make sure they make sense. */
        *ritems += INFO_NITEMS(rpkt.err_nitems);

        /* Check if end of sequence packet. */
        if (!ISMORE(rpkt.rm_vn_mode)) {
            if (lastseq != 999)
                continue;
            lastseq = seq;
        }

        /* Check if done. */
        numrecv++;
        if (numrecv <= lastseq)
            continue;
        break;
    }
    return 0;
}


/*
 * Return number of NTP peers for a machine.
 */
int
monlist(int sd, int timeout)
{
    int res, peers;

    /* Send monlist request. */
    res = sendrequest(sd, IMPL_XNTPD, REQ_MON_GETLIST);
    if (res != 0)
        return -1;

    /* Get the monlist response. */
    res = getresponse(sd, IMPL_XNTPD, REQ_MON_GETLIST, &peers, timeout);
    if (res != 0)
        return -1;
    return peers;
}
