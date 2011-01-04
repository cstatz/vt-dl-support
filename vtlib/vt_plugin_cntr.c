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

#include "vt_plugin_cntr.h"
#include "vt_plugin_cntr_int.h"
#include "vt_pform.h"
#include "vt_thrd.h"
#include "vt_trc.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This should not be exceeded */
#define VT_PLUGIN_COUNTERS_PER_THREAD 256

struct vt_plugin {
  /* info from get_info() */
  vt_plugin_cntr_info info;
  /* handle should be closed when finalize */
  void * dlfcn_handle;
  /* counter group for vt*/
  uint32_t counter_group;
  /* selected event_names */
  int num_selected_events;
  /* selected event_names */
  char ** selected_events;
  /* vt counter ids */
  uint32_t * vt_counter_ids;
};

static struct vt_plugin** vt_plugin_handles = NULL;
static uint32_t * nr_plugins = NULL;

/* the maximal number of events a callback may produce */
#define MAX_VALUES_CALLBACK 1024*1024

/* whether plugins are used or not*/
uint8_t vt_plugin_cntr_used = 0;

/* per thread values */
/* short cut for measuring */
struct vt_plugin_single_counter {
  /* the id, which was produced by the plugin */
  int32_t from_plugin_id;
  /* the id assigned by vt */
  uint32_t vt_counter_id;
  /* the N'th counter for this thread */
  uint32_t thread_incrementing_id;
  /* functions for en- and disabling */
  int32_t (*enable_counter)(int32_t);
  int32_t (*disable_counter)(int32_t);
  /* short cuts for getting values */
  uint64_t (*getValue)(int32_t);
  uint64_t (*getAllValues)(int32_t, vt_plugin_cntr_timevalue **);
};

/* used for per thread variables in VTThrd */
struct vt_plugin_cntr_defines{
  /* per synch type */
  uint32_t * size_of_counters;
  /* per synch type, size_of ...*/
  struct vt_plugin_single_counter ** vt_plugin_vector;
  /* last timestamp for callback, int per thread*/
  uint64_t last_callback_get_for_thread;
  /* current position in callback_values to write, int per thread */
  uint32_t current_callback_write_position;
  /* creates dummy thread for every thread that has a post mortem counter */
  uint32_t postmortem_dummy_thread_id;
  /* callback stuff */
  vt_plugin_cntr_timevalue * callback_values;
  void * callback_mutex;
};


/* called when activating events */
static void maybe_register_new_thread(VTThrd * thrd);
/* called when activating events */
static void add_events(struct vt_plugin current_plugin, VTThrd * thrd);

/* called by watchdog thread in a plugin */
int callback_function(void * ID, vt_plugin_cntr_timevalue tv);

