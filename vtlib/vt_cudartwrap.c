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

#include "config.h"         /* snprintf */

#include "vt_defs.h"
#include "vt_env.h"         /* get environment variables */
#include "vt_pform.h"       /* VampirTrace time measurement */
#include "vt_inttypes.h"    /* VampirTrace integer types */
#include "vt_defs.h"        /* VampirTrace constants */
#include "vt_error.h"       /* VampirTrace warning and error messages */
#include "vt_libwrap.h"     /* wrapping of CUDA Runtime API functions */
#include "vt_cudartwrap.h"    /* CUDA wrapper functions for external use */
#include "vt_trc.h"         /* VampirTrace events */
#include "vt_thrd.h"        /* thread creation for CUDA kernels */
#include "vt_memhook.h"     /* Switch memory tracing on/off */

#include <stdio.h>
#include <string.h>

#include "vt_cuda_runtime_api.h" /* contains functions to be wrapped */

/* mutexes for locking the CUDA wrap environment */
#if (defined(VT_MT) || defined(VT_HYB))
static VTThrdMutex* VTThrdMutexCuda = NULL;
# define CUDARTWRAP_LOCK() VTThrd_lock(&VTThrdMutexCuda)
# define CUDARTWRAP_UNLOCK() VTThrd_unlock(&VTThrdMutexCuda)
#else /* VT_MT || VT_HYB */
# define CUDARTWRAP_LOCK()
# define CUDARTWRAP_UNLOCK()
#endif /* VT_MT || VT_HYB */

/* do some initialization before calling the first CUDA function */
#define CUDARTWRAP_FUNC_INIT(_lw, _lwattr, _func, _rettype, _argtypes, _file, \
                             _line)                                           \
  VT_LIBWRAP_FUNC_INIT(_lw, _lwattr, _func, _rettype, _argtypes, _file,       \
                       _line);                                                \
                                                                              \
  if(!initialized){                                                           \
    CUDARTWRAP_LOCK();                                                        \
    if(!initialized){                                                         \
      cudartwrap_init();                                                      \
      initialized = 1;                                                        \
    }                                                                         \
    CUDARTWRAP_UNLOCK();                                                      \
  }

#define CUDARTWRAP_FUNC_START(_lw) \
  if( trace_enabled ) VT_LIBWRAP_FUNC_START(_lw)

#define CUDARTWRAP_FUNC_END(_lw) \
  if( trace_enabled ) VT_LIBWRAP_FUNC_END(_lw)

/*
 * Macro for synchronous (blocking) CUDA Memory Copies.
 * VampirTrace communication events are written on host stream!!!
 * !!! DeviceToDevice copies are written on CUDA stream in between the
 *     asynchronous tasks. Therefore do only write them after a flush.
 *     Make sure to check in flush, if task is after last synchronous call!!!
 *
 * @param kind the cudaMemcpyKind
 * @param bytes the number of bytes to be transfered
 * @param call the function call of the CUDA Runtime API function
 */
