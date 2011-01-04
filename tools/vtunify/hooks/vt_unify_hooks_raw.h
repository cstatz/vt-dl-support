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

#ifndef _VT_UNIFY_HOOKS_RAW_H_
#define _VT_UNIFY_HOOKS_RAW_H_

#include "vt_unify_hooks_base.h"

//
// HooksRaw class
//
class HooksRaw : public HooksBase
{
public:

   // contructor
   HooksRaw();

   // destructor
   ~HooksRaw();

private:

   // initialization/finalization hooks
   //

   void initHook( void );
   void finalizeHook( const bool & error );

   // phase hooks
   //

   void phaseHook_GetParams_pre( void );
   void phaseHook_GetParams_post( void );

   void phaseHook_GetUnifyControls_pre( void );
   void phaseHook_GetUnifyControls_post( void );

   void phaseHook_GetMinStartTime_pre( void );
   void phaseHook_GetMinStartTime_post( void );

   void phaseHook_UnifyDefinitions_pre( void );
   void phaseHook_UnifyDefinitions_post( void );

   void phaseHook_UnifyMarkers_pre( void );
   void phaseHook_UnifyMarkers_post( void );

   void phaseHook_UnifyStatistics_pre( void );
   void phaseHook_UnifyStatistics_post( void );

   void phaseHook_UnifyEvents_pre( void );
   void phaseHook_UnifyEvents_post( void );

   void phaseHook_WriteMasterControl_pre( void );
   void phaseHook_WriteMasterControl_post( void );

   void phaseHook_CleanUp_pre( void );
   void phaseHook_CleanUp_post( void );

   // record hooks
   //

   // definition records

   void readRecHook_DefinitionComment( HooksVaArgs_struct & args );
   void writeRecHook_DefinitionComment( HooksVaArgs_struct & args );

   void readRecHook_DefCreator( HooksVaArgs_struct & args );
   void writeRecHook_DefCreator( HooksVaArgs_struct & args );

   void readRecHook_DefTimerResolution( HooksVaArgs_struct & args );
   void writeRecHook_DefTimerResolution( HooksVaArgs_struct & args );

   void readRecHook_DefProcessGroup( HooksVaArgs_struct & args );
   void writeRecHook_DefProcessGroup( HooksVaArgs_struct & args );

   void readRecHook_DefProcess( HooksVaArgs_struct & args );
   void writeRecHook_DefProcess( HooksVaArgs_struct & args );

   void readRecHook_DefSclFile( HooksVaArgs_struct & args );
   void writeRecHook_DefSclFile( HooksVaArgs_struct & args );

   void readRecHook_DefScl( HooksVaArgs_struct & args );
   void writeRecHook_DefScl( HooksVaArgs_struct & args );

   void readRecHook_DefFileGroup( HooksVaArgs_struct & args );
   void writeRecHook_DefFileGroup( HooksVaArgs_struct & args );

   void readRecHook_DefFile( HooksVaArgs_struct & args );
   void writeRecHook_DefFile( HooksVaArgs_struct & args );

   void readRecHook_DefFunctionGroup( HooksVaArgs_struct & args );
   void writeRecHook_DefFunctionGroup( HooksVaArgs_struct & args );

   void readRecHook_DefFunction( HooksVaArgs_struct & args );
   void writeRecHook_DefFunction( HooksVaArgs_struct & args );

   void readRecHook_DefCollectiveOperation( HooksVaArgs_struct & args );
   void writeRecHook_DefCollectiveOperation( HooksVaArgs_struct & args );

   void readRecHook_DefCounterGroup( HooksVaArgs_struct & args );
   void writeRecHook_DefCounterGroup( HooksVaArgs_struct & args );

   void readRecHook_DefCounter( HooksVaArgs_struct & args );
   void writeRecHook_DefCounter( HooksVaArgs_struct & args );

   // summary records

   void readRecHook_FunctionSummary( HooksVaArgs_struct & args );
   void writeRecHook_FunctionSummary( HooksVaArgs_struct & args );

   void readRecHook_MessageSummary( HooksVaArgs_struct & args );
   void writeRecHook_MessageSummary( HooksVaArgs_struct & args );

   void readRecHook_CollopSummary( HooksVaArgs_struct & args );
   void writeRecHook_CollopSummary( HooksVaArgs_struct & args );

   void readRecHook_FileOperationSummary( HooksVaArgs_struct & args );
   void writeRecHook_FileOperationSummary( HooksVaArgs_struct & args );

   // marker records

   void readRecHook_DefMarker( HooksVaArgs_struct & args );
   void writeRecHook_DefMarker( HooksVaArgs_struct & args );

   void readRecHook_Marker( HooksVaArgs_struct & args );
   void writeRecHook_Marker( HooksVaArgs_struct & args );

   // event records

   void readRecHook_Enter( HooksVaArgs_struct & args );
   void writeRecHook_Enter( HooksVaArgs_struct & args );

   void readRecHook_Leave( HooksVaArgs_struct & args );
   void writeRecHook_Leave( HooksVaArgs_struct & args );

   void readRecHook_FileOperation( HooksVaArgs_struct & args );
   void writeRecHook_FileOperation( HooksVaArgs_struct & args );

   void readRecHook_BeginFileOperation( HooksVaArgs_struct & args );
   void writeRecHook_BeginFileOperation( HooksVaArgs_struct & args );

   void readRecHook_EndFileOperation( HooksVaArgs_struct & args );
   void writeRecHook_EndFileOperation( HooksVaArgs_struct & args );

   void readRecHook_SendMsg( HooksVaArgs_struct & args );
   void writeRecHook_SendMsg( HooksVaArgs_struct & args );

   void readRecHook_RecvMsg( HooksVaArgs_struct & args );
   void writeRecHook_RecvMsg( HooksVaArgs_struct & args );

   void readRecHook_CollectiveOperation( HooksVaArgs_struct & args );
   void writeRecHook_CollectiveOperation( HooksVaArgs_struct & args );

   void readRecHook_BeginCollectiveOperation( HooksVaArgs_struct & args );
   void writeRecHook_BeginCollectiveOperation( HooksVaArgs_struct & args );

   void readRecHook_EndCollectiveOperation( HooksVaArgs_struct & args );
   void writeRecHook_EndCollectiveOperation( HooksVaArgs_struct & args );

   void readRecHook_RMAPut( HooksVaArgs_struct & args );
   void writeRecHook_RMAPut( HooksVaArgs_struct & args );

   void readRecHook_RMAPutRemoteEnd( HooksVaArgs_struct & args );
   void writeRecHook_RMAPutRemoteEnd( HooksVaArgs_struct & args );

   void readRecHook_RMAGet( HooksVaArgs_struct & args );
   void writeRecHook_RMAGet( HooksVaArgs_struct & args );

   void readRecHook_RMAEnd( HooksVaArgs_struct & args );
   void writeRecHook_RMAEnd( HooksVaArgs_struct & args );

   void readRecHook_Counter( HooksVaArgs_struct & args );
   void writeRecHook_Counter( HooksVaArgs_struct & args );

   void readRecHook_EventComment( HooksVaArgs_struct & args );
   void writeRecHook_EventComment( HooksVaArgs_struct & args );

   // generic hook
   void genericHook( const uint32_t & id, HooksVaArgs_struct & args );

};

#endif // _VT_UNIFY_HOOKS_RAW_H_