void vt_plugin_cntr_init() {
  char ** plugins;
  int nr_selected_plugins = 0;
  char * plugin_read_start;
  int read_plugin;
  char * current_plugin;

  char * env_vt_plugin_metrics;
  char current_plugin_metric[255];
  char * next_plugin_metric;

  char buffer[512];

  void * handle;
  vt_plugin_cntr_info info;

  /* used union to get rid of compiler warning */
  union{
    void * vp;
    vt_plugin_cntr_info (* function)(void);

  } get_info;

  int index;
  int i;
  int found = 0;

  struct vt_plugin * current;

  /* set some internal variables to zero */
  vt_plugin_handles = calloc(
      VT_PLUGIN_CNTR_SYNCH_TYPE_MAX, sizeof(struct vt_plugin *));
  nr_plugins = calloc(
      VT_PLUGIN_CNTR_SYNCH_TYPE_MAX, sizeof(uint32_t));

  env_vt_plugin_metrics = getenv("VT_PLUGIN_CNTR_METRICS");
  if (env_vt_plugin_metrics == NULL)
    return;

  /* extract the plugin names */
  plugin_read_start = env_vt_plugin_metrics;
  read_plugin = 1;
  current_plugin = plugin_read_start;
  plugins = NULL;
  /* go through the plugin env. variable */
  for ( ; *current_plugin != '\0'; current_plugin++) {
    if (read_plugin) {
      if (*current_plugin == '_') {
        /* do not use the same plugin twice! */
        memcpy(buffer,plugin_read_start,
            ((current_plugin - plugin_read_start)) * sizeof(char));
        buffer[(current_plugin - plugin_read_start)]='\0';
        found = 0;
        for (i=0;i<nr_selected_plugins;i++){
          if (strcmp(buffer,plugins[i])==0)
            found = 1;
        }
        if (found){
          read_plugin = 0;
          continue;
        }
        else{
          nr_selected_plugins++;
          /* allocate the plugin name buffer */
          plugins = realloc(plugins, nr_selected_plugins * sizeof(char*));
            plugins[nr_selected_plugins - 1] = malloc((current_plugin
            - plugin_read_start + 1) * sizeof(char));
          /* copy the content to the buffer */
          memcpy(plugins[nr_selected_plugins - 1], plugin_read_start,
            ((current_plugin - plugin_read_start)) * sizeof(char));
          /* finish with null */
          plugins[nr_selected_plugins - 1][(current_plugin - plugin_read_start)]
                                         = '\0';
          read_plugin = 0;
        }
      }
    } else {
      /* a new plugin/counter starts after the ':' */
      if (*current_plugin == ':') {
        read_plugin = 1;
        plugin_read_start = current_plugin + 1;
      }
    }
  }

  /*go through all plugins:*/
  for (i = 0; i < nr_selected_plugins; i++) {
    current_plugin = plugins[i];
    vt_cntl_msg(3, "Selected plugin: %s", current_plugin);
    /* next one is stored in next_plugin,
    / * current is stored in current_plugin_buffer */
    /* load it from LD_LIBRARY_PATH*/
    sprintf(buffer, "lib%s.so", current_plugin);

    /* now dlopen it */
    handle = dlopen(buffer, RTLD_NOW);

    /* if it is not valid */
    if (handle == NULL) {
      vt_error_msg( "Error loading plugin: %s\n", dlerror());
      /* try loading next */
      continue;
    }

    /* now get the info */
    get_info.vp = dlsym(handle, "get_info");
    if (get_info.vp == NULL) {
      vt_error_msg( "Error getting info from plugin: %s\n", dlerror());
      /* try loading next */
      continue;
    }

    /* now store it */
    info = get_info.function();
    if (info.add_counter == NULL) {
      vt_error_msg( "Add counter not implemented in plugin %s\n",
          current_plugin);
      /* try loading next */
      continue;
    }

    if (info.get_event_info == NULL) {
      vt_error_msg( "Get event info not implemented in plugin %s\n",
          current_plugin);
      /* try loading next */
      continue;
    }

    if (info.synch != VT_PLUGIN_CNTR_SYNCH) {
      vt_error_msg( "unsupported synch-type in plugin %s\n",
          current_plugin);
      /* try loading next */
      continue;
    }

    /* check the type of plugin */
    switch (info.synch) {
    case VT_PLUGIN_CNTR_SYNCH:
      nr_plugins[VT_PLUGIN_CNTR_SYNCH]++;
      vt_plugin_handles[VT_PLUGIN_CNTR_SYNCH] =
        realloc(vt_plugin_handles[VT_PLUGIN_CNTR_SYNCH],
          nr_plugins[VT_PLUGIN_CNTR_SYNCH] * sizeof(struct vt_plugin));
      current = &vt_plugin_handles[VT_PLUGIN_CNTR_SYNCH]
                                   [nr_plugins[VT_PLUGIN_CNTR_SYNCH] - 1];
      if (info.get_current_value == NULL) {
        nr_plugins[VT_PLUGIN_CNTR_SYNCH]--;
        vt_error_msg("Get current results not implemented in plugin %s\n",
            current_plugin);
        /* try loading next */
        continue;
      }
      break;
    case VT_PLUGIN_CNTR_ASYNCH_CALLBACK:
      nr_plugins[VT_PLUGIN_CNTR_ASYNCH_CALLBACK]++;
      vt_plugin_handles[VT_PLUGIN_CNTR_ASYNCH_CALLBACK] = realloc(
          vt_plugin_handles[VT_PLUGIN_CNTR_ASYNCH_CALLBACK],
          nr_plugins[VT_PLUGIN_CNTR_ASYNCH_CALLBACK]* sizeof(struct vt_plugin));
      current = &vt_plugin_handles[VT_PLUGIN_CNTR_ASYNCH_CALLBACK]
                                   [nr_plugins[VT_PLUGIN_CNTR_ASYNCH_CALLBACK]
          - 1];
      if (info.set_callback_function == NULL) {
        nr_plugins[VT_PLUGIN_CNTR_ASYNCH_CALLBACK]--;
        vt_error_msg( "set callback not implemented in plugin %s\n",
            current_plugin);
        /* try loading next */
        continue;
      }
      if (info.set_pform_wtime_function == NULL) {
        nr_plugins[VT_PLUGIN_CNTR_ASYNCH_CALLBACK]--;
        vt_error_msg("set wtime not implemented in plugin %s\n",
            current_plugin);
        /* try loading next */
        continue;
      }
      break;
    case VT_PLUGIN_CNTR_ASYNCH_EVENT:
      nr_plugins[VT_PLUGIN_CNTR_ASYNCH_EVENT]++;
      vt_plugin_handles[VT_PLUGIN_CNTR_ASYNCH_EVENT] =
        realloc(vt_plugin_handles[VT_PLUGIN_CNTR_ASYNCH_EVENT],
          nr_plugins[VT_PLUGIN_CNTR_ASYNCH_EVENT] * sizeof(struct vt_plugin));
      current = &vt_plugin_handles[VT_PLUGIN_CNTR_ASYNCH_EVENT]
                                   [nr_plugins[VT_PLUGIN_CNTR_ASYNCH_EVENT] - 1];
      if (info.get_all_values == NULL) {
        nr_plugins[VT_PLUGIN_CNTR_ASYNCH_EVENT]--;
        vt_error_msg("get all values not implemented in plugin %s\n",
            current_plugin);
        /* try loading next */
        continue;
      }
      if (info.set_pform_wtime_function == NULL) {
        nr_plugins[VT_PLUGIN_CNTR_ASYNCH_EVENT]--;
        vt_error_msg("set wtime not implemented in plugin %s\n",
            current_plugin);
        /* try loading next */
        continue;
      }
      break;
    case VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM:
      nr_plugins[VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM]++;
      vt_plugin_handles[VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM] =
        realloc(vt_plugin_handles[VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM],
            nr_plugins[VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM] *
            sizeof(struct vt_plugin));
      current
        = &vt_plugin_handles[VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM]
                             [nr_plugins[VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM]
                               - 1];
      if (info.get_all_values == NULL) {
        vt_error_msg("get all values not implemented in plugin %s\n",
            current_plugin);
        nr_plugins[VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM]--;
        /* try loading next */
        continue;
      }
      if (info.set_pform_wtime_function == NULL) {
        vt_error_msg("set wtime not implemented in plugin %s\n",
            current_plugin);
        nr_plugins[VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM]--;
        /* try loading next */
        continue;
      }
      break;
    default:
      vt_error_msg(
          "Error getting synch type from plugin (invalid synch type)\n");
      continue;
    }

    /* clear out current plugin */
    memset(current, 0, sizeof(struct vt_plugin));

    /* add handle (should be closed in the end) */
    current->dlfcn_handle = handle;

    /* store the info object of the plugin */
    current->info = info;

    /* give plugin the wtime function to make it possible to convert times */
    if (current->info.set_pform_wtime_function != NULL) {
      current->info.set_pform_wtime_function(vt_pform_wtime);
    }
    /* initialize plugin */
    if (current->info.init()) {
      vt_error_msg("Error initializing plugin %s, init returned != 0\n",
          current_plugin);
      continue;
    }
    /* define a counter group for every plugin*/
    current->counter_group = vt_def_counter_group(
        VT_CURRENT_THREAD, current_plugin);

    /* now search for all available events on that plugin */
    next_plugin_metric = env_vt_plugin_metrics;
    while (next_plugin_metric[0] != 0) {
      /* shall contain current index in environment VT_PLUGIN_METRICS */
      index = 0;

      /* copy metric to current_plugin_metric char by char */
      while ((next_plugin_metric[index] != ':') && (next_plugin_metric[index]
          != '\0')) {
        current_plugin_metric[index] = next_plugin_metric[index];
        index++;
      }
      current_plugin_metric[index] = 0;
      if (next_plugin_metric[index] == ':')
        next_plugin_metric = &next_plugin_metric[index+1];
      else
        next_plugin_metric = &next_plugin_metric[index];
      vt_cntl_msg(3, "Selected plugin metric: %s", current_plugin_metric);
      /* If the plugin metric belongs to the current plugin */
      if (strstr(current_plugin_metric, current_plugin)
          == current_plugin_metric) {
        /* some meta data*/
        char * unit = NULL;
        uint32_t otf_prop = 0;
        /* check the event name from plugin */
        /* it could contain wildcards and other stuff */
        vt_plugin_cntr_metric_info * event_infos = info.get_event_info(
            &current_plugin_metric[strlen(current_plugin) + 1]);
        vt_plugin_cntr_metric_info * current_event_info;
        if (event_infos == NULL) {
          vt_error_msg(
              "Error initializing plugin metric %s, no info returned\n",
              current_plugin_metric);
          continue;
        }
        /* now for all events which are present in the struct */
        current_event_info = event_infos;
        for (current_event_info = event_infos;
          current_event_info->name != NULL;
          current_event_info++) {

          vt_cntl_msg(3, "Real plugin metric: %s", current_event_info->name);

          /* event is part of this plugin */
          current->num_selected_events++;

          /* allocate space for events */
          current->selected_events = realloc(current->selected_events,
              current->num_selected_events * sizeof(char*));

          /*the metric is everything after "plugin_"*/
          current->selected_events[current->num_selected_events - 1]
              = current_event_info->name;

          current->vt_counter_ids = realloc(current->vt_counter_ids,
              current->num_selected_events * sizeof(uint32_t));

          /* if a unit is provided, use it */
          unit = current_event_info->unit == NULL ? "#"
              : current_event_info->unit;

          /* if otf properties are provided, use them */
          otf_prop = current_event_info->cntr_property;
          /* define new counter */
          current->vt_counter_ids[current->num_selected_events - 1]
              = vt_def_counter(
                  VT_CURRENT_THREAD,
                  current->selected_events[current->num_selected_events - 1],
                  otf_prop, current->counter_group, unit);
          /* enable plugin counters */
          vt_plugin_cntr_used = 1;
        } /* end of: for all metrics related to the metric string */
        if (event_infos != NULL)
          free(event_infos);
      } /* end of if metric belongs to this plugin */
    } /* end of: for all plugin metrics */

  } /* end of: for all plugins */

  /* free temporary variables */
  for (i = 0; i < nr_selected_plugins; i++)
    if (plugins[i] != NULL)
      free(plugins[i]);
  if (plugins != NULL)
    free(plugins);

}