#define CUDA_SEND_RECV(_kind, _bytes, _call){                                  \
  uint64_t time = 0;                                                           \
  uint8_t do_trace = 0;                                                        \
  VTCUDADevice* vtDev = NULL;                                                  \
  uint32_t strmID = 0;                                                         \
  uint32_t ptid = 0;                                                           \
  if(trace_enabled){                                                           \
    VTCUDAStrm *strm;                                                          \
    VT_CHECK_THREAD;                                                           \
    ptid = VT_MY_THREAD;                                                       \
    do_trace = vt_is_trace_on(ptid);                                           \
    if(do_trace){                                                              \
      vtDev = VTCUDAcheckThread(0, ptid, &strm);                               \
      strmID = strm->tid;                                                      \
      if(_kind != cudaMemcpyHostToHost){                                       \
        if(syncLevel > 2) VTCUDAflush(vtDev, ptid);                            \
        else if(syncLevel > 0){                                                \
          time = vt_pform_wtime();                                             \
          if(syncLevel > 1) vt_enter(ptid, &time, rid_sync);                   \
          checkCUDACall(cudaThreadSynchronize_ptr(),"vtcudaSync() failed!");   \
          if(syncLevel > 1){time = vt_pform_wtime(); vt_exit(ptid, &time);}    \
        }                                                                      \
        CUDARTWRAP_LOCK();                                                     \
          if(_kind != cudaMemcpyDeviceToDevice) gpuCommPIDs[ptid] = GPU_COMM;  \
          gpuCommPIDs[strmID] |= GPU_COMM;                                     \
        CUDARTWRAP_UNLOCK();                                                   \
      }                                                                        \
      if(syncLevel == 1 && time != 0){ /* no hostTohost and sync==1 */         \
        (void)vt_enter(ptid, &time, VT_LIBWRAP_FUNC_ID);                       \
        time = vt_pform_wtime();                                               \
      }else{                                                                   \
        time = vt_pform_wtime();                                               \
        (void)vt_enter(ptid, &time, VT_LIBWRAP_FUNC_ID);                       \
      }                                                                        \
      if(_kind == cudaMemcpyHostToDevice){                                     \
        vt_mpi_rma_put(ptid, &time, strmID * 65536 + vt_my_trace,              \
                       gpuCommCID, 0, _bytes);                                 \
      }else if(_kind == cudaMemcpyDeviceToHost){                               \
        vt_mpi_rma_get(ptid, &time, strmID * 65536 + vt_my_trace,              \
                       gpuCommCID, 0, _bytes);                                 \
      }else if(_kind == cudaMemcpyDeviceToDevice && syncLevel > 2){            \
        vt_mpi_rma_get(strmID, &time, strmID * 65536 + vt_my_trace,            \
                       gpuCommCID, 0, _bytes);                                 \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  _call  /* the CUDA memcpy call itself */                                     \
  if(trace_enabled && do_trace){                                               \
    time = vt_pform_wtime();                                                   \
      if(_kind == cudaMemcpyDeviceToDevice && syncLevel > 2){                  \
        vt_mpi_rma_end(strmID, &time, gpuCommCID, 0);                          \
      }else if(_kind != cudaMemcpyHostToHost){                                 \
        vt_mpi_rma_end(ptid, &time, gpuCommCID, 0);                            \
      }                                                                        \
      if(syncLevel > 2) vtDev->sync.lastTime = time;                           \
    vt_exit(ptid, &time);                                                      \
  }                                                                            \
}

/* Records a memory copy and stores it in the entry buffer.
 * VampirTrace communication events are written on CUDA stream!!!
 * @param kind kind/direction of memory copy
 * @param bytes number of bytes to be transfered
 * @param stream the CUDA stream
 * @param call the CUDA function call itsef
 */
#define CUDA_MEMCPY_ASYNC(kind, bytes, stream, call){                          \
  VTCUDAMemcpy *mcpy = NULL;                                                   \
  if(trace_enabled){                                                           \
    if((kind != cudaMemcpyHostToHost) && trace_memcpyAsync){                   \
      mcpy = addMemcpy2Buf(kind, bytes, stream);                               \
      /*vt_cntl_msg(1,"[CUDA] Asynchronous Memcpy on stream %d ", stream);*/\
      checkCUDACall(cudaEventRecord_ptr(mcpy->evt->strt, stream), NULL);       \
    }                                                                          \
    VT_LIBWRAP_FUNC_START(lw);                                                 \
  }                                                                            \
  call  /* the CUDA memCpy call itself */                                      \
  if(trace_enabled){                                                           \
    VT_LIBWRAP_FUNC_END(lw);                                                   \
    if((kind != cudaMemcpyHostToHost) && trace_memcpyAsync){                   \
      checkCUDACall(cudaEventRecord_ptr(mcpy->evt->stop, stream), NULL);       \
    }                                                                          \
  }                                                                            \
}

#define checkCUDACall(ecode, msg) __checkCUDACall(ecode, msg, __FILE__,__LINE__)

#define KERNEL_STR_SIZE 100

/* number of bytes the buffer for asynchronous tasks has */
#define DEF_ASYNC_BSIZE 8192  /* bytes */
#define MIN_ASYNC_ENTRY sizeof(VTCUDAMemcpy)    /* bytes */
/* cudaEventCreate  takes more than a second for buffer > X */
#define MAX_ASYNC_BSIZE 2097152 /* 8192^8 bytes */

/* defines for GPU GROUP and GPU COMM */
#define NO_GPU    0x00  /* thread is no gpu and does no gpu communication */
#define GPU       0x01  /* thread is a gpu */
#define GPU_COMM  0x02  /* thread does gpu communication (CPU or GPU) */

/* gobal communicator id for all GPU threads */
static uint32_t gpuGroupCID;

/* communicator for all node local threads communicating with GPU */
static uint32_t gpuCommCID;

/* Process/Thread IDs, which participate in gpu communication.
 * Index of the list is the thread ID (VTThrd...)
 */
static uint8_t *gpuCommPIDs;

/* library wrapper object */
static VTLibwrap* lw = VT_LIBWRAP_NULL;

/* library wrapper attributes */
static void cudartwrap_lw_attr_init(VTLibwrapAttr* attr);
static VTLibwrapAttr lw_attr =
  VT_LIBWRAP_ATTR_INITIALIZER(cudartwrap_lw_attr_init);

/* flag: cuda specific stuff initialized? */
static uint8_t initialized = 0;

/* flag: tracing of CUDA API enabled? */
static uint8_t trace_enabled = 0;

/* flag: write GPU idle time as region in CUDA stream 0? */
static uint8_t show_gpu_idle = 0;

/* flag: synchronization and flush points during runtime enabled? */
static uint8_t syncLevel = 1;

/* flag: tracing of kernels enabled? */
static uint8_t trace_kernels = 1;

/* flag: tracing of asynchronous memory copies enabled? */
static uint8_t trace_memcpyAsync = 1;

/* flag: event based tracing (kernels, memcpyAsync) enabled? */
static uint8_t trace_events = 1;

/* number of bytes used to buffer asynchronous tasks */
static size_t asyncBufSize = MAX_ASYNC_BSIZE;

/* flag: CUDA wrapper already finalized? */
static uint8_t finalized = 0;

/* global region IDs for wrapper internal tracing */
static uint32_t rid_check, rid_create, rid_sync, rid_flush;
static uint32_t rid_idle = VT_NO_ID;

/* structure for VampirTrace - CUDA time synchronization */
typedef struct
{
  cudaEvent_t strtEvt;   /**< the start event */
  cudaEvent_t stopEvt;   /**< the stop event */
  uint64_t strtTime;     /**< VampirTrace start time */
  uint64_t lastTime;     /**< VampirTrace time after cuda memory copy */
}VTCUDAsync;

/* structure of a VampirTrace CUDA stream */
typedef struct vtcudaStream
{
  cudaStream_t stream;         /**< the CUDA stream */
  uint32_t tid;                /**< VT thread id for this stream (unique) */
  cudaEvent_t lastEvt;         /**< last written CUDA event (needed in flush) */
  uint64_t lastVTTime;         /**< last written VampirTrace time */
  struct vtcudaStream *next;   /**< points to next cuda stream in list */
}VTCUDAStrm;

typedef enum{
  VTCUDABUF_ENTRY_TYPE__Kernel,
	VTCUDABUF_ENTRY_TYPE__Memcpy
} VTCUDABuf_EntryTypes;

typedef struct
{
  cudaEvent_t strt;    /**< the start event */
  cudaEvent_t stop;    /**< the stop event */
} VTCUDABufEvt;

/* basic CUDA task buffer entry */
typedef struct
{
  VTCUDABuf_EntryTypes type;  /**< type of buffer entry */
  VTCUDAStrm *strm;           /**< corresponding stream/thread */
  VTCUDABufEvt *evt;          /**< points to start/stop cuda event */
} VTCUDAbufEntry;

/* structure for a CUDA kernel call */
typedef struct
{
  VTCUDABuf_EntryTypes type;  /**< type of buffer entry */
  VTCUDAStrm *strm;           /**< corresponding stream/thread */
  VTCUDABufEvt *evt;          /**< points to start/stop cuda event */
  uint32_t rid;               /**< VampirTrace region id */
}VTCUDAKernel;

/* structure for a CUDA kernel call */
typedef struct
{
  VTCUDABuf_EntryTypes type; /**< type of buffer entry */
  VTCUDAStrm *strm;          /**< the corresponding stream/thread */
  VTCUDABufEvt *evt;         /**< points to start/stop cuda event */
  uint32_t pid;              /**< the callers process/thread id */
  enum cudaMemcpyKind kind;  /**< CUDA memory copy kind (e.g. host->device) */
  size_t byteCount;          /**< number of bytes */
}VTCUDAMemcpy;

/* structure for a CUDA device */
typedef struct
{
  int device;                /**< CUDA device id (first key) */
  uint32_t ptid;             /**< the host thread id (second key) */
  uint8_t concurrentKernels; /**< is concurrent kernel execution supported? */
  VTCUDAStrm *strmList;      /**< CUDA stream list */
  VTCUDAsync sync;           /**< synchronization time and events */
  buffer_t asyncbuf;         /**< points to the first byte in buffer */
  buffer_t buf_pos;          /**< current buffer position */
  buffer_t buf_size;         /**< buffer size (in bytes) */
  VTCUDABufEvt *evtbuf;      /**< the preallocated cuda event list */
  VTCUDABufEvt *evtbuf_pos;  /**< current unused event space */
}VTCUDADevice;
/* vector of cuda nodes */
static VTCUDADevice** cudaDevices = NULL;

/* number of traced cuda devices (has to be locked!!!) */
static uint32_t gpuNum = 0;

/* maximum events needed for task buffer size */
static size_t maxEvtNum = MAX_ASYNC_BSIZE / sizeof(VTCUDAKernel);

/* pointer to cuda functions which should not be traced */
static cudaError_t (*cudaGetDeviceCount_ptr)(int*) = VT_LIBWRAP_NULL;
static cudaError_t (*cudaGetDevice_ptr)(int*) = VT_LIBWRAP_NULL;
static cudaError_t (*cudaGetDeviceProperties_ptr)(struct cudaDeviceProp *, int) = VT_LIBWRAP_NULL;
static cudaError_t (*cudaEventCreate_ptr)(cudaEvent_t *) = VT_LIBWRAP_NULL;
/*static cudaError_t (*cudaEventCreateWithFlags_ptr)(cudaEvent_t *, int) = VT_LIBWRAP_NULL;*/
static cudaError_t (*cudaEventRecord_ptr)(cudaEvent_t, cudaStream_t) = VT_LIBWRAP_NULL;
static cudaError_t (*cudaEventSynchronize_ptr)(cudaEvent_t) = VT_LIBWRAP_NULL;
static cudaError_t (*cudaEventElapsedTime_ptr)(float *, cudaEvent_t, cudaEvent_t) = VT_LIBWRAP_NULL;
static cudaError_t (*cudaEventDestroy_ptr)(cudaEvent_t) = VT_LIBWRAP_NULL;
/*static cudaError_t (*cudaStreamSynchronize_ptr)(cudaStream_t) = VT_LIBWRAP_NULL;*/
static cudaError_t (*cudaThreadSynchronize_ptr)(void) = VT_LIBWRAP_NULL;
static cudaError_t (*cudaGetLastError_ptr)(void) = VT_LIBWRAP_NULL;
static const char *(*cudaGetErrorString_ptr)(cudaError_t) = VT_LIBWRAP_NULL;

/* 
 * CUDA wrapper function declarations
 */
static void VTCUDAflush(VTCUDADevice*, uint32_t);

/* Checks if a CUDA runtime API call returns successful and respectively prints
 * the error.
 * @param ecode the CUDA error code
 * @param msg a message to get more detailled information about the error
 * @param the corresponding file
 * @param the line the error occured
 */
static void __checkCUDACall(cudaError_t ecode, const char* msg,
                                   const char *file, const int line)
{
  if(cudaSuccess != ecode){
    if(msg != NULL) vt_cntl_msg(1,msg);
    vt_error_msg("[CUDA Error <%s>:%i] %s", file, line,
                 cudaGetErrorString_ptr(ecode));
  }
}

/*
 * initializer function for library wrapper attributes
   (called from first triggered wrapper event)
 */
static void cudartwrap_lw_attr_init(VTLibwrapAttr* attr)
{
  /* initialize library wrapper attributes */
  attr->shlibs_num = 0;
#ifdef DEFAULT_CUDARTLIB_PATHNAME
  attr->shlibs_num = 1;
  attr->shlibs[0] = DEFAULT_CUDARTLIB_PATHNAME;
#endif /* DEFAULT_CUDARTLIB_PATHNAME */
  attr->func_group = "CUDART_API";
  attr->wait_for_init = 0;

  /* *** do some additional initialization *** */
  /* create mutexes for locking */
#if (defined(VT_MT) || defined (VT_HYB))
  VTThrd_createMutex(&VTThrdMutexCuda);
#endif /* VT_MT || VT_HYB */
}

/* 
 * Initialization after first CUDA wrapper function init
 */
static void cudartwrap_init(void)
{
  /* general CUDA tracing enabled? */
  trace_enabled = (uint8_t)vt_env_cudarttrace();

  if(trace_enabled){
    int deviceCount;
    size_t minTaskSize = sizeof(VTCUDAKernel) + sizeof(VTCUDAMemcpy);

    syncLevel = (uint8_t)vt_env_cudatrace_sync();
    trace_kernels = (uint8_t)vt_env_cudatrace_kernel();
    trace_memcpyAsync = (uint8_t)vt_env_cudatrace_memcpyasync();

    trace_events = 0;
    if(trace_kernels){
      minTaskSize = sizeof(VTCUDAKernel);
      trace_events = 1;
    }

    if(trace_memcpyAsync){
      if(sizeof(VTCUDAMemcpy) < minTaskSize) minTaskSize = sizeof(VTCUDAMemcpy);
      trace_events = 1;
    }

    /* if events are used */
    if(trace_events){
      /* get user-defined task buffer size and check it */
      asyncBufSize = vt_env_cudatrace_bsize();
      if(asyncBufSize < MIN_ASYNC_ENTRY){
        if(asyncBufSize > 0){
          vt_warning("[CUDA] Minimal buffer size is %d bytes", MIN_ASYNC_ENTRY);
        }
        asyncBufSize = DEF_ASYNC_BSIZE;
      }else if(MAX_ASYNC_BSIZE < asyncBufSize){
        vt_warning("[CUDA] Current CUDA buffer size requires %d CUDA events.\n"
                   "The recommended max. CUDA buffer size is %d. "
                   "(export VT_CUDA_BUFFER_SIZE=2097152)",
                   2*asyncBufSize/sizeof(VTCUDAKernel), MAX_ASYNC_BSIZE);
        /* TODO: dynamic event creation for more than 2097152 bytes cuda buffer size */
      }
      vt_cntl_msg(2,"[CUDA] current cuda buffer size: %d bytes \n"
                    "(Kernel: %d bytes, MemcpyAsync: %d bytes)",
                  asyncBufSize, sizeof(VTCUDAKernel), sizeof(VTCUDAMemcpy));

      /* determine maximum necessary VT-events (=2 CUDA events) */
      maxEvtNum = asyncBufSize / minTaskSize;
    }

    show_gpu_idle = (uint8_t)vt_env_cudatrace_idle() & trace_kernels;

    /* initialize CUDA functions for not traced internal use */
    {
      static int func_id = VT_LIBWRAP_NOID;
      
      VTLibwrap_func_init(lw, "cudaGetDeviceCount", NULL, 0,
                          (void**)(&cudaGetDeviceCount_ptr), &func_id);
      VTLibwrap_func_init(lw, "cudaGetDevice", NULL, 0,
                          (void**)(&cudaGetDevice_ptr), &func_id);
      VTLibwrap_func_init(lw, "cudaGetDeviceProperties", NULL, 0,
                          (void**)(&cudaGetDeviceProperties_ptr), &func_id);
      VTLibwrap_func_init(lw, "cudaEventCreate", NULL, 0,
                          (void**)(&cudaEventCreate_ptr), &func_id);
      /*VTLibwrap_func_init(lw, "cudaEventCreateWithFlags", NULL, 0,
                          (void**)(&cudaEventCreateWithFlags_ptr), &func_id);*/
      VTLibwrap_func_init(lw, "cudaEventRecord", NULL, 0,
                          (void**)(&cudaEventRecord_ptr), &func_id);
      VTLibwrap_func_init(lw, "cudaEventSynchronize", NULL, 0,
                          (void**)(&cudaEventSynchronize_ptr), &func_id);
      VTLibwrap_func_init(lw, "cudaEventElapsedTime", NULL, 0,
                          (void**)(&cudaEventElapsedTime_ptr), &func_id);
      VTLibwrap_func_init(lw, "cudaEventDestroy", NULL, 0,
                          (void**)(&cudaEventDestroy_ptr), &func_id);
      /*VTLibwrap_func_init(lw, "cudaStreamSynchronize", NULL, 0,
                          (void**)(&cudaStreamSynchronize_ptr), &func_id);*/
      VTLibwrap_func_init(lw, "cudaThreadSynchronize", NULL, 0,
                          (void**)(&cudaThreadSynchronize_ptr), &func_id);
      VTLibwrap_func_init(lw, "cudaGetErrorString", NULL, 0,
                          (void**)(&cudaGetErrorString_ptr), &func_id);
      VTLibwrap_func_init(lw, "cudaGetLastError", NULL, 0,
                          (void**)(&cudaGetLastError_ptr), &func_id);
    }

    /* create vector for all available CUDA devices */
    checkCUDACall(cudaGetDeviceCount_ptr(&deviceCount),
                  "cudaGetDeviceCount(int *count) failed!");

    /* allocate memory for CUDA nodes and kernels */
    if(cudaDevices == NULL){
      cudaDevices = (VTCUDADevice**)calloc(deviceCount, sizeof(VTCUDADevice*));
      if(cudaDevices == NULL) vt_error_msg("calloc for CUDA devices failed!");
    }

    /* create group property list for threads */
    gpuCommPIDs = (uint8_t*)calloc(VTThrdMaxNum, sizeof(uint8_t));

#if (defined(VT_MT) || defined(VT_HYB))
    VTTHRD_LOCK_IDS();
#endif
    /* get a communicator id for GPU communication */
    gpuCommCID = vt_get_curid();
    gpuGroupCID = vt_get_curid();

    /* get global region IDs for wrapper internal tracing */
    if(show_gpu_idle){
      rid_idle = vt_def_region(VT_MASTER_THREAD, "gpu_idle", VT_NO_ID,
                               VT_NO_LNO, VT_NO_LNO, "CUDA_IDLE", VT_FUNCTION);
    }
    rid_check = vt_def_region(VT_MASTER_THREAD, "vtcudaCheckThread", VT_NO_ID,
                              VT_NO_LNO, VT_NO_LNO, "VT_CUDA", VT_FUNCTION);
    rid_create = vt_def_region(VT_MASTER_THREAD, "vtcudaCreateDevice", VT_NO_ID,
                               VT_NO_LNO, VT_NO_LNO, "VT_CUDA", VT_FUNCTION);
    rid_sync = vt_def_region(VT_MASTER_THREAD, "vtcudaSynchronize", VT_NO_ID,
                             VT_NO_LNO, VT_NO_LNO, "CUDA_SYNC", VT_FUNCTION);
    rid_flush = vt_def_region(VT_MASTER_THREAD, "vtcudaFlush", VT_NO_ID, VT_NO_LNO,
                              VT_NO_LNO, "VT_CUDA", VT_FUNCTION);
#if (defined(VT_MT) || defined(VT_HYB))
    VTTHRD_UNLOCK_IDS();
#endif

    /* register the finalize function of the CUDA wrapper to be called before
     * the program exits and CUDA has done its implizit clean-up */
    atexit(vt_cudartwrap_finalize);
  }
}

/*
 * Creates process groups for all GPU threads in trace and groups for threads,
 * which participate in GPU communication.
 */
static void createGPUGroups(void)
{
  uint32_t i, ctrGPUGroup, ctrGPUComm;

  ctrGPUGroup = 0;
  ctrGPUComm = 0;

  /* get number of GPU communication threads and gpu threads to determine
     array size */
  for(i = 0; i < VTThrdn; i++){
    if((gpuCommPIDs[i] & GPU_COMM) == GPU_COMM) ctrGPUComm++;
    if((gpuCommPIDs[i] & GPU) == GPU) ctrGPUGroup++;
  }

  /* create array of GPU communication threads and define group */
  if(ctrGPUComm > 0){
    uint32_t *gpu_comm_array = (uint32_t*)malloc(ctrGPUComm*sizeof(uint32_t));
    int j = 0;
    for(i = 0; i < VTThrdn; i++){
      if((gpuCommPIDs[i] & GPU_COMM) == GPU_COMM){
        gpu_comm_array[j++] = i;
      }
    }
    vt_def_gpu_comm(ctrGPUComm, gpu_comm_array, "__GPU_COMM__", gpuCommCID);
  }

  /* create array of GPU threads and define group */
  if(ctrGPUGroup > 0){
    uint32_t *gpu_group_array = (uint32_t*)malloc(ctrGPUGroup*sizeof(uint32_t));
    int j = 0;
    for(i = 0; i < VTThrdn; i++){
      if((gpuCommPIDs[i] & GPU) == GPU){
        gpu_group_array[j++] = i;
      }
    }
    vt_def_gpu_comm(ctrGPUGroup, gpu_group_array, "__GPU_GROUP__", gpuGroupCID);
  }
}

/*
 * Free the local allocated VampirTrace structure and set the VT device to NULL.
 * Has to be locked!!!
 *
 * @param vtDev pointer to the VampirTrace CUDA device
 */
static void VTCUDAremoveDeviceFromList(VTCUDADevice *vtDev)
{
  uint32_t i;

  for(i = 0; i < gpuNum; i++){
    if(vtDev == cudaDevices[i]){
      free(vtDev);
      cudaDevices[i] = NULL;
      return;
    }
  }
}

/*
 * Cleans up the structure of a VampirTrace CUDA device.
 *
 * @param ptid the VampirTrace thread, which executes this cleanup
 * @param vtDev pointer to VampirTrace CUDA device structure to be cleaned up
 * @param cleanEvents cleanup CUDA events? 1 - yes, 0 - no
 */
static void VTCUDAcleanupDevice(uint32_t ptid, VTCUDADevice *vtDev,
                                uint8_t cleanEvents)
{
  /* check if device already cleanup (e.g. with cudaThreadExit() call) */
  if(vtDev == NULL) return;

  /* flush kernels, if possible */
  if(cleanEvents){
    if(cudaEventQuery(vtDev->sync.strtEvt) == cudaErrorInvalidResourceHandle){
      cleanEvents = 0;
      vt_warning("[CUDA] Events are invalid. Context has been destroyed, \n"
                 "before asynchronous tasks could be flushed! "
                 "Traces might be incomplete!");
    }else{
      VTCUDAflush(vtDev, ptid);
    }
  }

  /* write idle end time to CUDA stream 0 */
  if(show_gpu_idle == 1){
    uint64_t idle_end = vt_pform_wtime();
    vt_exit(vtDev->strmList->tid, &idle_end);
  }

  /* cleanup stream list */
  if(vtDev->strmList != NULL){
    free(vtDev->strmList);
    vtDev->strmList = NULL;
  }

  if(trace_events){
    /* destroy CUDA events (cudaThreadExit() implicitly destroys events) */
    if(vtDev->evtbuf != NULL){
      size_t k;
      /* destroy CUDA events for asynchronous task measurement */
      if(cleanEvents){
        checkCUDACall(cudaGetLastError_ptr(), "Error check before event destroy");
        for(k = 0; k < maxEvtNum; k++){
          cudaEventDestroy_ptr((vtDev->evtbuf[k]).strt);
          cudaEventDestroy_ptr((vtDev->evtbuf[k]).stop);
        }
        checkCUDACall(cudaGetLastError_ptr(), "cudaEventDestroy failed");
      }

      /* free the event buffer */
      free(vtDev->evtbuf);
      vtDev->evtbuf = NULL;
      vtDev->evtbuf_pos = NULL;
    }

    /* destroy synchronization events */
    if(cleanEvents){
      checkCUDACall(cudaEventDestroy_ptr(vtDev->sync.strtEvt),
                    "cudaEventDestroy(syncStrtEvt) failed!");
      checkCUDACall(cudaEventDestroy_ptr(vtDev->sync.stopEvt),
                    "cudaEventDestroy(syncStopEvt) failed!");
    }
  
    /* cleanup entry buffer */
    if(vtDev->asyncbuf != NULL){
      free(vtDev->asyncbuf);
      vtDev->asyncbuf = NULL;
    }
  }

  /* free malloc of VTCUDADevice, set pointer to this VT device NULL */
  VTCUDAremoveDeviceFromList(vtDev);
}

/*
 * Cleanup CUDA wrapper
 */
void vt_cudartwrap_finalize(void)
{
  if(!finalized){
    CUDARTWRAP_LOCK();
    if(!finalized){
      if(trace_enabled){
        uint32_t i, ptid;

        createGPUGroups();

        VT_CHECK_THREAD;
        ptid = VT_MY_THREAD;

        /* for every CUDA device */
        for(i = 0; i < gpuNum; i++){
          VTCUDADevice *vtDev = cudaDevices[i];
          int device;

          if(vtDev == NULL) continue;

          checkCUDACall(cudaGetDevice_ptr(&device), "cudaGetDevice(device) failed!");
          if(vtDev->device == device && vtDev->ptid == ptid){
            VTCUDAcleanupDevice(ptid, vtDev, 1);
          }else{
            if(vtDev->buf_pos != vtDev->asyncbuf){
              vt_warning("[CUDA] Current device is %d (Thread %d).\n"
                         "Can neither flush asynchronous tasks nor cleanup resources for device %d with thread %d!\n"
                         "To avoid this warning call cudaThreadExit() before the host thread, using CUDA, exits.",
                         device, ptid, vtDev->device, vtDev->ptid);
            }
            VTCUDAcleanupDevice(ptid, vtDev, 0);
          }
          cudaDevices[i] = NULL;
        }

        /* cleanup GPU device list */
        if(cudaDevices != NULL){
          free(cudaDevices);
          cudaDevices = NULL;
        }
      }
      finalized = 1;
      CUDARTWRAP_UNLOCK();
#if (defined(VT_MT) || defined (VT_HYB))
      VTTHRD_LOCK_ENV();
      VTThrd_deleteMutex(&VTThrdMutexCuda);
      VTTHRD_UNLOCK_ENV();
#endif /* VT_MT || VT_HYB */
    }
  }
}

/* 
 * Create a synchronization point for CUDA and VampirTrace time.
 *
 * @param syncEvt the CUDA event to be synchronized
 *
 * @return the corresponding VampirTrace timestamp
 */
static uint64_t VTCUDAsynchronizeEvt(cudaEvent_t syncEvt)
{
  uint64_t syncTime;
  cudaError_t ret;

  /* Record and synchronization events on stream 0 (prior to FERMI)
     see NVIDIA CUDA Programming Guide 2.3, sections 3.2.6.1 and 3.2.6.2
     -> "Events in stream zero are recorded after all preceding tasks/commands
         from all streams are completed by the device." */
  cudaEventRecord_ptr(syncEvt, 0);
  /*ret = cudaThreadSynchronize_ptr();*/
  ret = cudaEventSynchronize_ptr(syncEvt);
  syncTime = vt_pform_wtime();
  
  /* error handling */
  if(cudaSuccess != ret){
    if(cudaErrorInvalidResourceHandle == ret){
      vt_warning("[CUDA] Synchronization stop event is invalid. Context has "
                 "been destroyed, \nbefore asynchronous tasks could be flushed! "
                 "Traces might be incomplete!");
      return (uint64_t)-1;
    }else{
      vt_error_msg("[CUDA Error <%s>:%i] %s",  __FILE__,__LINE__,
                   cudaGetErrorString_ptr(ret));
    }
  }

  return syncTime;
}

/* 
 * Write asynchronous CUDA tasks to VampirTrace CUDA thread/stream
 *
 * @param node pointer to the VTCUDADevice to be flushed
 */
static void VTCUDAflush(VTCUDADevice *vtDev, uint32_t ptid)
{
  uint64_t flush_time, syncStopTime;
  float diff_flush_ms;
  VTCUDAsync *sync;

  /* check if device available */
  if(vtDev == NULL) return;
  /* check if buffer entries available */
  if(vtDev->buf_pos == vtDev->asyncbuf) return;

  sync = &(vtDev->sync);

  /* trace the synchronization (this is no VampirTrace overhead!!!) */
  flush_time = vt_pform_wtime();
  vt_enter(ptid, &flush_time, rid_sync);
    syncStopTime = VTCUDAsynchronizeEvt(sync->stopEvt);
    if(syncStopTime == (uint64_t)-1) return;
  vt_exit(ptid, &syncStopTime);

  /* trace the flush itself (VampirTrace overhead) */
  vt_enter(ptid, &syncStopTime, rid_flush);

  /* get time between syncStrtEvt and syncStopEvt */
  checkCUDACall(cudaEventElapsedTime_ptr(&diff_flush_ms, sync->strtEvt,
                                         sync->stopEvt),
                "cudaEventElapsedTime(float *, cudaEvent_t syncStrtEvt, "
                "cudaEvent_t syncStopEvt) failed!");

  vt_cntl_msg(3, "Time between CUDA syncEvts: %f ms (%llu ticks)",
                 diff_flush_ms, syncStopTime-sync->strtTime);

  /* if copy time not yet set, e.g. flush limit reached */
  if(sync->strtTime > sync->lastTime){
    sync->lastTime = sync->strtTime;
    vt_cntl_msg(1,"This should not appear!");
  }

  /* set the synchronization start point (for all streams), no asynchronous
   * task may begin before this time */
  {
    VTCUDAStrm *curStrm = vtDev->strmList;
    do{
      curStrm->lastEvt = sync->strtEvt;
      curStrm->lastVTTime = sync->strtTime;
      curStrm = curStrm->next;
    }while(curStrm != NULL);
  }

  {
    uint64_t serialKernelTime = 0;

    /* conversion factor between VampirTrace and CUDA time */
    const double factorX = (double)(syncStopTime - sync->strtTime)/
                           (double)diff_flush_ms;

    /* write events for all recorded asynchronous calls */
    buffer_t entry = vtDev->asyncbuf;
    while(entry < vtDev->buf_pos){
      uint64_t strttime, stoptime; /* will be written in vt_enter/vt_exit */
      VTCUDAbufEntry *bufEntry = (VTCUDAbufEntry*)entry;
      uint32_t tid = bufEntry->strm->tid;

      /* get VampirTrace start and stop timestamp (in: bufEntry, minStrtTS, factorX)*/
      {
        VTCUDAStrm *strm = bufEntry->strm;
        float diff_ms;
        
        /* time between synchronize start event and kernel start event */
        checkCUDACall(cudaEventElapsedTime_ptr(&diff_ms, strm->lastEvt, bufEntry->evt->strt),
                      "cudaEventElapsedTime(diff, tmpStrtEvt, knStrtEvt) failed!");

        /* convert CUDA kernel start event to VampirTrace timestamp */
        strttime = strm->lastVTTime + (uint64_t)((double)diff_ms * factorX);

        /* check if kernel start time is before last synchronous CUDA call */
        if(strttime < sync->lastTime){
          strttime = sync->lastTime;
          vt_warning("[CUDA] event before last synchronous CUDA call measured!");
        }

        /* time between kernel start event and kernel stop event */
        checkCUDACall(cudaEventElapsedTime_ptr(&diff_ms, bufEntry->evt->strt, bufEntry->evt->stop),
                      "cudaEventElapsedTime(diff, knStrtEvt, knStopEvt) failed!");

        /* convert CUDA kernel stop event to VampirTrace timestamp */
        stoptime = strttime + (uint64_t)((double)diff_ms * factorX);

        if(stoptime > syncStopTime){
          stoptime = syncStopTime;
          if(strttime > syncStopTime){
            strttime = syncStopTime;
          }
          vt_warning("[CUDA] time measurement of kernel or memcpyAsync failed!");
        }

        /* set new synchronized CUDA start event and VampirTrace start timestamp,
           which keeps period small and reduces conversion errors (casts) */
        strm->lastVTTime = stoptime;
        strm->lastEvt = bufEntry->evt->stop;
      }
      
      if(bufEntry->type == VTCUDABUF_ENTRY_TYPE__Kernel){
        VTCUDAKernel *kn = (VTCUDAKernel*)entry;

        /* CUDA devices prior to FERMI only allow execution of one kernel */
        if(strttime < serialKernelTime && vtDev->concurrentKernels == 0){
          strttime = serialKernelTime;
        }
        serialKernelTime = stoptime;

        /* write VampirTrace events to CUDA threads */
        /* gpu idle time will be written to first cuda stream in list */
        if(show_gpu_idle) vt_exit(vtDev->strmList->tid, &strttime);
        vt_enter(tid, &strttime, kn->rid);
        vt_exit(tid, &stoptime);
        if(show_gpu_idle) vt_enter(vtDev->strmList->tid, &stoptime, rid_idle);

        /* go to next entry in buffer */
        entry += sizeof(VTCUDAKernel);
      }else if(bufEntry->type == VTCUDABUF_ENTRY_TYPE__Memcpy){
        /* write communication (in: mcpy, strttime, stoptime) */
        VTCUDAMemcpy *mcpy = (VTCUDAMemcpy*)entry;

        if(mcpy->kind == cudaMemcpyHostToDevice){
          vt_mpi_rma_get(tid, &strttime, mcpy->pid * 65536 + vt_my_trace,
                          gpuCommCID, 0, mcpy->byteCount);
        }else if(mcpy->kind == cudaMemcpyDeviceToHost){
          vt_mpi_rma_put(tid, &strttime, mcpy->pid * 65536 + vt_my_trace,
                          gpuCommCID, 0, mcpy->byteCount);
        }else if(mcpy->kind == cudaMemcpyDeviceToDevice){
          vt_mpi_rma_get(tid, &strttime, tid * 65536 + vt_my_trace,
                          gpuCommCID, 0, mcpy->byteCount);
       }

        vt_mpi_rma_end(tid, &stoptime, gpuCommCID, 0);

        /* go to next entry in buffer */
        entry += sizeof(VTCUDAMemcpy);
      }

    }
  }

  /* set new syncStrtTime and syncStrtEvt */
  {
    cudaEvent_t tmp_Evt = sync->strtEvt;
    sync->strtEvt = sync->stopEvt;
    sync->stopEvt = tmp_Evt;
  }
  sync->strtTime = syncStopTime;
  sync->lastTime = syncStopTime;

  /* reset entry and event buffer */
  vtDev->buf_pos = vtDev->asyncbuf;
  vtDev->evtbuf_pos = vtDev->evtbuf;

  flush_time = vt_pform_wtime();
  vt_exit(ptid, &flush_time);
}

/* 
 * Uses VampirTrace Thread API to create a CUDA Thread.
 *
 * @param tname the name of the thread to be registered
 * @param ptid the parent thread id
 * @param vt_tid pointer to the thread id of the thread to be registered
 */
static void VTCUDAregisterThread(const char* tname, uint32_t ptid,
                                 uint32_t *vt_tid)
{
  if(!vt_is_alive){
    vt_cntl_msg(2, "VampirTrace is not alive. No virtual thread created.\n "
                   "Writing events on master thread (0)");
    return;
  }

  /* create new thread object */
  *vt_tid = VTThrd_createNewThreadId();
  vt_cntl_msg(2,"Creating thread '%s' with id: %d",tname , *vt_tid);
  VTThrd_create(*vt_tid, ptid, tname, 1);
  VTThrd_open(*vt_tid);
}

/**
 * Creates a VampirTrace CUDA stream object and returns it.
 *
 *  @param device the CUDA device id this stream is created for
 *  @param stream the CUDA stream id
 *  @param ptid the VampirTrace thread ID of the calling thread
 *
 *  @return the created stream object
 */
static VTCUDAStrm* VTCUDAcreateStream(int device, cudaStream_t stream,
                                      uint32_t ptid)
{
  uint32_t gpu_tid = 0;
  char thread_name[16];
  VTCUDAStrm *nstrm;

  /* allocate memory for stream */
  nstrm = (VTCUDAStrm*) malloc(sizeof (VTCUDAStrm));
  if(nstrm == NULL) vt_error_msg("malloc(sizeof(VTCUDAStrm)) failed!");

  nstrm->stream = stream;
  /*nstrm->lastEvt = -1;*/
  nstrm->lastVTTime = 0;
  nstrm->next = NULL;

  /* create VT-User-Thread with name and parent id and get its id */
  /*if(-1 == snprintf(thread_name, 15, "CUDA[%d:%d]", device, (int)stream))*/
  if(-1 == snprintf(thread_name, 15, "CUDA[%d]", device))
    vt_cntl_msg(1, "Could not create thread name for CUDA thread!");
  VTCUDAregisterThread(thread_name, ptid, &gpu_tid);
  nstrm->tid = gpu_tid;

  /* set the threads property to GPU */
  CUDARTWRAP_LOCK();
    gpuCommPIDs[gpu_tid] = GPU;
  CUDARTWRAP_UNLOCK();

  return nstrm;
}

/*
 * Creates a VTCUDADevice and returns a pointer to the created object.
 *
 * @param device the CUDA device id
 *
 * @return VampirTrace CUDA device object
 */
static VTCUDADevice* VTCUDAcreateDevice(uint32_t ptid, int device)
{
  VTCUDADevice *vtDev = (VTCUDADevice*)malloc(sizeof(VTCUDADevice));
  if(vtDev == NULL) vt_error_msg("Could not allocate memory for VTCUDADevice!");
  vtDev->device = device;
  vtDev->ptid = ptid;
  vtDev->asyncbuf = NULL;
  vtDev->buf_pos = NULL;
  vtDev->buf_size = NULL;
  vtDev->evtbuf = NULL;
  vtDev->evtbuf_pos = NULL;
  vtDev->strmList = NULL;

#if (defined(CUDART_VERSION) && (CUDART_VERSION >= 3000))
  /* get compute capability of CUDA device */
  {
    struct cudaDeviceProp deviceProp;
    cudaGetDeviceProperties_ptr(&deviceProp, device);
    vtDev->concurrentKernels = (uint8_t)deviceProp.concurrentKernels;
  }
#else
    vtDev->concurrentKernels = 0;
#endif

  /* async buffer or events may not be used */
  if(trace_events){
    /* --- set VampirTrace - CUDA time synchronization --- */
    checkCUDACall(cudaEventCreate_ptr(&(vtDev->sync.strtEvt)),
                  "cudaEventCreate(syncStrtEvt) failed!");

    checkCUDACall(cudaEventCreate_ptr(&(vtDev->sync.stopEvt)),
                  "cudaEventCreate(syncStopEvt) failed!");

    /* record init event for later synchronization with VampirTrace time */
    vtDev->sync.strtTime = VTCUDAsynchronizeEvt(vtDev->sync.strtEvt);

    /* set initial memory copy timestamp, if no memory copies are done */
    vtDev->sync.lastTime = vtDev->sync.strtTime;

    /* allocate buffers for asynchronous entries */
    {
      size_t i;

      vtDev->asyncbuf = malloc(asyncBufSize);
      if(vtDev->asyncbuf == NULL){
        vt_error_msg("malloc of asynchronous CUDA call buffer failed! "
                    "Reduce buffer size with VT_BUFFER_SIZE!");
      }
      vtDev->buf_pos = vtDev->asyncbuf;
      vtDev->buf_size = vtDev->asyncbuf + asyncBufSize;

      vtDev->evtbuf = calloc(maxEvtNum, sizeof(VTCUDABufEvt));
      vtDev->evtbuf_pos = vtDev->evtbuf;

      /* create CUDA events */
      for(i = 0; i < maxEvtNum; i++){
        cudaEventCreate_ptr(&((vtDev->evtbuf[i]).strt));
        cudaEventCreate_ptr(&((vtDev->evtbuf[i]).stop));
      }
      checkCUDACall(cudaGetLastError_ptr(), "cudaEventCreate failed");
    }/* ------ */
  }

  return vtDev;
}

/*
 * Invokes the device creation for VTCUDA.
 *
 * @param ptid the host process/thread id
 * @param cudaDev the CUDA device identifier
 *
 * @return the VampirTrace CUDA device structure
 */
static VTCUDADevice* VTCUDAinitDevice(uint32_t ptid, int cudaDev)
{
  uint64_t time;
  VTCUDADevice *vtDev = NULL;

  /* cuda device not found, create new cuda device node */
  time = vt_pform_wtime();
  vt_enter(ptid, &time, rid_create);
  vtDev = VTCUDAcreateDevice(ptid, cudaDev);

  time = vt_pform_wtime();
  vt_exit(ptid, &time);

  /* set the current stream (stream 0) */
  vtDev->strmList = VTCUDAcreateStream(cudaDev, 0, ptid);

  /* write enter event for GPU_IDLE on stream 0 (has to be written first */
  if(show_gpu_idle == 1) vt_enter(vtDev->strmList->tid, &vt_start_time, rid_idle);

  /* add thread to CUDA device node list */
  CUDARTWRAP_LOCK();
    cudaDevices[gpuNum] = vtDev;
    gpuNum++;
  CUDARTWRAP_UNLOCK();

  time = vt_pform_wtime();
  vt_exit(ptid, &time);

  return vtDev;
}

/*
 * Check if the active CUDA device with the given CUDA stream is registered.
 * Creates a CUDA device object or VampirTrace CUDA stream if not registered.
 *
 * @param stream the CUDA stream, which should be used after this function call.
 * @param ptid the VampirTrace thread ID of the calling thread
 * @param vtStrm pointer to a pointer of a CUDA stream (caller needs stream id)
 *
 * @return VampirTrace CUDA device object
 */
static VTCUDADevice* VTCUDAcheckThread(cudaStream_t stream, uint32_t ptid,
                                       VTCUDAStrm **vtStrm)
{
    VTCUDADevice *vtDev;
    uint32_t i;
    uint64_t time_check;
    int device;

    time_check = vt_pform_wtime();
    vt_enter(ptid, &time_check, rid_check);

    /* get the device to set the gpu_tid */
    checkCUDACall(cudaGetDevice_ptr(&device), "cudaGetDevice(device) failed!");

    CUDARTWRAP_LOCK();
    vt_cntl_msg(3, "Using CUDA device %d (%d registered)", device, gpuNum);

    /* check if this device+stream has been registered as VampirTrace thread */
    for(i = 0; i < gpuNum; i++){
      vtDev = cudaDevices[i];
      if(vtDev != NULL && vtDev->device == device && ptid == vtDev->ptid){
        /* the cuda device is already listed -> stream 0 exists */
        VTCUDAStrm *curStrm, *ptrLastStrm;
        curStrm = vtDev->strmList;

        CUDARTWRAP_UNLOCK();
        do{
          if(stream == curStrm->stream){
            *vtStrm = curStrm;
            time_check = vt_pform_wtime();
            vt_exit(ptid, &time_check);
            return vtDev;
          }
          ptrLastStrm = curStrm;
          curStrm = curStrm->next;
        }while(curStrm != NULL);
        /* stream not found */

        /* append newly created stream (stream 0 is probably used most, will
           therefore always be the first element in the list */
        ptrLastStrm->next = VTCUDAcreateStream(device, stream, ptid);
        *vtStrm = ptrLastStrm->next;

        time_check = vt_pform_wtime();
        vt_exit(ptid, &time_check);
        return vtDev;
      }
    }
    CUDARTWRAP_UNLOCK();

    /* cuda device not found, create new cuda device node */
   vtDev = VTCUDAinitDevice(ptid, device);

   /* the return values */
   *vtStrm = vtDev->strmList;
   return vtDev;
}

/* 
 * Retrieves the CUDA node for the current CUDA device.
 *
 * @param ptid the VampirTrace thread id of the active thread
 * 
 * @return the VampirTrace CUDA device structure for current CUDA device or NULL if not found
 */
static VTCUDADevice* VTCUDAgetDevice(uint32_t ptid){
  int device;
  uint32_t i;

  /* get the device to set the gpu_tid */
  checkCUDACall(cudaGetDevice_ptr(&device),
                "cudaGetDevice(int *device) failed!");
               
  CUDARTWRAP_LOCK();
                
  vt_cntl_msg(3, "Lookup CUDA device %d of %d", device, gpuNum);

  for (i = 0; i < gpuNum; i++) {
    VTCUDADevice *vtDev = cudaDevices[i];
    if(vtDev != NULL && vtDev->device == device && ptid == vtDev->ptid){
      /* the cuda device is already listed -> stream 0 exists */
      CUDARTWRAP_UNLOCK();
      return vtDev;
    }
  }
  CUDARTWRAP_UNLOCK();

  vt_cntl_msg(2, "[CUDA] Useless or wrong cuda*-function call?");
  return NULL;
}

/* 
 * Add memory copy to entry buffer.
 *
 * @param kind kind/direction of memory copy
 * @param count number of bytes for this data transfer
 * @param stream the cuda stream
 *
 * @return pointer to the VampirTrace CUDA memory copy structure
 */
static VTCUDAMemcpy* addMemcpy2Buf(enum cudaMemcpyKind kind, int count,
                                   cudaStream_t stream)
{
  VTCUDADevice *vtDev;
  VTCUDAStrm *ptrStrm;
  VTCUDAMemcpy *mcpy;
  uint32_t ptid;

  VT_CHECK_THREAD;
  ptid = VT_MY_THREAD;
  vtDev = VTCUDAcheckThread(stream, ptid, &ptrStrm);

  /* check if there is enough buffer space */
  if(vtDev->buf_pos + sizeof(VTCUDAMemcpy) > vtDev->buf_size){
    VTCUDAflush(vtDev, ptid);
  }

  /* get and increase entry buffer position */
  mcpy = (VTCUDAMemcpy*)vtDev->buf_pos;
  vtDev->buf_pos += sizeof(VTCUDAMemcpy);

  /* initialize asynchronous memory copy entry */
  mcpy->type = VTCUDABUF_ENTRY_TYPE__Memcpy;
  mcpy->strm = ptrStrm;
  mcpy->byteCount = count;
  mcpy->pid = ptid;
  mcpy->kind = kind;

  /* get and increase event buffer position */
  mcpy->evt = vtDev->evtbuf_pos;
  vtDev->evtbuf_pos++;

  /* both threads are involved in GPU communication */
  CUDARTWRAP_LOCK();
    gpuCommPIDs[ptid] = GPU_COMM;
    gpuCommPIDs[ptrStrm->tid] |= GPU_COMM;
  CUDARTWRAP_UNLOCK();

  return mcpy;
}

/* The structure of a cuda kernel element. The list will be filled in 
 * __cudaRegisterFunction() and used in cudaLaunch() to get function name from 
 * function pointer.
 */
typedef struct kernelele {
  const char* pointer;                /**< the host function */
  struct kernelele *next;             /**< pointer to next kernel element */
  char name[KERNEL_STR_SIZE];         /**< name of the cuda kernel */
  /*char deviceName[DEVICE_NAME_SIZE];  *< name of the cuda device */
  uint32_t rid;                       /**< region id for this kernel */
}kernelelement;
static kernelelement *kernelListHead = NULL;

/*
 * Parse the device function name:
 * "_Z<kernel_length><kernel_name><templates>..." (no namespace)
 * "_ZN<ns_length><ns_name>...<ns_length><ns_name><kernel_length>..." (with namespace)
 *
 * @param e pointer to the kernel element
 * @param devFunc the CUDA internal kernel function name
 */
static void extractKernelName(kernelelement *e, const char* devFunc)
{
  int i = 0;       /* position in device function (source string) */
  int nlength = 0; /* length of namespace or kernel */
  int ePos = 0;    /* position in final kernel string */
  char *curr_elem, kn_templates[KERNEL_STR_SIZE];
  char *tmpEnd, *tmpElemEnd;

  /*vt_cntl_msg(1,"[CUDA] device funtion name: %s'", devFunc);*/

  /* init for both cases: namespace available or not */
  if(devFunc[2] == 'N'){
    nlength = atoi(&devFunc[3]); /* get length of first namespace */
    i = 4;
  }else{
    nlength = atoi(&devFunc[2]); /* get length of kernel */
    i = 3;
  }

  /* unless string null termination */
  while(devFunc[i] != '\0'){
    /* found either namespace or kernel name (no digits) */
    if(devFunc[i] < '0' || devFunc[i] > '9'){
      /* copy name to kernel function */
      if((ePos + nlength) < KERNEL_STR_SIZE){
        (void)strncpy(&e->name[ePos], &devFunc[i], nlength);
        ePos += nlength; /* set next position to write */
      }else{
        nlength = KERNEL_STR_SIZE - ePos;
        (void)strncpy(&e->name[ePos], &devFunc[i], nlength);
        vt_cntl_msg(1,"[CUDA]: kernel name '%s' contains more than %d chars!",
                      devFunc, KERNEL_STR_SIZE);
        return;
      }

      i += nlength; /* jump over name */
      nlength = atoi(&devFunc[i]); /* get length of next namespace or kernel */

      /* finish if no digit after namespace or kernel */
      if(nlength == 0){
        e->name[ePos] = '\0'; /* set string termination */
        break;
      }else{
        if((ePos + 3) < KERNEL_STR_SIZE){
          (void)strncpy(&e->name[ePos], "::\0", 3);
          ePos += 2;
        }else{
          vt_cntl_msg(1,"[CUDA]: kernel name '%s' contains more than %d chars!",
                        devFunc, KERNEL_STR_SIZE);
          return;
        }
      }
    }else i++;
  }

  /* copy the end of the kernel name string to extract templates */
  if(-1 == snprintf(kn_templates, KERNEL_STR_SIZE, "%s", &devFunc[i+1]))
    vt_cntl_msg(1, "[CUDA]: Error parsing kernel '%s'", devFunc);
  curr_elem = kn_templates; /* should be 'L' */

  /* search templates (e.g. "_Z10cptCurrentILb1ELi10EEv6SField8SParListifff") */
  tmpEnd=strstr(curr_elem,"EE");
  /* check for templates: curr_elem[0] points to 'L' AND string contains "EE" */
  if(tmpEnd != NULL && curr_elem[0]=='L'){ /* templates exist */
    tmpEnd[1] = '\0'; /* set 2nd 'E' to \0 as string end marker */

    /* write at postion 'I' with '<' */
    /* elem->name[ePos]='<'; */
    if(-1 == snprintf(&(e->name[ePos]),KERNEL_STR_SIZE-ePos,"<"))
      vt_cntl_msg(1,"[CUDA] Parsing templates of kernel '%s' failed!", devFunc);
    ePos++; /* continue with next character */

    do{
      int res;
      curr_elem++; /* set pointer to template type length or template type */
      /* find end of template element */
      tmpElemEnd = strchr(curr_elem + atoi(curr_elem), 'E');
      tmpElemEnd[0] = '\0'; /* set termination char after template element */
      /* find next non-digit char */
      while(*curr_elem >= '0' && *curr_elem <= '9') curr_elem++;
      /* append template value to kernel name */
      if(-1 == (res = snprintf(&(e->name[ePos]),
                               KERNEL_STR_SIZE-ePos,"%s,",curr_elem)))
        vt_cntl_msg(1,"[CUDA]: Parsing templates of kernel '%s' crashed!", devFunc);
      ePos += res; /* continue after template value */
      curr_elem =tmpElemEnd + 1; /* set current element to begin of next template */
    }while(tmpElemEnd < tmpEnd);
    if((ePos-1) < KERNEL_STR_SIZE) (void)strncpy(&e->name[ePos-1], ">\0", 2);
    else vt_cntl_msg(1,"[CUDA]: Templates of '%s' too long for internal buffer!", devFunc);
  } /* else: kernel has no templates */
  /*vt_cntl_msg(1,"[CUDA] funtion name: %s'",e->name);*/
}

/*
 * Inserts a new element in the kernel list (LIFO).
 * (Called by __cudaRegisterFunction)
 *
 *  @param hostFun the name of the host function
 *  @param deviceFun the name of kernel (device function)
 */
static void insertKernelElement(const char* hostFun, 
                                const char* deviceFun
                                /*, const char *deviceName*/)
{
  kernelelement* e = (kernelelement*) malloc(sizeof(kernelelement));
  e->pointer = hostFun;
  /*strncpy(e->deviceName, deviceName, DEVICE_NAME_SIZE);*/
  extractKernelName(e,deviceFun);

#if (defined(VT_MT) || defined(VT_HYB))
      VTTHRD_LOCK_IDS();
#endif
   e->rid = vt_def_region(VT_MASTER_THREAD, e->name, VT_NO_ID, VT_NO_LNO, 
                           VT_NO_LNO, "CUDA_KERNEL", VT_FUNCTION);
#if (defined(VT_MT) || defined(VT_HYB))
      VTTHRD_UNLOCK_IDS();
#endif

  /* lock list operation if multi-threaded */
  CUDARTWRAP_LOCK();
    e->next = kernelListHead;
    kernelListHead = e;
  CUDARTWRAP_UNLOCK();
}

/** Get kernel element from kernel pointer (to lookup name and token).
 *  @param listHead pointer to the last kernel element in the list
 *  @param hostFun the identifier string of the cuda kernel
 *  @return the kernelNULL, if nothing was found
 *  @todo linear search should be replaced with hash
 */
static kernelelement* getKernelElement(const char* hostFun)
{
  kernelelement *actual = NULL;

  /* lock list operation if multi-threaded */
  CUDARTWRAP_LOCK();

  actual = kernelListHead;

  while(actual != NULL) {
    if(hostFun == actual->pointer) {
      CUDARTWRAP_UNLOCK();
      return actual;
    }
    actual = actual->next;
  }

  CUDARTWRAP_UNLOCK();

  return NULL; /* not found */
}

/*
 * This function is being called before execution of a cuda program for every
 * cuda kernel (host_runtime.h)
 */
void __cudaRegisterFunction(void **, const char *, char *, const char *, int,
                            uint3 *, uint3 *, dim3 *, dim3 *, int *);
void  __cudaRegisterFunction(void   **fatCubinHandle,
  const char    *hostFun,
        char    *deviceFun,
  const char    *deviceName,
        int      thread_limit,
        uint3   *tid,
        uint3   *bid,
        dim3    *bDim,
        dim3    *gDim,
        int     *wSize )
{

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "__cudaRegisterFunction",
      cudaError_t , (void   **,
            const char    *,
                  char    *,
            const char    *,
                  int      ,
                  uint3   *,
                  uint3   *,
                  dim3    *,
                  dim3    *,
                  int     *),
      NULL, 0);

    VT_LIBWRAP_FUNC_CALL(lw, (fatCubinHandle,hostFun,deviceFun,deviceName,
        thread_limit,tid,bid,bDim,gDim,wSize));

    if(trace_enabled && trace_kernels){
      insertKernelElement(hostFun, deviceFun/*, deviceName*/);
    }
}

