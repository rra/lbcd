/*
 * ntp defines extracted from
 *
 * ntp_request.h
 * ntp_types.h
 * ntp_fp.h
 *
 */

#ifndef __monlist_h__
#define __monlist_h__

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
 * A request packet.  These are almost a fixed length.
 */
struct req_pkt {
	u_char rm_vn_mode;		/* response, more, version, mode */
	u_char auth_seq;		/* key, sequence number */
	u_char implementation;		/* implementation number */
	u_char request;			/* request number */
	u_short err_nitems;		/* error code/number of data items */
	u_short mbz_itemsize;		/* item size */
	char data[32];			/* data area */
	l_fp tstamp;			/* time stamp, for authentication */
	u_int32 keyid;			/* encryption key */
};


/*
 * Input packet lengths.  One with the mac, one without.
 */
#define	REQ_LEN_MAC	(sizeof(struct req_pkt))
#define	REQ_LEN_NOMAC	REQ_LEN_MAC

/*
 * A response packet.  The length here is variable, this is a
 * maximally sized one.  Note that this implementation doesn't
 * authenticate responses.
 */
#define	RESP_HEADER_SIZE	(8)
#define	RESP_DATA_SIZE		(500)

struct resp_pkt {
	u_char rm_vn_mode;		/* response, more, version, mode */
	u_char auth_seq;		/* key, sequence number */
	u_char implementation;		/* implementation number */
	u_char request;			/* request number */
	u_short err_nitems;		/* error code/number of data items */
	u_short mbz_itemsize;		/* item size */
	char data[RESP_DATA_SIZE];	/* data area */
};


/*
 * Information error codes
 */
#define	INFO_OKAY	0
#define	INFO_ERR_IMPL	1	/* incompatable implementation */
#define	INFO_ERR_REQ	2	/* unknown request code */
#define	INFO_ERR_FMT	3	/* format error */
#define	INFO_ERR_NODATA	4	/* no data for this request */
#define	INFO_ERR_AUTH	7	/* authentication failure */

/*
 * Bit setting macros for multifield items.
 */
#define	RESP_BIT	0x80
#define	MORE_BIT	0x40

#define	ISRESPONSE(rm_vn_mode)	(((rm_vn_mode)&RESP_BIT)!=0)
#define	ISMORE(rm_vn_mode)	(((rm_vn_mode)&MORE_BIT)!=0)
#define INFO_VERSION(rm_vn_mode) ((u_char)(((rm_vn_mode)>>3)&0x7))
#define	INFO_MODE(rm_vn_mode)	((rm_vn_mode)&0x7)

#define	RM_VN_MODE(resp, more)	((u_char)(((resp)?RESP_BIT:0)\
				|((more)?MORE_BIT:0)\
				|((NTP_VERSION)<<3)\
				|(MODE_PRIVATE)))

#define	INFO_IS_AUTH(auth_seq)	(((auth_seq) & 0x80) != 0)
#define	INFO_SEQ(auth_seq)	((auth_seq)&0x7f)
#define	AUTH_SEQ(auth, seq)	((u_char)((((auth)!=0)?0x80:0)|((seq)&0x7f)))

#define	INFO_ERR(err_nitems)	((u_short)((ntohs(err_nitems)>>12)&0xf))
#define	INFO_NITEMS(err_nitems)	((u_short)(ntohs(err_nitems)&0xfff))
#define	ERR_NITEMS(err, nitems)	(htons((u_short)((((u_short)(err)<<12)&0xf000)\
				|((u_short)(nitems)&0xfff))))

#define	INFO_MBZ(mbz_itemsize)	((ntohs(mbz_itemsize)>>12)&0xf)
#define	INFO_ITEMSIZE(mbz_itemsize)	(ntohs(mbz_itemsize)&0xfff)
#define	MBZ_ITEMSIZE(itemsize)	(htons((u_short)(itemsize)))


/*
 * XNTPD request codes go here.
 */
#define	REQ_MON_GETLIST		20	/* return data collected by monitor */

/*
 * Other defines
 */
#define	IMPL_XNTPD	2
#define NTP_VERSION     ((u_char)3) /* current version number */
#define MODE_PRIVATE    7       /* implementation defined function */

#endif /*  __monlist_h__ */
