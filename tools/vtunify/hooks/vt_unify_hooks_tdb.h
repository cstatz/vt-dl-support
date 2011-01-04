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

#ifndef _VT_UNIFY_HOOKS_TDB_H_
#define _VT_UNIFY_HOOKS_TDB_H_

#include "vt_unify_hooks_base.h"
#include "vt_unify.h"

#include <errno.h>

#ifdef VT_MPI
#   include "vt_unify_mpi.h"
#endif /* VT_MPI */

#include "vt_inttypes.h"

#include "otf.h"

class collop_class {

  public:
    uint64_t num;
    uint64_t bytes_sent;
    uint64_t bytes_recv;

  public:

    collop_class();
    collop_class( uint64_t _num, uint64_t _bytes_sent, uint64_t _bytes_recv );

    collop_class operator+=(collop_class cs);

};

class io_class {

  public:

    uint64_t num_read;
    uint64_t bytes_read;
    uint64_t num_written;
    uint64_t bytes_written;
    uint64_t num_open;
    uint64_t num_close;
    uint64_t num_seek;

  public:

    io_class();
//     io_class( uint64_t _num_read, uint64_t _bytes_read, uint64_t _num_written,
//               uint64_t _bytes_written, uint64_t _num_open, uint64_t _num_close,
//               uint64_t _num_seek );

    io_class operator+=(io_class is);

};

// DataTdb classes

/* is only used on rank 0 for definitions */
class MasterData {

  public:

    std::string otf_version;
    std::string creator;
    std::string filename;
    std::string filepath;
    std::string hostname;

    uint64_t starttime;
    uint64_t runtime;
    uint64_t filesize;
    uint64_t num_streams;

    uint32_t num_processes;
    uint32_t num_active_processes;
    uint32_t num_threads;

    bool is_compressed;

    bool create_output;


    std::map<std::string, std::string> vt_env;
    std::vector<std::string> counter;

    /* stores all function group definitions */
    /* fgroup_id, fgroup_name */
    std::map<uint32_t, std::string> fgroup;

    /* stores the fgroup_id for all "flush"-functions, */
    /* necessary to check later if it was a vt_flush */
    /* function_id, fgroup_id */
    std::map<uint32_t, uint32_t> flush_fgroup_ids;

    /* save function_id fpr vt_flush here */
    uint32_t vt_flush_id;

    /* collop, type */
    std::map<uint32_t, uint32_t> collop_def;

    /* type, values */
    std::map<uint32_t, collop_class> collop;


  public:

    MasterData();

    ~MasterData();

    uint64_t calc_filesize();

    int set_filepath( std::string file_prefix );

    int set_hostname();

    void set_otf_version();

  private:

    std::string intToHex( int i );
    std::string safe_getcwd();

};

// MPI-Data

class ThreadData {

  public:

    uint64_t num_events;

    uint64_t num_enter;
    uint64_t num_leave;
    uint64_t num_sent;
    uint64_t num_recv;
    uint64_t bytes_sent;
    uint64_t bytes_recv;

    uint64_t num_rma;
    uint64_t bytes_rma;

    uint64_t num_marker;
    uint64_t num_stats;
    uint64_t num_snaps;

    uint64_t num_vt_flushes;

    /* collop, values */
    std::map<uint32_t, collop_class> collop;

    /* ioflag, values */
    std::map<uint32_t, io_class> io;

  public:

    ThreadData();

    ThreadData operator+=(ThreadData td);

    bool toBuffer( uint64_t *buf );

    ThreadData fromBuffer( const uint64_t *buf, const uint64_t *num_bytes );

};







//
// HooksTdb class
//
class HooksTdb : public HooksBase
{
public:

   // contructor
   HooksTdb();

   // destructor
   ~HooksTdb();

   static bool isEnabled(void);

private:

   // initialization/finalization hooks
   //

   void initHook( void );
   void finalizeHook( const bool & error );

   // :TODO:
   // @Johannes: decomment needed hook methods below

   // phase hooks
   //

//    void phaseHook_GetParams_pre( void );
//    void phaseHook_GetParams_post( void );
//
//    void phaseHook_GetUnifyControls_pre( void );
//    void phaseHook_GetUnifyControls_post( void );
//
//    void phaseHook_GetMinStartTime_pre( void );
//    void phaseHook_GetMinStartTime_post( void );
//
//    void phaseHook_UnifyDefinitions_pre( void );
    void phaseHook_UnifyDefinitions_post( void );
//
//    void phaseHook_UnifyMarkers_pre( void );
//    void phaseHook_UnifyMarkers_post( void );
//
//    void phaseHook_UnifyStatistics_pre( void );
//    void phaseHook_UnifyStatistics_post( void );
//
    void phaseHook_UnifyEvents_pre( void );
//    void phaseHook_UnifyEvents_post( void );
//
//    void phaseHook_WriteMasterControl_pre( void );
//    void phaseHook_WriteMasterControl_post( void );
//
//    void phaseHook_CleanUp_pre( void );
//    void phaseHook_CleanUp_post( void );

   // record hooks
   //