/**
 * this should be done for every thread
 * vt_plugin.c decides which plugin to enable
 */
void vt_plugin_cntr_thread_init(VTThrd * thrd) {
  uint32_t i = 0;
  uint32_t j = 0;

  vt_cntl_msg(3, "Process %i Thread %s%s registers own plugin metrics",
      vt_my_ptrace, thrd->name,thrd->name_suffix);
    /* for all plugin types*/
  for (i = 0; i < VT_PLUGIN_CNTR_SYNCH_TYPE_MAX; i++) {
    /* all plugins by this type */
    for (j = 0; j < nr_plugins[i]; j++) {
      if (vt_plugin_handles[i][j].info.run_per == VT_PLUGIN_CNTR_ONCE)
        if ((vt_my_trace != 0) || (thrd != VTThrdv[0]))
          continue;
      if (vt_plugin_handles[i][j].info.run_per == VT_PLUGIN_CNTR_PER_HOST)
        if ((!vt_my_trace_is_master) || (thrd != VTThrdv[0]))
          continue;

      if (vt_plugin_handles[i][j].info.run_per == VT_PLUGIN_CNTR_PER_PROCESS)
        /* for all threads != 0 dont trace*/
        if (thrd!=VTThrdv[0])
          continue;
     maybe_register_new_thread(thrd);
      /* now, that the thread is registered, register the counters */
      add_events(vt_plugin_handles[i][j], thrd);
    }
  }

}