/* -- cuda_runtime_api.h:cudaMalloc3D -- */

cudaError_t  cudaMalloc3D(struct cudaPitchedPtr *pitchedDevPtr, struct cudaExtent extent)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMalloc3D",
    cudaError_t , (struct cudaPitchedPtr *, struct cudaExtent ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (pitchedDevPtr, extent));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* if  < CUDA 3.1 */
#if (defined(CUDART_VERSION) && (CUDART_VERSION < 3010))

/* -- cuda_runtime_api.h:cudaMalloc3DArray -- */

cudaError_t  cudaMalloc3DArray(struct cudaArray **arrayPtr, const struct cudaChannelFormatDesc *desc, struct cudaExtent extent)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMalloc3DArray",
    cudaError_t , (struct cudaArray **, const struct cudaChannelFormatDesc *, struct cudaExtent ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (arrayPtr, desc, extent));

  if(trace_enabled){
    VT_LIBWRAP_FUNC_END(lw);
  }

  return ret;
}

/* -- cuda_runtime_api.h:cudaMallocArray -- */

cudaError_t  cudaMallocArray(struct cudaArray **array, const struct cudaChannelFormatDesc *desc, size_t width, size_t height)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMallocArray",
    cudaError_t , (struct cudaArray **, const struct cudaChannelFormatDesc *, size_t , size_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (array, desc, width, height));

  if(trace_enabled){
    VT_LIBWRAP_FUNC_END(lw);
  }

  return ret;
}

