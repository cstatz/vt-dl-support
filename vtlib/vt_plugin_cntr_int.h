/**
 * VampirTrace
 * http://www.tu-dresden.de/zih/vampirtrace
 *
 * Copyright (c) 2005-2010, ZIH, TU Dresden, Federal Republic of Germany
 *
 * Copyright (c) 1998-2005, Forschungszentrum Juelich, Juelich Supercomputing
 *                          Centre, Federal Republic of Germany
 *
 * See the file COPYING in the package base directory for details
 **/

#if defined(VT_PLUGIN_CNTR)

#ifndef _VT_PLUGIN_CNTR_INT_H
#define _VT_PLUGIN_CNTR_INT_H

#include "vt_plugin_cntr.h"
#include "vt_thrd.h"

EXTERN uint8_t vt_plugin_cntr_used;

/**
 * VampirTrace internal functions, which may change in later releases
 */

/**
 * get the number of synch metrics for the current thread
 */
uint32_t vt_plugin_cntr_get_num_synch_metrics(VTThrd * thrd);
/**
 * get the current value of the synch counter nr for the current thread
 */
uint64_t vt_plugin_cntr_get_synch_value(VTThrd * thrd, int nr, uint32_t * cid,
    uint64_t * value);

/**
 * write all callback data for threadID, which occured between the
 * last call of this function and time
 */
void vt_plugin_cntr_write_callback_data(uint64_t time, VTThrd * thrd);

/**
 * get the asynch values of postmortem counters
 * currently not implemented
 */
int vt_plugin_cntr_get_asynch_postmortem_values(VTThrd * thrd,
      vt_plugin_cntr_timevalue **);
/**
 * get the asynch values
 * currently not implemented
 */
int vt_plugin_cntr_get_asynch_values(VTThrd * thrd, vt_plugin_cntr_timevalue **);

/**
 * This should read the environment, and map the libraries
 */
void vt_plugin_cntr_init(void);
/**
 * This should set the correct counters for the current vampir trace thread
 */
void vt_plugin_cntr_thread_init(VTThrd * thrd);
/**
 * enable counters before tracing
 */
void vt_plugin_cntr_thread_enable_counters(VTThrd * thrd);
/**
 * disable counters after tracing
 */
void vt_plugin_cntr_thread_disable_counters(VTThrd * thrd);

/**
 * This should free all per thread ressources
 */
void vt_plugin_cntr_thread_exit(VTThrd * thrd);

/**
 * This should free all general ressources
 */
void vt_plugin_cntr_finalize(void);
/**
 * This should be used to check whether the current thread is
 * a monitor thread of a callback function.
 * Monitor threads should not be traced.
 */
int vt_plugin_cntr_is_registered_monitor_thread(void);

/**
 * writes all post_mortem events to the result files
 * threadID is the threadID of the current running thread
 * vd should be the VTThrd* of the thread, which is closed currently
 * This should be called when closing all threads
 */
void vt_plugin_cntr_final_write_post_mortem(VTThrd * thrd);



#endif /* _VT_PLUGIN_CNTR_INT_H */

#endif /* VT_PLUGIN_CNTR */