/**
 * enable the counters, that were added before
 * this should be done for every thread. vt_plugin.c decides
 * which plugins/counters to enable
 */
void vt_plugin_cntr_thread_enable_counters(VTThrd * thrd) {
  uint32_t i, j;
  struct vt_plugin_cntr_defines * plugin_cntr_defines;

  vt_cntl_msg(3, "Process %i Thread %s-%s enables own plugin metrics",
      vt_my_ptrace, thrd->name,thrd->name_suffix);

  /* check whether thread is ok */
  if (thrd == NULL) return;
  if (thrd->plugin_cntr_defines == NULL) return;
  plugin_cntr_defines =
    (struct vt_plugin_cntr_defines *) thrd->plugin_cntr_defines;
  /* for all plugin types*/
  for (i = 0; i < VT_PLUGIN_CNTR_SYNCH_TYPE_MAX; i++) {
    /* all plugins by this type */
    for (j = 0; j < plugin_cntr_defines->size_of_counters[i]; j++) {
      struct vt_plugin_single_counter vts =
        plugin_cntr_defines->vt_plugin_vector[i][j];
      if (vts.enable_counter != NULL) {
        vts.enable_counter(vts.from_plugin_id);
      }
    }
  }
}

/**
 * disable the counters, that were added before
 * this should be done for every thread. vt_plugin.c decides
 * which plugins/counters to disabled
 */