#endif

/* -- cuda_runtime_api.h:cudaMemset3D -- */

cudaError_t  cudaMemset3D(struct cudaPitchedPtr pitchedDevPtr, int value, struct cudaExtent extent)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemset3D",
    cudaError_t , (struct cudaPitchedPtr , int , struct cudaExtent ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (pitchedDevPtr, value, extent));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpy3D -- */

cudaError_t  cudaMemcpy3D(const struct cudaMemcpy3DParms *p)
{
  cudaError_t  ret;

  enum cudaMemcpyKind kind = p->kind;
  struct cudaExtent extent = p->extent;
  int count = extent.height*extent.width*extent.depth;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpy3D",
    cudaError_t , (const struct cudaMemcpy3DParms *),
    NULL, 0);

  CUDA_SEND_RECV(kind, count,
      ret = VT_LIBWRAP_FUNC_CALL(lw, (p));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpy3DAsync -- */

cudaError_t  cudaMemcpy3DAsync(const struct cudaMemcpy3DParms *p, cudaStream_t stream)
{
  cudaError_t  ret;
  enum cudaMemcpyKind kind = p->kind;
  struct cudaExtent extent = p->extent;
  int count = extent.height*extent.width*extent.depth;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpy3DAsync",
    cudaError_t , (const struct cudaMemcpy3DParms *, cudaStream_t ),
    NULL, 0);

  CUDA_MEMCPY_ASYNC(kind, count, stream,
    ret = VT_LIBWRAP_FUNC_CALL(lw, (p, stream));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMalloc -- */

cudaError_t  cudaMalloc(void **devPtr, size_t size)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMalloc",
    cudaError_t , (void **, size_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (devPtr, size));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaMallocHost -- */

cudaError_t  cudaMallocHost(void **ptr, size_t size)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMallocHost",
    cudaError_t , (void **, size_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (ptr, size));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaMallocPitch -- */

cudaError_t  cudaMallocPitch(void **devPtr, size_t *pitch, size_t width, size_t height)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMallocPitch",
    cudaError_t , (void **, size_t *, size_t , size_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (devPtr, pitch, width, height));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaFree -- */

cudaError_t  cudaFree(void *devPtr)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaFree",
    cudaError_t , (void *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (devPtr));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaFreeHost -- */

cudaError_t  cudaFreeHost(void *ptr)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaFreeHost",
    cudaError_t , (void *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (ptr));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaFreeArray -- */

cudaError_t  cudaFreeArray(struct cudaArray *array)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaFreeArray",
    cudaError_t , (struct cudaArray *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (array));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaHostAlloc -- */

cudaError_t  cudaHostAlloc(void **pHost, size_t bytes, unsigned int flags)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaHostAlloc",
    cudaError_t , (void **, size_t , unsigned int ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (pHost, bytes, flags));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaHostGetDevicePointer -- */

cudaError_t  cudaHostGetDevicePointer(void **pDevice, void *pHost, unsigned int flags)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaHostGetDevicePointer",
    cudaError_t , (void **, void *, unsigned int ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (pDevice, pHost, flags));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpy -- */

cudaError_t  cudaMemcpy(void *dst, const void *src, size_t count, enum cudaMemcpyKind kind)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpy",
    cudaError_t , (void *, const void *, size_t , enum cudaMemcpyKind ),
    NULL, 0);

  CUDA_SEND_RECV(kind, count,
      ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, src, count, kind));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpyToArray -- */

