/*
 * monlist.c
 *
 * xntpdc's monlist command butchered and packaged in a single .c
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "monlist.h"

/*
 * prototypes
 */
static int sendrequest(int sd, int implcode, int reqcode, int auth);
static int getresponse(int sd, int implcode, int reqcode, int *ritems);

/*
 * monlist
 *
 * Return min(500,number of ntp peers) for a machine
 */
int
monlist(int sd)
{
  int res, peers;

  /* send monlist request */
  if ((res = sendrequest(sd, IMPL_XNTPD, REQ_MON_GETLIST, 0)) != 0)
    return -1;

  /* get the monlist response */
  if ((res = getresponse(sd, IMPL_XNTPD, REQ_MON_GETLIST, &peers)) != 0)
    return -1;

  return peers;
}

/*
 * sendrequest - format and send a request packet
 */
static int
sendrequest(int sd, int implcode, int reqcode, int auth)
{
  struct req_pkt qpkt;

  memset(&qpkt, 0, sizeof qpkt);
  qpkt.rm_vn_mode = RM_VN_MODE(0, 0);
  qpkt.implementation = (u_char)implcode;
  qpkt.request = (u_char)reqcode;
  qpkt.err_nitems = ERR_NITEMS(0, 0);
  qpkt.mbz_itemsize = MBZ_ITEMSIZE(0);
  qpkt.auth_seq = AUTH_SEQ(0, 0);

  if (send(sd, (char *)&qpkt, REQ_LEN_NOMAC, 0) == -1) {
    return -1;
  }

  return 0;
}

/*
 * getresponse - get number of entries in the first packet
 */
#define MAXPACKETS 100

static int
getresponse(int sd, int implcode, int reqcode, int *ritems)
{
  struct resp_pkt rpkt;
  struct timeval tv = { 1, 0 };
  int items;
  int seq;
  fd_set fds;
  int n;
  int seenpacket[MAXPACKETS];
  int lastseq = 999;
  int numrecv = 0;

  /* Initialize ritems, seenpacket */
  *ritems = 0;
  memset(seenpacket,0,sizeof(seenpacket));

  /* Receive reply packet */
  while (1) {
    FD_ZERO(&fds);
    FD_SET(sd, &fds);
    if ((n = select(sd+1, &fds, NULL, NULL, &tv)) < 1) {
      return -1;
    }

    if ((n = recv(sd, (char *)&rpkt, sizeof(rpkt), 0)) == -1) {
      return -1;
    }

    /*
     * Check for format errors.
     */
    if ((n < RESP_HEADER_SIZE) ||
	(INFO_VERSION(rpkt.rm_vn_mode) != NTP_VERSION) ||
	(INFO_MODE(rpkt.rm_vn_mode) != MODE_PRIVATE) ||
	(INFO_IS_AUTH(rpkt.auth_seq)) ||
	(!ISRESPONSE(rpkt.rm_vn_mode)) ||
	(INFO_MBZ(rpkt.mbz_itemsize) != 0)){
      continue;
    }

    /*
     * Check implementation/request.  Could be old data getting to us.
     */
    if (rpkt.implementation != implcode || rpkt.request != reqcode) {
      continue;
    }
  
    /*
     * Check the error code.  If non-zero, return it.
     */
    if (INFO_ERR(rpkt.err_nitems) != INFO_OKAY) {
      return (int)INFO_ERR(rpkt.err_nitems);
    }
  
    /*
     * If it isn't the first packet, we don't care.
     */
    seq = INFO_SEQ(rpkt.auth_seq);
    if  (seq >= MAXPACKETS)
      continue;
    if (seenpacket[seq])
      continue;
    seenpacket[seq]++; 

    /*
     * Collect items and size.  Make sure they make sense.
     */
    *ritems += INFO_NITEMS(rpkt.err_nitems);

    /*
     * Check if end of sequence packet
     */
    if (!ISMORE(rpkt.rm_vn_mode)) {
      if (lastseq != 999)
	continue;
      lastseq = seq;
    }

    /*
     * Check if done
     */
    ++numrecv;
    if (numrecv <= lastseq)
      continue;

    break;
  }
  return 0;
}