void vt_plugin_cntr_thread_disable_counters(VTThrd * thrd) {
  uint32_t i, j;

  struct vt_plugin_cntr_defines * plugin_cntr_defines;

  vt_cntl_msg(3, "Process %i Thread %s%s disables own plugin metrics",
      vt_my_ptrace, thrd->name,thrd->name_suffix);

  /* check whether thread is ok */
  if (thrd == NULL) return;
  if (thrd->plugin_cntr_defines == NULL) return;

  plugin_cntr_defines =
    (struct vt_plugin_cntr_defines *) thrd->plugin_cntr_defines;

  /* for all plugin types*/
  for (i = 0; i < VT_PLUGIN_CNTR_SYNCH_TYPE_MAX; i++) {
    /* all plugins by this type */
    for (j = 0; j < plugin_cntr_defines->size_of_counters[i]; j++) {
      struct vt_plugin_single_counter vts =
        plugin_cntr_defines->vt_plugin_vector[i][j];
      if (vts.disable_counter != NULL) {
        vts.disable_counter(vts.from_plugin_id);
      }
    }
  }

}
/**
 * This should be called after the last thread exited.
 * It should free all ressources used by vt_plugin
 */
void vt_plugin_cntr_finalize() {
  uint32_t i, j;
  int k;

  vt_cntl_msg(3, "Process %i exits plugin", vt_my_ptrace);

  /* free all ressources */
  for (i = 0; i < VT_PLUGIN_CNTR_SYNCH_TYPE_MAX; i++) {

    /* data per plugin */
    for (j = 0; j < nr_plugins[i]; j++) {
      vt_plugin_handles[i][j].info.finalize();
      if (vt_plugin_handles[i][j].vt_counter_ids != NULL)
        free(vt_plugin_handles[i][j].vt_counter_ids);
      if (vt_plugin_handles[i][j].selected_events != NULL){
        for (k=0; k<vt_plugin_handles[i][j].num_selected_events; k++)
          if (vt_plugin_handles[i][j].selected_events[k])
            free(vt_plugin_handles[i][j].selected_events[k]);
        free(vt_plugin_handles[i][j].selected_events);
      }
    }
    free(vt_plugin_handles[i]);

  }
  free(vt_plugin_handles);
  if (nr_plugins)
    free(nr_plugins);
}

static void maybe_register_new_thread(VTThrd * thrd) {
  struct vt_plugin_cntr_defines * plugin_cntr_defines;
  vt_assert(thrd!=NULL);
  /* "register" a thread */
  if (thrd->plugin_cntr_defines == NULL) {
    uint32_t i;
    thrd->plugin_cntr_defines=calloc(1,sizeof(struct vt_plugin_cntr_defines));
    vt_assert(thrd->plugin_cntr_defines!=NULL);
    plugin_cntr_defines=
      (struct vt_plugin_cntr_defines * )thrd->plugin_cntr_defines;
    plugin_cntr_defines->vt_plugin_vector =
      calloc(VT_PLUGIN_CNTR_SYNCH_TYPE_MAX , sizeof(struct vt_plugin_single_counter *));
    /* number of counters of type */
    plugin_cntr_defines->size_of_counters =
      calloc(VT_PLUGIN_CNTR_SYNCH_TYPE_MAX , sizeof(uint32_t));

    /* single elements in vectors */
    for (i = 0; i < VT_PLUGIN_CNTR_SYNCH_TYPE_MAX; i++) {
      /* counters per type */
      plugin_cntr_defines->vt_plugin_vector[i] = calloc(VT_PLUGIN_COUNTERS_PER_THREAD,
        sizeof(struct vt_plugin_single_counter));
    }
  }
}