cudaError_t  cudaMemcpyToArray(struct cudaArray *dst, size_t wOffset, size_t hOffset, const void *src, size_t count, enum cudaMemcpyKind kind)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpyToArray",
    cudaError_t , (struct cudaArray *, size_t , size_t , const void *, size_t , enum cudaMemcpyKind ),
    NULL, 0);

  CUDA_SEND_RECV(kind, count,
      ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, wOffset, hOffset, src, count, kind));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpyFromArray -- */

cudaError_t  cudaMemcpyFromArray(void *dst, const struct cudaArray *src, size_t wOffset, size_t hOffset, size_t count, enum cudaMemcpyKind kind)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpyFromArray",
    cudaError_t , (void *, const struct cudaArray *, size_t , size_t , size_t , enum cudaMemcpyKind ),
    NULL, 0);

  CUDA_SEND_RECV(kind, count,
      ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, src, wOffset, hOffset, count, kind));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpyArrayToArray -- */

cudaError_t  cudaMemcpyArrayToArray(struct cudaArray *dst, size_t wOffsetDst, size_t hOffsetDst, const struct cudaArray *src, size_t wOffsetSrc, size_t hOffsetSrc, size_t count, enum cudaMemcpyKind kind)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpyArrayToArray",
    cudaError_t , (struct cudaArray *, size_t , size_t , const struct cudaArray *, size_t , size_t , size_t , enum cudaMemcpyKind ),
    NULL, 0);

  CUDA_SEND_RECV(kind, count,
      ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, wOffsetDst, hOffsetDst, src, wOffsetSrc, hOffsetSrc, count, kind));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpy2D -- */

