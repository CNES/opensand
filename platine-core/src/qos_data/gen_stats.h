/**
 * @file gen_stats.h
 * @brief Statistics for Traffic Classifier in Linux kernel
 * @author iproute2 authors
 *
 * This file comes from iproute2 sources. Its content is released under the
 * GPL2 license.
 */

#ifndef __LINUX_GEN_STATS_H
#define __LINUX_GEN_STATS_H

#include <linux/types.h>


//#define MIN(x,y) ((x)<(y)?(x):(y))

#define TC_H_MAJ_MASK (0xFFFF0000U)
#define TC_H_MIN_MASK (0x0000FFFFU)
#define TC_H_MAJ(h) ((h)&TC_H_MAJ_MASK)
#define TC_H_MIN(h) ((h)&TC_H_MIN_MASK)
#define TC_H_MAKE(maj,min) (((maj)&TC_H_MAJ_MASK)|((min)&TC_H_MIN_MASK))

#define TC_H_UNSPEC     (0U)
#define TC_H_ROOT       (0xFFFFFFFFU)
#define TC_H_INGRESS    (0xFFFFFFF1U)

#define TC_HDLB_NUMPRIO         8
#define TC_HDLB_MAXDEPTH        8
#define TC_HDLB_PROTOVER        3 /* the same as HDLB and TC's major */

#define SPRINT_BSIZE 64
#define SPRINT_BUF(x)   char x[SPRINT_BSIZE]


enum
{
	TCA_STATS_UNSPEC,
	TCA_STATS_BASIC,
	TCA_STATS_RATE_EST,
	TCA_STATS_QUEUE,
	TCA_STATS_APP,
	__TCA_STATS_MAX,
};
#define TCA_STATS_MAX (__TCA_STATS_MAX - 1)

/**
 * struct gnet_stats_basic - byte/packet throughput statistics
 * @bytes: number of seen bytes
 * @packets: number of seen packets
 */
struct gnet_stats_basic
{
	__u64 bytes;
	__u32 packets;
};

/**
 * struct gnet_stats_rate_est - rate estimator
 * @bps: current byte rate
 * @pps: current packet rate
 */
struct gnet_stats_rate_est
{
	__u32 bps;
	__u32 pps;
};

/**
 * struct gnet_stats_queue - queuing statistics
 * @qlen: queue length
 * @backlog: backlog size of queue
 * @drops: number of dropped packets
 * @requeues: number of requeues
 * @overlimits: number of enqueues over the limit
 */
struct gnet_stats_queue
{
	__u32 qlen;
	__u32 backlog;
	__u32 drops;
	__u32 requeues;
	__u32 overlimits;
};

/**
 * struct gnet_estimator - rate estimator configuration
 * @interval: sampling period
 * @ewma_log: the log of measurement window weight
 */
struct gnet_estimator
{
	signed char interval;
	unsigned char ewma_log;
};

struct tc_stats
{
	__u64 bytes;      /* NUmber of enqueues bytes */
	__u32 packets;    /* Number of enqueued packets   */
	__u32 drops;      /* Packets dropped because of lack of resources */
	__u32 overlimits; /* Number of throttle events when this
	                   * flow goes out of allocated bandwidth */
	__u32 bps;        /* Current flow byte rate */
	__u32 pps;        /* Current flow packet rate */
	__u32 qlen;
	__u32 backlog;
};

struct tc_ratespec
{
	unsigned char  cell_log;
	unsigned char __reserved;
	unsigned short feature;
	short addend;
	unsigned short mpu;
	__u32 rate;
};

struct tc_hdlb_opt
{
	struct tc_ratespec t_rate;
	struct tc_ratespec p_rate;
	__u32 b;
	__u32 c;
	__u32 quantum;
	__u32 level; /* out only */
	__u32 prio;
};

struct tc_hdlb_glob
{
	__u32 version;      /* to match HDLB/TC */
	__u32 rate2quantum; /* bps->quantum divisor */
	__u32 defcls;       /* default class number */
	__u32 debug;        /* debug flags */

	/* stats */
	__u32 direct_pkts; /* count of non shapped packets */
};

enum
{
	TCA_HDLB_UNSPEC,
	TCA_HDLB_PARMS,
	TCA_HDLB_INIT,
	TCA_HDLB_T_RTAB,
	TCA_HDLB_P_RTAB,
	__TCA_HDLB_MAX,
};

struct tc_hdlb_xstats
{
	__u32 lends;
	__u32 borrows;
	__u32 giants; /* too big packets (rate will not be accurate) */
	__u32 Rtk;
	__u32 Ptk;
};


#endif /* __LINUX_GEN_STATS_H */