static void add_events(struct vt_plugin current_plugin, VTThrd * thrd) {
  int j;
  struct vt_plugin_single_counter * current;
  uint32_t * current_size;
  uint32_t thread_index;
  struct vt_plugin_cntr_defines * plugin_cntr_defines =
    (struct vt_plugin_cntr_defines * )thrd->plugin_cntr_defines;
  /* get the current counters for this thread and synch type*/
  current =
    plugin_cntr_defines->vt_plugin_vector[current_plugin.info.synch];
  if (current==NULL){
    plugin_cntr_defines->vt_plugin_vector=
      calloc(VT_PLUGIN_COUNTERS_PER_THREAD, sizeof(struct vt_plugin_single_counter));
    current =
      plugin_cntr_defines->vt_plugin_vector[current_plugin.info.synch];
  }
  /* get the number of counters for this thread and synch type*/
  current_size =
    &(plugin_cntr_defines->size_of_counters[current_plugin.info.synch]);

  vt_cntl_msg(3, "Process %i Thread %s-%s adds own plugin metrics",
      vt_my_ptrace, thrd->name,thrd->name_suffix);

  for (j = 0; j < current_plugin.num_selected_events; j++) {
    if (*current_size>=VT_PLUGIN_COUNTERS_PER_THREAD){
      vt_error_msg(
          "You're about to add more then %i plugin counters,"
          "which is impossible\n",VT_PLUGIN_COUNTERS_PER_THREAD);
      continue;
    }
    /* currently no key value :( */
    if (current_plugin.info.synch == VT_PLUGIN_CNTR_ASYNCH_CALLBACK) {
      if (*current_size == 0) {
        /* allocate resources */
#if (defined(VT_MT) || defined (VT_HYB) || defined(VT_JAVA))
        plugin_cntr_defines->callback_values=malloc(
            MAX_VALUES_CALLBACK*sizeof(vt_plugin_cntr_timevalue));
        VTThrd_createMutex((VTThrdMutex **) &(plugin_cntr_defines->callback_mutex));
#else
        vt_error_msg(
                    "callback events need thread support, you might use -vt:mt or -vt:hyb\n");
        continue;
#endif  /* VT_MT || VT_HYB || VT_JAVA */
      }
      else{
        vt_error_msg(
            "currently only one callback event per thread can be executed\n");
        continue;
      }
    }
    /* add counter */
    current[*current_size].from_plugin_id
        = current_plugin.info.add_counter(current_plugin.selected_events[j]);

    /* add successfully? */
    if (current[*current_size].from_plugin_id < 0){
        vt_error_msg(
            "Error while adding plugin counter \"%s\" to thread \"%s%s\"\n",
            current_plugin.selected_events[j],thrd->name,thrd->name_suffix);
      continue;
    }
   /* get the vampir trace id for the counter */
    current[*current_size].vt_counter_id
        = current_plugin.vt_counter_ids[j];
    current[*current_size].enable_counter
        = current_plugin.info.enable_counter;
    current[*current_size].disable_counter
        = current_plugin.info.disable_counter;

    /* per type stuff */
    if (current_plugin.info.synch == VT_PLUGIN_CNTR_SYNCH)
      /* synch counters have to implement getValue */
      current[*current_size].getValue
          = current_plugin.info.get_current_value;
    if ((current_plugin.info.synch == VT_PLUGIN_CNTR_ASYNCH_EVENT) ||
        (current_plugin.info.synch == VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM)){
      /* these have to implement getAllValues */
      current[*current_size].getAllValues
          = current_plugin.info.get_all_values;
      /* if there's no dummy thread (will be removed for 5.10) */
      if (plugin_cntr_defines->postmortem_dummy_thread_id==0){
        /* create a dummy thread */
        thread_index = VTThrd_createNewThreadId();
        VTThrd_create(thread_index, VT_MY_THREAD, "Plugin Counter", 1);
        VTThrd_open(thread_index);
        plugin_cntr_defines->postmortem_dummy_thread_id=thread_index;
      } else
      {
        /* use the existing dummy thread */
        thread_index=plugin_cntr_defines->postmortem_dummy_thread_id;
      }
    }
    if (current_plugin.info.synch == VT_PLUGIN_CNTR_ASYNCH_CALLBACK) {
      /* callback should set the callback function */
      plugin_cntr_defines->callback_values = malloc(
          MAX_VALUES_CALLBACK * sizeof(vt_plugin_cntr_timevalue));
      current[*current_size].thread_incrementing_id = j;
      current_plugin.info.set_callback_function(
          plugin_cntr_defines,
          current[*current_size].from_plugin_id, callback_function);

    }

    (*current_size)++;
  }
}

uint32_t vt_plugin_cntr_get_num_synch_metrics(VTThrd * thrd) {
  uint32_t num=0;
  vt_assert(thrd != NULL);
  if(thrd->plugin_cntr_defines == NULL) return 0;
  num = ((struct vt_plugin_cntr_defines * )(thrd->plugin_cntr_defines))
          ->size_of_counters[VT_PLUGIN_CNTR_SYNCH];
  return num;
}

uint64_t vt_plugin_cntr_get_synch_value(VTThrd * thrd,
    int threadIncrementingID, uint32_t * cid,
    uint64_t * value) {
  *cid = ((struct vt_plugin_cntr_defines * )(thrd->plugin_cntr_defines))->
    vt_plugin_vector[VT_PLUGIN_CNTR_SYNCH][threadIncrementingID].vt_counter_id;
  *value=
  ((struct vt_plugin_cntr_defines * )(thrd->plugin_cntr_defines))->
    vt_plugin_vector[VT_PLUGIN_CNTR_SYNCH][threadIncrementingID].getValue(
        ((struct vt_plugin_cntr_defines * )(thrd->plugin_cntr_defines))->
          vt_plugin_vector[VT_PLUGIN_CNTR_SYNCH][threadIncrementingID].from_plugin_id);
  return *value;
}