cudaError_t  cudaMemcpy2D(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height, enum cudaMemcpyKind kind)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpy2D",
    cudaError_t , (void *, size_t , const void *, size_t , size_t , size_t , enum cudaMemcpyKind ),
    NULL, 0);

  CUDA_SEND_RECV(kind, width*height,
      ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, dpitch, src, spitch, width, height, kind));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpy2DToArray -- */

cudaError_t  cudaMemcpy2DToArray(struct cudaArray *dst, size_t wOffset, size_t hOffset, const void *src, size_t spitch, size_t width, size_t height, enum cudaMemcpyKind kind)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpy2DToArray",
    cudaError_t , (struct cudaArray *, size_t , size_t , const void *, size_t , size_t , size_t , enum cudaMemcpyKind ),
    NULL, 0);

  CUDA_SEND_RECV(kind, width*height,
      ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, wOffset, hOffset, src, spitch, width, height, kind));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpy2DFromArray -- */

cudaError_t  cudaMemcpy2DFromArray(void *dst, size_t dpitch, const struct cudaArray *src, size_t wOffset, size_t hOffset, size_t width, size_t height, enum cudaMemcpyKind kind)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpy2DFromArray",
    cudaError_t , (void *, size_t , const struct cudaArray *, size_t , size_t , size_t , size_t , enum cudaMemcpyKind ),
    NULL, 0);

  CUDA_SEND_RECV(kind, width*height,
      ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, dpitch, src, wOffset, hOffset, width, height, kind));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpy2DArrayToArray -- */

cudaError_t  cudaMemcpy2DArrayToArray(struct cudaArray *dst, size_t wOffsetDst, size_t hOffsetDst, const struct cudaArray *src, size_t wOffsetSrc, size_t hOffsetSrc, size_t width, size_t height, enum cudaMemcpyKind kind)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpy2DArrayToArray",
    cudaError_t , (struct cudaArray *, size_t , size_t , const struct cudaArray *, size_t , size_t , size_t , size_t , enum cudaMemcpyKind ),
    NULL, 0);

  CUDA_SEND_RECV(kind, width*height,
      ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, wOffsetDst, hOffsetDst, src, wOffsetSrc, hOffsetSrc, width, height, kind));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpyToSymbol -- */

cudaError_t  cudaMemcpyToSymbol(const char *symbol, const void *src, size_t count, size_t offset, enum cudaMemcpyKind kind)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpyToSymbol",
    cudaError_t , (const char *, const void *, size_t , size_t , enum cudaMemcpyKind ),
    NULL, 0);

  CUDA_SEND_RECV(kind, count,
      ret = VT_LIBWRAP_FUNC_CALL(lw, (symbol, src, count, offset, kind));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpyFromSymbol -- */

cudaError_t  cudaMemcpyFromSymbol(void *dst, const char *symbol, size_t count, size_t offset, enum cudaMemcpyKind kind)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpyFromSymbol",
    cudaError_t , (void *, const char *, size_t , size_t , enum cudaMemcpyKind ),
    NULL, 0);

  CUDA_SEND_RECV(kind, count,
      ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, symbol, count, offset, kind));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpyAsync -- */

cudaError_t  cudaMemcpyAsync(void *dst, const void *src, size_t count, enum cudaMemcpyKind kind, cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpyAsync",
    cudaError_t , (void *, const void *, size_t , enum cudaMemcpyKind , cudaStream_t ),
    NULL, 0);

  CUDA_MEMCPY_ASYNC(kind, count, stream,
    ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, src, count, kind, stream));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpyToArrayAsync -- */

cudaError_t  cudaMemcpyToArrayAsync(struct cudaArray *dst, size_t wOffset, size_t hOffset, const void *src, size_t count, enum cudaMemcpyKind kind, cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpyToArrayAsync",
    cudaError_t , (struct cudaArray *, size_t , size_t , const void *, size_t , enum cudaMemcpyKind , cudaStream_t ),
    NULL, 0);

  CUDA_MEMCPY_ASYNC(kind, count, stream,
    ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, wOffset, hOffset, src, count, kind, stream));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpyFromArrayAsync -- */

cudaError_t  cudaMemcpyFromArrayAsync(void *dst, const struct cudaArray *src, size_t wOffset, size_t hOffset, size_t count, enum cudaMemcpyKind kind, cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpyFromArrayAsync",
    cudaError_t , (void *, const struct cudaArray *, size_t , size_t , size_t , enum cudaMemcpyKind , cudaStream_t ),
    NULL, 0);

  CUDA_MEMCPY_ASYNC(kind, count, stream,
    ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, src, wOffset, hOffset, count, kind, stream));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpy2DAsync -- */

cudaError_t  cudaMemcpy2DAsync(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height, enum cudaMemcpyKind kind, cudaStream_t stream)
{
  cudaError_t  ret;


  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpy2DAsync",
    cudaError_t , (void *, size_t , const void *, size_t , size_t , size_t , enum cudaMemcpyKind , cudaStream_t ),
    NULL, 0);

  CUDA_MEMCPY_ASYNC(kind, width*height, stream,
    ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, dpitch, src, spitch, width, height, kind, stream));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpy2DToArrayAsync -- */

cudaError_t  cudaMemcpy2DToArrayAsync(struct cudaArray *dst, size_t wOffset, size_t hOffset, const void *src, size_t spitch, size_t width, size_t height, enum cudaMemcpyKind kind, cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpy2DToArrayAsync",
    cudaError_t , (struct cudaArray *, size_t , size_t , const void *, size_t , size_t , size_t , enum cudaMemcpyKind , cudaStream_t ),
    NULL, 0);

  CUDA_MEMCPY_ASYNC(kind, width*height, stream,
    ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, wOffset, hOffset, src, spitch, width, height, kind, stream));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpy2DFromArrayAsync -- */

cudaError_t  cudaMemcpy2DFromArrayAsync(void *dst, size_t dpitch, const struct cudaArray *src, size_t wOffset, size_t hOffset, size_t width, size_t height, enum cudaMemcpyKind kind, cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpy2DFromArrayAsync",
    cudaError_t , (void *, size_t , const struct cudaArray *, size_t , size_t , size_t , size_t , enum cudaMemcpyKind , cudaStream_t ),
    NULL, 0);

  CUDA_MEMCPY_ASYNC(kind, width*height, stream,
    ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, dpitch, src, wOffset, hOffset, width, height, kind, stream));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpyToSymbolAsync -- */

