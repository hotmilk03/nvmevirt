// SPDX-License-Identifier: GPL-2.0-only

#ifndef _NVMEVIRT_CONV_FTL_H
#define _NVMEVIRT_CONV_FTL_H

#include <linux/types.h>
#include "pqueue/pqueue.h"
#include "ssd_config.h"
#include "ssd.h"

typedef enum {
	HOT_POOL = 0,
	COLD_POOL = 1,
} pool_t;

enum {
	HEAD = 0,
	TAIL = 1,
};

enum {
	COPY_ALL = 0,
	COPY_VALID = 1,
};

struct convparams {
	uint32_t gc_thres_lines;
	uint32_t gc_thres_lines_high;
	bool enable_gc_delay;
	bool enable_wl_delay;

	double op_area_pcent;
	int pba_pcent; /* (physical space / logical space) * 100*/
	
	uint32_t wl_thres;
};

struct line {
	int id; /* line id, the same as corresponding block id */
	int ipc; /* invalid page count in this line */
	int vpc; /* valid page count in this line */
	int ec; /* erase count */
	int eec; /* effective erasual cycle */
	struct list_head entry;
	/* position in the priority queue for victim lines */
	pool_t pool;
	size_t gc_pos;
	size_t min_ec_pos;
	size_t max_ec_pos;
	size_t eec_pos;
};

/* wp: record next write addr */
struct write_pointer {
	struct line *curline;
	uint32_t ch;
	uint32_t lun;
	uint32_t pg;
	uint32_t blk;
	uint32_t pl;
};

struct line_mgmt {
	struct line *lines;

	/* free line list, we only need to maintain a list of blk numbers */
	struct list_head free_line_list;
	pqueue_t *victim_line_pq;
	pqueue_t *hot_max_ec_pq;
	pqueue_t *hot_min_ec_pq;
	pqueue_t *cold_min_ec_pq;
	pqueue_t *hot_min_eec_pq;
	pqueue_t *cold_max_eec_pq;
	struct list_head full_line_list;

	uint32_t tt_lines;
	uint32_t free_line_cnt;
	uint32_t victim_line_cnt;
	uint32_t full_line_cnt;

	uint32_t max_ec;
	uint32_t sum_ec;
};

struct write_flow_control {
	uint32_t write_credits;
	uint32_t credits_to_refill;
};

struct conv_ftl {
	struct ssd *ssd;

	struct convparams cp;
	struct ppa *maptbl; /* page level mapping table */
	uint64_t *rmap; /* reverse mapptbl, assume it's stored in OOB */
	struct write_pointer wp;
	struct write_pointer gc_wp;
	struct write_pointer wl_wp;
	struct line_mgmt lm;
	struct write_flow_control wfc;
};

void conv_init_namespace(struct nvmev_ns *ns, uint32_t id, uint64_t size, void *mapped_addr,
			 uint32_t cpu_nr_dispatcher);

void conv_remove_namespace(struct nvmev_ns *ns);

bool conv_proc_nvme_io_cmd(struct nvmev_ns *ns, struct nvmev_request *req,
			   struct nvmev_result *ret);

#endif