void vt_plugin_cntr_thread_exit(VTThrd * thrd) {
  uint32_t i;

  struct vt_plugin_cntr_defines * defines =
    (struct vt_plugin_cntr_defines * )thrd->plugin_cntr_defines;

  vt_cntl_msg(3, "Process %i Thread %s-%s exits plugin counters",
      vt_my_ptrace, thrd->name,thrd->name_suffix);

  /* make sure that we can process */
  if (defines==NULL) return;

  vt_plugin_cntr_thread_disable_counters(thrd);
  /* free per thread resources */
  if (defines->size_of_counters!=NULL)
    free (defines->size_of_counters);
  if (defines->vt_plugin_vector!=NULL){
    for (i=0;i<VT_PLUGIN_CNTR_SYNCH_TYPE_MAX;i++)
      if (defines->vt_plugin_vector[i]!=NULL)
        free(defines->vt_plugin_vector[i]);
    free (defines->vt_plugin_vector);
  }
  if (defines->callback_values!=NULL)
    free (defines->callback_values);
#if (defined(VT_MT) || defined (VT_HYB) || defined(VT_JAVA))
  if (defines->callback_mutex!=NULL)
    VTThrd_deleteMutex((VTThrdMutex **) &(defines->callback_mutex));
#endif  /* VT_MT || VT_HYB || VT_JAVA */
  free(defines);

}

/**
 * This is called by a callback plugin
 * It writes data to a data buffer, which is later "collected"
 * with write_callback_data by vampir_trace
 */
int32_t callback_function(void * ID, vt_plugin_cntr_timevalue tv) {
  struct vt_plugin_cntr_defines * defines =
      (struct vt_plugin_cntr_defines *) ID;
  vt_plugin_cntr_timevalue * timeList;
  if (defines->current_callback_write_position>=MAX_VALUES_CALLBACK) return -1;
  timeList = defines->callback_values;
#if (defined(VT_MT) || defined (VT_HYB) || defined(VT_JAVA))
  VTThrd_lock((VTThrdMutex **) &defines->callback_mutex);
#endif  /* VT_MT || VT_HYB || VT_JAVA */
  timeList[defines->current_callback_write_position++] = tv;
#if (defined(VT_MT) || defined (VT_HYB) || defined(VT_JAVA))
  VTThrd_unlock((VTThrdMutex **) &defines->callback_mutex);
#endif  /* VT_MT || VT_HYB || VT_JAVA */
  return 0;
}

/**
 * write collected callback data to otf
 * this has to be done in a mutex. currently the mutex is per thread,
 * later it might be done per counter of a thread?
 */
void vt_plugin_cntr_write_callback_data(uint64_t time, VTThrd * thrd) {
  uint32_t counterID, i, k;
  struct vt_plugin_cntr_defines * plugin_cntr_defines =
    (struct vt_plugin_cntr_defines * )thrd->plugin_cntr_defines;

  if (plugin_cntr_defines==NULL)
  if (plugin_cntr_defines->size_of_counters[VT_PLUGIN_CNTR_ASYNCH_CALLBACK] == 0)
    return;
#if (defined(VT_MT) || defined (VT_HYB) || defined(VT_JAVA))
  VTThrd_lock((VTThrdMutex **) &plugin_cntr_defines->callback_mutex);
#endif  /* VT_MT || VT_HYB || VT_JAVA */
  for (i = 0;
  i < plugin_cntr_defines->size_of_counters[VT_PLUGIN_CNTR_ASYNCH_CALLBACK];
  i++) {
    vt_plugin_cntr_timevalue * values = plugin_cntr_defines->callback_values;
    counterID = plugin_cntr_defines->vt_plugin_vector[VT_PLUGIN_CNTR_ASYNCH_CALLBACK]
                                 [i].vt_counter_id;
    k = 0;
    while ((k < plugin_cntr_defines->current_callback_write_position)) {
      if ((values[k].timestamp <= time) && (values[k].timestamp
          > plugin_cntr_defines->last_callback_get_for_thread))
        vt_count(VT_MY_THREAD,&values[k].timestamp, counterID, values[k].value);
      else
        break;
      k++;
    }
    plugin_cntr_defines->current_callback_write_position = 0;
    plugin_cntr_defines->last_callback_get_for_thread = time;
  }
#if (defined(VT_MT) || defined (VT_HYB) || defined(VT_JAVA))
  VTThrd_unlock((VTThrdMutex **) &plugin_cntr_defines->callback_mutex);
#endif  /* VT_MT || VT_HYB || VT_JAVA */
}