cudaError_t  cudaMemcpyToSymbolAsync(const char *symbol, const void *src, size_t count, size_t offset, enum cudaMemcpyKind kind, cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpyToSymbolAsync",
    cudaError_t , (const char *, const void *, size_t , size_t , enum cudaMemcpyKind , cudaStream_t ),
    NULL, 0);

  CUDA_MEMCPY_ASYNC(kind, count, stream,
    ret = VT_LIBWRAP_FUNC_CALL(lw, (symbol, src, count, offset, kind, stream));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemcpyFromSymbolAsync -- */

cudaError_t  cudaMemcpyFromSymbolAsync(void *dst, const char *symbol, size_t count, size_t offset, enum cudaMemcpyKind kind, cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemcpyFromSymbolAsync",
    cudaError_t , (void *, const char *, size_t , size_t , enum cudaMemcpyKind , cudaStream_t ),
    NULL, 0);

  CUDA_MEMCPY_ASYNC(kind, count, stream,
    ret = VT_LIBWRAP_FUNC_CALL(lw, (dst, symbol, count, offset, kind, stream));
  );

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemset -- */

cudaError_t  cudaMemset(void *devPtr, int value, size_t count)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemset",
    cudaError_t , (void *, int , size_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (devPtr, value, count));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaMemset2D -- */

cudaError_t  cudaMemset2D(void *devPtr, size_t pitch, int value, size_t width, size_t height)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemset2D",
    cudaError_t , (void *, size_t , int , size_t , size_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (devPtr, pitch, value, width, height));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGetSymbolAddress -- */

cudaError_t  cudaGetSymbolAddress(void **devPtr, const char *symbol)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGetSymbolAddress",
    cudaError_t , (void **, const char *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (devPtr, symbol));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGetSymbolSize -- */

cudaError_t  cudaGetSymbolSize(size_t *size, const char *symbol)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGetSymbolSize",
    cudaError_t , (size_t *, const char *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (size, symbol));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGetDeviceCount -- */

cudaError_t  cudaGetDeviceCount(int *count)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGetDeviceCount",
    cudaError_t , (int *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (count));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGetDeviceProperties -- */

cudaError_t  cudaGetDeviceProperties(struct cudaDeviceProp *prop, int device)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGetDeviceProperties",
    cudaError_t , (struct cudaDeviceProp *, int ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (prop, device));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaChooseDevice -- */

cudaError_t  cudaChooseDevice(int *device, const struct cudaDeviceProp *prop)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaChooseDevice",
    cudaError_t , (int *, const struct cudaDeviceProp *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (device, prop));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaSetDevice -- */

cudaError_t  cudaSetDevice(int device)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaSetDevice",
    cudaError_t , (int ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (device));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGetDevice -- */

cudaError_t  cudaGetDevice(int *device)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGetDevice",
    cudaError_t , (int *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (device));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaSetValidDevices -- */

cudaError_t  cudaSetValidDevices(int *device_arr, int len)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaSetValidDevices",
    cudaError_t , (int *, int ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (device_arr, len));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaBindTexture -- */

cudaError_t  cudaBindTexture(size_t *offset, const struct textureReference *texref, const void *devPtr, const struct cudaChannelFormatDesc *desc, size_t size)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaBindTexture",
    cudaError_t , (size_t *, const struct textureReference *, const void *, const struct cudaChannelFormatDesc *, size_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (offset, texref, devPtr, desc, size));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaBindTexture2D -- */

cudaError_t  cudaBindTexture2D(size_t *offset, const struct textureReference *texref, const void *devPtr, const struct cudaChannelFormatDesc *desc, size_t width, size_t height, size_t pitch)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaBindTexture2D",
    cudaError_t , (size_t *, const struct textureReference *, const void *, const struct cudaChannelFormatDesc *, size_t , size_t , size_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (offset, texref, devPtr, desc, width, height, pitch));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaBindTextureToArray -- */

cudaError_t  cudaBindTextureToArray(const struct textureReference *texref, const struct cudaArray *array, const struct cudaChannelFormatDesc *desc)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaBindTextureToArray",
    cudaError_t , (const struct textureReference *, const struct cudaArray *, const struct cudaChannelFormatDesc *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (texref, array, desc));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaUnbindTexture -- */

cudaError_t  cudaUnbindTexture(const struct textureReference *texref)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaUnbindTexture",
    cudaError_t , (const struct textureReference *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (texref));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGetTextureAlignmentOffset -- */

cudaError_t  cudaGetTextureAlignmentOffset(size_t *offset, const struct textureReference *texref)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGetTextureAlignmentOffset",
    cudaError_t , (size_t *, const struct textureReference *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (offset, texref));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGetTextureReference -- */

cudaError_t  cudaGetTextureReference(const struct textureReference **texref, const char *symbol)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGetTextureReference",
    cudaError_t , (const struct textureReference **, const char *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (texref, symbol));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGetChannelDesc -- */

cudaError_t  cudaGetChannelDesc(struct cudaChannelFormatDesc *desc, const struct cudaArray *array)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGetChannelDesc",
    cudaError_t , (struct cudaChannelFormatDesc *, const struct cudaArray *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (desc, array));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaCreateChannelDesc -- */

struct cudaChannelFormatDesc  cudaCreateChannelDesc(int x, int y, int z, int w, enum cudaChannelFormatKind f)
{
  struct cudaChannelFormatDesc  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaCreateChannelDesc",
    struct cudaChannelFormatDesc , (int , int , int , int , enum cudaChannelFormatKind ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (x, y, z, w, f));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGetLastError -- */

cudaError_t  cudaGetLastError()
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGetLastError",
    cudaError_t , (void),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, ());

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGetErrorString -- */

const char * cudaGetErrorString(cudaError_t error)
{
  const char * ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGetErrorString",
    const char *, (cudaError_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (error));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaConfigureCall -- */
cudaError_t  cudaConfigureCall(dim3 gridDim, dim3 blockDim, size_t sharedMem, cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaConfigureCall",
    cudaError_t , (dim3 , dim3 , size_t , cudaStream_t),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (gridDim, blockDim, sharedMem, stream));

  if(trace_enabled){
    VT_LIBWRAP_FUNC_END(lw);  /* no extra if(trace_enabled) */

    if(trace_kernels){
      VTCUDADevice* vtDev;
      VTCUDAStrm *ptrStrm;
      uint32_t ptid;

      VT_CHECK_THREAD;
      ptid = VT_MY_THREAD;

      if(vt_is_trace_on(ptid)){
        vtDev = VTCUDAcheckThread(stream, ptid, &ptrStrm);

        /* check if there is enough buffer space */
        if(vtDev->buf_pos + sizeof(VTCUDAKernel) > vtDev->buf_size){
          VTCUDAflush(vtDev, ptid);
        }

        ((VTCUDAKernel*)vtDev->buf_pos)->strm = ptrStrm;
      }
    }
  }

  return ret;
}

/* -- cuda_runtime_api.h:cudaSetupArgument -- */
cudaError_t  cudaSetupArgument(const void *arg, size_t size, size_t offset)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaSetupArgument",
    cudaError_t , (const void *, size_t , size_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (arg, size, offset));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaLaunch -- */
cudaError_t  cudaLaunch(const char *entry)
{
  cudaError_t  ret;
  /*VTCUDADevice *node = NULL;*/
  VTCUDAKernel *kernel = NULL;
  kernelelement* e = NULL;
  uint8_t do_trace = 0;
  uint32_t ptid = 0;
  uint64_t time;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaLaunch",
    cudaError_t , (const char *), NULL, 0);

  if(trace_enabled){
    VT_CHECK_THREAD;
    ptid = VT_MY_THREAD;

    do_trace = vt_is_trace_on(ptid);

    if(trace_kernels && do_trace){
      /* get kernel element */
      e = getKernelElement(entry);
      if(e != NULL){

        /* check if the kernel will be traced on the correct thread */
        VTCUDADevice *vtDev = VTCUDAgetDevice(ptid);
        kernel = (VTCUDAKernel*)vtDev->buf_pos;

        vt_cntl_msg(3, "[CUDA] Launch %s (%d) on device %d:%d (stream %d)",
                      e->name, e->rid, vtDev->device, vtDev->ptid,
                      kernel->strm->stream);

        kernel->rid = e->rid;

        /* set type of buffer entry */
        kernel->type = VTCUDABUF_ENTRY_TYPE__Kernel;

        /*  get an already created unused event */
        kernel->evt = vtDev->evtbuf_pos;

        /* increment buffers */
        vtDev->evtbuf_pos++;
        vtDev->buf_pos += sizeof(VTCUDAKernel);

        checkCUDACall(cudaEventRecord_ptr(kernel->evt->strt, kernel->strm->stream),
                      "cudaEventRecord(startEvt, strmOfLastKernel) failed!");
      }
    }
    time = vt_pform_wtime();
    (void)vt_enter(ptid, &time, VT_LIBWRAP_FUNC_ID);
    /*VT_LIBWRAP_FUNC_START(lw); */
  }

  /* call cudaLaunch itself */
  ret = VT_LIBWRAP_FUNC_CALL(lw, (entry));

  if(trace_enabled){
    time = vt_pform_wtime();
    vt_exit(ptid, &time);
    /*VT_LIBWRAP_FUNC_END(lw); */

    if(e != NULL && trace_kernels && do_trace){
      checkCUDACall(cudaEventRecord_ptr(kernel->evt->stop, kernel->strm->stream),
                    "cudaEventRecord(stopEvt, streamOfCurrentKernel) failed!");
    }
  }

  return ret;
}

/* -- cuda_runtime_api.h:cudaFuncGetAttributes -- */

cudaError_t  cudaFuncGetAttributes(struct cudaFuncAttributes *attr, const char *func)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaFuncGetAttributes",
    cudaError_t , (struct cudaFuncAttributes *, const char *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (attr, func));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaStreamCreate -- */