   // definition records

//    void readRecHook_DefinitionComment( HooksVaArgs_struct & args );
    void writeRecHook_DefinitionComment( HooksVaArgs_struct & args );
//
//    void readRecHook_DefCreator( HooksVaArgs_struct & args );
    void writeRecHook_DefCreator( HooksVaArgs_struct & args );
//
//    void readRecHook_DefTimerResolution( HooksVaArgs_struct & args );
//    void writeRecHook_DefTimerResolution( HooksVaArgs_struct & args );
//
//    void readRecHook_DefProcessGroup( HooksVaArgs_struct & args );
//    void writeRecHook_DefProcessGroup( HooksVaArgs_struct & args );
//
//    void readRecHook_DefProcess( HooksVaArgs_struct & args );
    void writeRecHook_DefProcess( HooksVaArgs_struct & args );
//
//    void readRecHook_DefSclFile( HooksVaArgs_struct & args );
//    void writeRecHook_DefSclFile( HooksVaArgs_struct & args );
//
//    void readRecHook_DefScl( HooksVaArgs_struct & args );
//    void writeRecHook_DefScl( HooksVaArgs_struct & args );
//
//    void readRecHook_DefFileGroup( HooksVaArgs_struct & args );
//    void writeRecHook_DefFileGroup( HooksVaArgs_struct & args );
//
//    void readRecHook_DefFile( HooksVaArgs_struct & args );
//    void writeRecHook_DefFile( HooksVaArgs_struct & args );
//
//    void readRecHook_DefFunctionGroup( HooksVaArgs_struct & args );
    void writeRecHook_DefFunctionGroup( HooksVaArgs_struct & args );
//
//    void readRecHook_DefFunction( HooksVaArgs_struct & args );
    void writeRecHook_DefFunction( HooksVaArgs_struct & args );
//
//    void readRecHook_DefCollectiveOperation( HooksVaArgs_struct & args );
    void writeRecHook_DefCollectiveOperation( HooksVaArgs_struct & args );
//
//    void readRecHook_DefCounterGroup( HooksVaArgs_struct & args );
//    void writeRecHook_DefCounterGroup( HooksVaArgs_struct & args );
//
//    void readRecHook_DefCounter( HooksVaArgs_struct & args );
    void writeRecHook_DefCounter( HooksVaArgs_struct & args );

   // summary records

//    void readRecHook_FunctionSummary( HooksVaArgs_struct & args );
    void writeRecHook_FunctionSummary( HooksVaArgs_struct & args );
//
//    void readRecHook_MessageSummary( HooksVaArgs_struct & args );
    void writeRecHook_MessageSummary( HooksVaArgs_struct & args );
//
//    void readRecHook_CollopSummary( HooksVaArgs_struct & args );
    void writeRecHook_CollopSummary( HooksVaArgs_struct & args );
//
//    void readRecHook_FileOperationSummary( HooksVaArgs_struct & args );
    void writeRecHook_FileOperationSummary( HooksVaArgs_struct & args );

   // marker records

//    void readRecHook_DefMarker( HooksVaArgs_struct & args );
//    void writeRecHook_DefMarker( HooksVaArgs_struct & args );
//
//    void readRecHook_Marker( HooksVaArgs_struct & args );
    void writeRecHook_Marker( HooksVaArgs_struct & args );

   // event records

//    void readRecHook_Enter( HooksVaArgs_struct & args );
    void writeRecHook_Enter( HooksVaArgs_struct & args );
//
//    void readRecHook_Leave( HooksVaArgs_struct & args );
    void writeRecHook_Leave( HooksVaArgs_struct & args );
//
//    void readRecHook_FileOperation( HooksVaArgs_struct & args );
    void writeRecHook_FileOperation( HooksVaArgs_struct & args );
//
//    void readRecHook_BeginFileOperation( HooksVaArgs_struct & args );
    void writeRecHook_BeginFileOperation( HooksVaArgs_struct & args );
//
//    void readRecHook_EndFileOperation( HooksVaArgs_struct & args );
    void writeRecHook_EndFileOperation( HooksVaArgs_struct & args );

//
//    void readRecHook_SendMsg( HooksVaArgs_struct & args );
    void writeRecHook_SendMsg( HooksVaArgs_struct & args );
//
//    void readRecHook_RecvMsg( HooksVaArgs_struct & args );
    void writeRecHook_RecvMsg( HooksVaArgs_struct & args );
//
//  void readRecHook_CollectiveOperation( HooksVaArgs_struct & args );
    void writeRecHook_CollectiveOperation( HooksVaArgs_struct & args );

//  void readRecHook_BeginCollectiveOperation( HooksVaArgs_struct & args );
    void writeRecHook_BeginCollectiveOperation( HooksVaArgs_struct & args );

//  void readRecHook_EndCollectiveOperation( HooksVaArgs_struct & args );
    void writeRecHook_EndCollectiveOperation( HooksVaArgs_struct & args );

    //
//    void readRecHook_RMAPut( HooksVaArgs_struct & args );
    void writeRecHook_RMAPut( HooksVaArgs_struct & args );
//
//    void readRecHook_RMAPutRemoteEnd( HooksVaArgs_struct & args );
    void writeRecHook_RMAPutRemoteEnd( HooksVaArgs_struct & args );
//
//    void readRecHook_RMAGet( HooksVaArgs_struct & args );
    void writeRecHook_RMAGet( HooksVaArgs_struct & args );
//
//    void readRecHook_RMAEnd( HooksVaArgs_struct & args );
    void writeRecHook_RMAEnd( HooksVaArgs_struct & args );
//
//    void readRecHook_Counter( HooksVaArgs_struct & args );
    void writeRecHook_Counter( HooksVaArgs_struct & args );
//
//    void readRecHook_EventComment( HooksVaArgs_struct & args );
    void writeRecHook_EventComment( HooksVaArgs_struct & args );

    // set thread num hook
    void setThreadNumHook( const int & threadnum );

   // generic hook
//    void genericHook( const uint32_t & id, HooksVaArgs_struct & args );

    void handleFileOperation( uint32_t *operation, uint64_t *bytes);

// variables
public:

    MasterData RootData;
    std::vector<ThreadData> ThreadVector;


};

#endif // _VT_UNIFY_HOOKS_TDB_H_