/* may be called per thread */
void vt_plugin_cntr_final_write_post_mortem(VTThrd * thrd){
  uint32_t counter_index;
  vt_plugin_cntr_timevalue ** time_values;
  vt_plugin_cntr_timevalue ** current_time_values;
  vt_plugin_cntr_timevalue * selected_time_value;
  uint32_t selected_counter=0;
  uint32_t number_of_counters;
  vt_plugin_cntr_timevalue tmp_time_value;
  uint64_t * number_of_values;
  uint64_t i,j;
  int change;
  uint64_t last_timestamp=0;
  uint32_t count_id;
  struct vt_plugin_cntr_defines * plugin_cntr_defines =
    (struct vt_plugin_cntr_defines * )thrd->plugin_cntr_defines;

  if (plugin_cntr_defines==NULL)
    return;

  if (plugin_cntr_defines->size_of_counters
      [VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM]==0)
    return;
  /* for all post_mortem counters */
  number_of_counters=
    plugin_cntr_defines->size_of_counters[VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM];
  number_of_values=calloc(number_of_counters,sizeof(uint64_t));
  time_values=calloc(number_of_counters,sizeof(vt_plugin_cntr_timevalue *));
  current_time_values=calloc(number_of_counters,
      sizeof(vt_plugin_cntr_timevalue *));

  /* for all counters of this thread */
  for(counter_index=0;
    counter_index<number_of_counters;
    counter_index++){
   /* get data */
    number_of_values[counter_index]=
      plugin_cntr_defines->vt_plugin_vector
        [VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM][counter_index].getAllValues(
            plugin_cntr_defines->vt_plugin_vector
            [VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM][counter_index].from_plugin_id,
          &time_values[counter_index]
      );

    /* sort data  if there is some*/
    if (number_of_values[counter_index]!=0){
      for (i=number_of_values[counter_index]-1;i!=(uint64_t)-1;i--){
        change=0;
        for (j=0;j<i;j++){
          if (time_values[counter_index][j].timestamp>time_values[counter_index][j+1].timestamp){
            tmp_time_value=time_values[counter_index][j];
            time_values[counter_index][j]=time_values[counter_index][j+1];
            time_values[counter_index][j+1]=tmp_time_value;
            change=1;
          }
        }
      if (!change) break;
      }
      /* select the first event for all counters */
      current_time_values[counter_index]=time_values[counter_index];
    }
    else{
      current_time_values[counter_index]=NULL;
    }
  }
  /* sort all results returned */
  while (1){
    /* select the next in time, NULL means none selected/counter
     * has no more data
     */
    selected_time_value=NULL;
    for(counter_index=0; counter_index<number_of_counters; counter_index++){
      /* select one if none is selected */
      if (selected_time_value==NULL){
        if(current_time_values[counter_index]!=NULL){
          selected_time_value=current_time_values[counter_index];
          selected_counter=counter_index;
        }
      }
      /* compare timestamps */
      else{
        if(current_time_values[counter_index]!=NULL)
          if (selected_time_value->timestamp>current_time_values[counter_index]->timestamp){
            selected_time_value=current_time_values[counter_index];
            selected_counter=counter_index;
          }

      }
    }
    /* if there's no selected event we're done */
    if (selected_time_value==NULL) break;

    /* otherwise we write the event */
    count_id = plugin_cntr_defines->vt_plugin_vector
      [VT_PLUGIN_CNTR_ASYNCH_POST_MORTEM][selected_counter].vt_counter_id;
    vt_count(
        plugin_cntr_defines->postmortem_dummy_thread_id,
        &(selected_time_value->timestamp),
        count_id,
        selected_time_value->value);

    last_timestamp=selected_time_value->timestamp;

    /* and move the pointer to the next element */
    current_time_values[selected_counter]++;

    /* but if there is no more data for this counter, we set it to NULL */
    if (time_values[selected_counter]+number_of_values[selected_counter]<=current_time_values[selected_counter])
      current_time_values[selected_counter]=NULL;
  }
    /* free allocated stuff */
    for (i=0;i<number_of_counters;i++){
      free(time_values[i]);
    }
    free(time_values);
    free(number_of_values);
}

int vt_plugin_cntr_is_registered_monitor_thread() {
  uint32_t i,j;
  /* check whether tracing this thread is allowed */
  for (j = 0; j < VT_PLUGIN_CNTR_SYNCH_TYPE_MAX; j++)
    for (i = 0; i < nr_plugins[j]; i++)
      /* function implemented? */
      if (vt_plugin_handles[j][i].info.is_thread_registered)
        /* does it return != 0 ? */
        if (vt_plugin_handles[j][i].info.is_thread_registered())
          return 1;
  return 0;
}