cudaError_t  cudaStreamCreate(cudaStream_t *pStream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaStreamCreate",
    cudaError_t , (cudaStream_t *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (pStream));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaStreamDestroy -- */

cudaError_t  cudaStreamDestroy(cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaStreamDestroy",
    cudaError_t , (cudaStream_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (stream));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaStreamSynchronize -- */

cudaError_t  cudaStreamSynchronize(cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaStreamSynchronize",
    cudaError_t , (cudaStream_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (stream));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaStreamQuery -- */

cudaError_t  cudaStreamQuery(cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaStreamQuery",
    cudaError_t , (cudaStream_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (stream));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaEventCreate -- */

cudaError_t  cudaEventCreate(cudaEvent_t *event)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaEventCreate",
    cudaError_t , (cudaEvent_t *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (event));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaEventRecord -- */

cudaError_t  cudaEventRecord(cudaEvent_t event, cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaEventRecord",
    cudaError_t , (cudaEvent_t , cudaStream_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (event, stream));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaEventQuery -- */

cudaError_t  cudaEventQuery(cudaEvent_t event)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaEventQuery",
    cudaError_t , (cudaEvent_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (event));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaEventSynchronize -- */

cudaError_t  cudaEventSynchronize(cudaEvent_t event)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaEventSynchronize",
    cudaError_t , (cudaEvent_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (event));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaEventDestroy -- */

cudaError_t  cudaEventDestroy(cudaEvent_t event)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaEventDestroy",
    cudaError_t , (cudaEvent_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (event));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaEventElapsedTime -- */

cudaError_t  cudaEventElapsedTime(float *ms, cudaEvent_t start, cudaEvent_t end)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaEventElapsedTime",
    cudaError_t , (float *, cudaEvent_t , cudaEvent_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (ms, start, end));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaSetDoubleForDevice -- */

cudaError_t  cudaSetDoubleForDevice(double *d)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaSetDoubleForDevice",
    cudaError_t , (double *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (d));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaSetDoubleForHost -- */

cudaError_t  cudaSetDoubleForHost(double *d)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaSetDoubleForHost",
    cudaError_t , (double *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (d));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaThreadExit -- */

cudaError_t  cudaThreadExit()
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaThreadExit",
    cudaError_t , (void), NULL, 0);

  if(trace_enabled){
    uint32_t ptid;
    VTCUDADevice *vtDev;

    VT_CHECK_THREAD;
    ptid = VT_MY_THREAD;

    vtDev = VTCUDAgetDevice(ptid);

    /*vt_cntl_msg(2, "cudaThreadExit called at %llu", vt_pform_wtime());*/
      /* cleanup the CUDA device associated to this thread */
    CUDARTWRAP_LOCK();
      VTCUDAcleanupDevice(ptid, vtDev, 1);
    CUDARTWRAP_UNLOCK();
    VT_LIBWRAP_FUNC_START(lw); /* no extra if(trace_enabled) */
  }

  ret = VT_LIBWRAP_FUNC_CALL(lw, ());

  if(trace_enabled) VT_LIBWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaThreadSynchronize -- */

cudaError_t  cudaThreadSynchronize()
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaThreadSynchronize",
    cudaError_t , (void),
    NULL, 0);

  if(trace_enabled){
    if(syncLevel > 2){
      VTCUDADevice *node = NULL;
      uint32_t ptid;

      VT_CHECK_THREAD;
      ptid = VT_MY_THREAD;
      node = VTCUDAgetDevice(ptid);
      VTCUDAflush(node, ptid);
    }
    VT_LIBWRAP_FUNC_START(lw); /* no extra if(trace_enabled) */
  }

  ret = VT_LIBWRAP_FUNC_CALL(lw, ());

  if(trace_enabled) VT_LIBWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaDriverGetVersion -- */

cudaError_t  cudaDriverGetVersion(int *driverVersion)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaDriverGetVersion",
    cudaError_t , (int *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (driverVersion));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaRuntimeGetVersion -- */
cudaError_t  cudaRuntimeGetVersion(int *runtimeVersion)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaRuntimeGetVersion",
    cudaError_t , (int *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (runtimeVersion));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* CUDA 2.3 */
#if (defined(CUDART_VERSION) && (CUDART_VERSION >= 2030))

/* -- cuda_runtime_api.h:cudaHostGetFlags(unsigned int *pFlags, void *pHost) -- */
cudaError_t  cudaHostGetFlags(unsigned int *pFlags, void *pHost)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaHostGetFlags",
      cudaError_t, (unsigned int *, void *),
      NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (pFlags, pHost));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

#endif

/* CUDA 3.0 */
#if (defined(CUDART_VERSION) && (CUDART_VERSION >= 3000))

/* -- cuda_runtime_api.h:cudaMemGetInfo -- */
cudaError_t  cudaMemGetInfo(size_t *free, size_t *total)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemGetInfo",
    cudaError_t , (size_t *, size_t *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (free, total));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaFuncSetCacheConfig -- */
cudaError_t  cudaFuncSetCacheConfig(const char *func, enum cudaFuncCache cacheConfig)
{
  cudaError_t  ret;

  VT_LIBWRAP_FUNC_INIT(lw, lw_attr, "cudaFuncSetCacheConfig",
    cudaError_t , (const char *, enum cudaFuncCache ),
    NULL, 0);

  VT_LIBWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (func, cacheConfig));

  VT_LIBWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGraphicsUnregisterResource -- */
cudaError_t  cudaGraphicsUnregisterResource(struct cudaGraphicsResource *resource)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGraphicsUnregisterResource",
    cudaError_t , (struct cudaGraphicsResource *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (resource));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGraphicsResourceSetMapFlags -- */
cudaError_t  cudaGraphicsResourceSetMapFlags(struct cudaGraphicsResource *resource, unsigned int flags)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGraphicsResourceSetMapFlags",
    cudaError_t , (struct cudaGraphicsResource *, unsigned int ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (resource, flags));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGraphicsMapResources -- */
cudaError_t  cudaGraphicsMapResources(int count, struct cudaGraphicsResource **resources, cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGraphicsMapResources",
    cudaError_t , (int , struct cudaGraphicsResource **, cudaStream_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (count, resources, stream));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGraphicsUnmapResources -- */
cudaError_t  cudaGraphicsUnmapResources(int count, struct cudaGraphicsResource **resources, cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGraphicsUnmapResources",
    cudaError_t , (int , struct cudaGraphicsResource **, cudaStream_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (count, resources, stream));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGraphicsResourceGetMappedPointer -- */
cudaError_t  cudaGraphicsResourceGetMappedPointer(void **devPtr, size_t *size, struct cudaGraphicsResource *resource)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGraphicsResourceGetMappedPointer",
    cudaError_t , (void **, size_t *, struct cudaGraphicsResource *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (devPtr, size, resource));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGraphicsSubResourceGetMappedArray -- */
cudaError_t  cudaGraphicsSubResourceGetMappedArray(struct cudaArray **arrayPtr, struct cudaGraphicsResource *resource, unsigned int arrayIndex, unsigned int mipLevel)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaGraphicsSubResourceGetMappedArray",
    cudaError_t , (struct cudaArray **, struct cudaGraphicsResource *, unsigned int , unsigned int ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (arrayPtr, resource, arrayIndex, mipLevel));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

#endif

/* CUDA 3.1 */
#if (defined(CUDART_VERSION) && (CUDART_VERSION >= 3010))

/* -- cuda_runtime_api.h:cudaMallocArray -- */
cudaError_t  cudaMallocArray(struct cudaArray **array, const struct cudaChannelFormatDesc *desc, size_t width, size_t height, unsigned int flags)
{
  cudaError_t  ret;

  VT_LIBWRAP_FUNC_INIT(lw, lw_attr, "cudaMallocArray",
    cudaError_t , (struct cudaArray **, const struct cudaChannelFormatDesc *, size_t , size_t , unsigned int ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (array, desc, width, height, flags));

  if(trace_enabled){
    VT_LIBWRAP_FUNC_END(lw);
  }

  return ret;
}

/* -- cuda_runtime_api.h:cudaMalloc3DArray -- */
cudaError_t  cudaMalloc3DArray(struct cudaArray **arrayPtr, const struct cudaChannelFormatDesc *desc, struct cudaExtent extent, unsigned int flags)
{
  cudaError_t  ret;

  VT_LIBWRAP_FUNC_INIT(lw, lw_attr, "cudaMalloc3DArray",
    cudaError_t , (struct cudaArray **, const struct cudaChannelFormatDesc *, struct cudaExtent , unsigned int ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (arrayPtr, desc, extent, flags));

  if(trace_enabled){
    VT_LIBWRAP_FUNC_END(lw);
  }

  return ret;
}

/* -- cuda_runtime_api.h:cudaBindSurfaceToArray -- */
cudaError_t  cudaBindSurfaceToArray(const struct surfaceReference *surfref, const struct cudaArray *array, const struct cudaChannelFormatDesc *desc)
{
  cudaError_t  ret;

  VT_LIBWRAP_FUNC_INIT(lw, lw_attr, "cudaBindSurfaceToArray",
    cudaError_t , (const struct surfaceReference *, const struct cudaArray *, const struct cudaChannelFormatDesc *),
    NULL, 0);

  VT_LIBWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (surfref, array, desc));

  VT_LIBWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGetSurfaceAlignmentOffset -- */
cudaError_t  cudaGetSurfaceAlignmentOffset(size_t *offset, const struct surfaceReference *surfref)
{
  cudaError_t  ret;

  VT_LIBWRAP_FUNC_INIT(lw, lw_attr, "cudaGetSurfaceAlignmentOffset",
    cudaError_t , (size_t *, const struct surfaceReference *),
    NULL, 0);

  VT_LIBWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (offset, surfref));

  VT_LIBWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGetSurfaceReference -- */
cudaError_t  cudaGetSurfaceReference(const struct surfaceReference **surfref, const char *symbol)
{
  cudaError_t  ret;

  VT_LIBWRAP_FUNC_INIT(lw, lw_attr, "cudaGetSurfaceReference",
    cudaError_t , (const struct surfaceReference **, const char *),
    NULL, 0);

  VT_LIBWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (surfref, symbol));

  VT_LIBWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaPeekAtLastError -- */
cudaError_t  cudaPeekAtLastError()
{
  cudaError_t  ret;

  VT_LIBWRAP_FUNC_INIT(lw, lw_attr, "cudaPeekAtLastError",
    cudaError_t , (void),
    NULL, 0);

  VT_LIBWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, ());

  VT_LIBWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaThreadSetLimit -- */
cudaError_t  cudaThreadSetLimit(enum cudaLimit limit, size_t value)
{
  cudaError_t  ret;

  VT_LIBWRAP_FUNC_INIT(lw, lw_attr, "cudaThreadSetLimit",
    cudaError_t , (enum cudaLimit , size_t ),
    NULL, 0);

  VT_LIBWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (limit, value));

  VT_LIBWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaThreadGetLimit -- */
cudaError_t  cudaThreadGetLimit(size_t *pValue, enum cudaLimit limit)
{
  cudaError_t  ret;

  VT_LIBWRAP_FUNC_INIT(lw, lw_attr, "cudaThreadGetLimit",
    cudaError_t , (size_t *, enum cudaLimit ),
    NULL, 0);

  VT_LIBWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (pValue, limit));

  VT_LIBWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaGetExportTable -- */
cudaError_t cudaGetExportTable(const void **ppExportTable, const cudaUUID_t *pExportTableId)
{
  cudaError_t  ret;

  VT_LIBWRAP_FUNC_INIT(lw, lw_attr, "cudaGetExportTable",
    cudaError_t , (const void **, const cudaUUID_t *),
    NULL, 0);

  VT_LIBWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (ppExportTable, pExportTableId));

  VT_LIBWRAP_FUNC_END(lw);

  return ret;
}

#endif

/*
 *  Adaptions for CUDA 3.2
 */
#if (defined(CUDART_VERSION) && (CUDART_VERSION < 3020))

/* -- cuda_runtime_api.h:cudaSetDeviceFlags -- */
cudaError_t  cudaSetDeviceFlags(int flags)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaSetDeviceFlags",
    cudaError_t , (int ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (flags));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaEventCreateWithFlags -- */
cudaError_t  cudaEventCreateWithFlags(cudaEvent_t *event, int flags)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaEventCreateWithFlags",
    cudaError_t , (cudaEvent_t *, int ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (event, flags));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

#else

/* -- cuda_runtime_api.h:cudaSetDeviceFlags -- */
cudaError_t  cudaSetDeviceFlags(unsigned int flags)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaSetDeviceFlags",
    cudaError_t , (int ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (flags));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaEventCreateWithFlags -- */
cudaError_t  cudaEventCreateWithFlags(cudaEvent_t *event, unsigned int flags)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaEventCreateWithFlags",
    cudaError_t , (cudaEvent_t *, int ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (event, flags));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cudaMemsetAsync -- */
cudaError_t  cudaMemsetAsync(void *devPtr, int value, size_t count, cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemsetAsync",
    cudaError_t , (void *, int , size_t , cudaStream_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (devPtr, value, count, stream));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cudaMemset2DAsync -- */
cudaError_t  cudaMemset2DAsync(void *devPtr, size_t pitch, int value, size_t width, size_t height, cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemset2DAsync",
    cudaError_t , (void *, size_t , int , size_t , size_t , cudaStream_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (devPtr, pitch, value, width, height, stream));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cudaMemset3DAsync -- */
cudaError_t  cudaMemset3DAsync(struct cudaPitchedPtr pitchedDevPtr, int value, struct cudaExtent extent, cudaStream_t stream)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaMemset3DAsync",
    cudaError_t , (struct cudaPitchedPtr , int , struct cudaExtent , cudaStream_t ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (pitchedDevPtr, value, extent, stream));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cuda_runtime_api.h:cudaStreamWaitEvent -- */
cudaError_t  cudaStreamWaitEvent(cudaStream_t stream, cudaEvent_t event, unsigned int flags)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaStreamWaitEvent",
    cudaError_t , (cudaStream_t , cudaEvent_t , unsigned int ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (stream, event, flags));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cudaThreadGetCacheConfig -- */
cudaError_t  cudaThreadGetCacheConfig(enum cudaFuncCache *pCacheConfig)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaThreadGetCacheConfig",
    cudaError_t , (enum cudaFuncCache *),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (pCacheConfig));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

/* -- cudaThreadSetCacheConfig -- */
cudaError_t  cudaThreadSetCacheConfig(enum cudaFuncCache cacheConfig)
{
  cudaError_t  ret;

  CUDARTWRAP_FUNC_INIT(lw, lw_attr, "cudaThreadSetCacheConfig",
    cudaError_t , (enum cudaFuncCache ),
    NULL, 0);

  CUDARTWRAP_FUNC_START(lw);

  ret = VT_LIBWRAP_FUNC_CALL(lw, (cacheConfig));

  CUDARTWRAP_FUNC_END(lw);

  return ret;
}

#endif /* CUDA 3.2 */