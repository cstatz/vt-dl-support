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

#ifndef _VT_UNIFY_HOOKS_BASE_H_
#define _VT_UNIFY_HOOKS_BASE_H_

#include "vt_unify_hooks.h"

//
// HooksBase class
//
class HooksBase
{
   friend class Hooks;

public:

   // contructor
   HooksBase();

   // destructor
   virtual ~HooksBase();

private:

   void triggerPhaseHook( const Hooks::PhaseT & phase );

   void triggerReadRecordHook( const Hooks::RecordT & rectype,
                               HooksVaArgs_struct & args );

   void triggerWriteRecordHook( const Hooks::RecordT & rectype,
                                HooksVaArgs_struct & args );

   // initialization/finalization hooks
   // (must be defined in inheriting hook classes)
   //

   virtual void initHook( void ) = 0;
   virtual void finalizeHook( const bool & error ) = 0;

   // phase hooks
   //

   virtual void phaseHook_GetParams_pre( void );
   virtual void phaseHook_GetParams_post( void );

   virtual void phaseHook_GetUnifyControls_pre( void );
   virtual void phaseHook_GetUnifyControls_post( void );

   virtual void phaseHook_GetMinStartTime_pre( void );
   virtual void phaseHook_GetMinStartTime_post( void );

   virtual void phaseHook_UnifyDefinitions_pre( void );
   virtual void phaseHook_UnifyDefinitions_post( void );

   virtual void phaseHook_UnifyMarkers_pre( void );
   virtual void phaseHook_UnifyMarkers_post( void );

   virtual void phaseHook_UnifyStatistics_pre( void );
   virtual void phaseHook_UnifyStatistics_post( void );

   virtual void phaseHook_UnifyEvents_pre( void );
   virtual void phaseHook_UnifyEvents_post( void );

   virtual void phaseHook_WriteMasterControl_pre( void );
   virtual void phaseHook_WriteMasterControl_post( void );

   virtual void phaseHook_CleanUp_pre( void );
   virtual void phaseHook_CleanUp_post( void );

   // record hooks
   //

   // definition records

   virtual void readRecHook_DefinitionComment( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefinitionComment( HooksVaArgs_struct & args );

   virtual void readRecHook_DefCreator( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefCreator( HooksVaArgs_struct & args );

   virtual void readRecHook_DefTimerResolution( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefTimerResolution( HooksVaArgs_struct & args );

   virtual void readRecHook_DefProcessGroup( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefProcessGroup( HooksVaArgs_struct & args );

   virtual void readRecHook_DefProcess( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefProcess( HooksVaArgs_struct & args );

   virtual void readRecHook_DefSclFile( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefSclFile( HooksVaArgs_struct & args );

   virtual void readRecHook_DefScl( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefScl( HooksVaArgs_struct & args );

   virtual void readRecHook_DefFileGroup( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefFileGroup( HooksVaArgs_struct & args );

   virtual void readRecHook_DefFile( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefFile( HooksVaArgs_struct & args );

   virtual void readRecHook_DefFunctionGroup( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefFunctionGroup( HooksVaArgs_struct & args );

   virtual void readRecHook_DefFunction( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefFunction( HooksVaArgs_struct & args );

   virtual void readRecHook_DefCollectiveOperation( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefCollectiveOperation( HooksVaArgs_struct & args );

   virtual void readRecHook_DefCounterGroup( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefCounterGroup( HooksVaArgs_struct & args );

   virtual void readRecHook_DefCounter( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefCounter( HooksVaArgs_struct & args );

   // summary records

   virtual void readRecHook_FunctionSummary( HooksVaArgs_struct & args );
   virtual void writeRecHook_FunctionSummary( HooksVaArgs_struct & args );

   virtual void readRecHook_MessageSummary( HooksVaArgs_struct & args );
   virtual void writeRecHook_MessageSummary( HooksVaArgs_struct & args );

   virtual void readRecHook_CollopSummary( HooksVaArgs_struct & args );
   virtual void writeRecHook_CollopSummary( HooksVaArgs_struct & args );

   virtual void readRecHook_FileOperationSummary( HooksVaArgs_struct & args );
   virtual void writeRecHook_FileOperationSummary( HooksVaArgs_struct & args );

   // marker records

   virtual void readRecHook_DefMarker( HooksVaArgs_struct & args );
   virtual void writeRecHook_DefMarker( HooksVaArgs_struct & args );

   virtual void readRecHook_Marker( HooksVaArgs_struct & args );
   virtual void writeRecHook_Marker( HooksVaArgs_struct & args );

   // event records

   virtual void readRecHook_Enter( HooksVaArgs_struct & args );
   virtual void writeRecHook_Enter( HooksVaArgs_struct & args );

   virtual void readRecHook_Leave( HooksVaArgs_struct & args );
   virtual void writeRecHook_Leave( HooksVaArgs_struct & args );

   virtual void readRecHook_FileOperation( HooksVaArgs_struct & args );
   virtual void writeRecHook_FileOperation( HooksVaArgs_struct & args );

   virtual void readRecHook_BeginFileOperation( HooksVaArgs_struct & args );
   virtual void writeRecHook_BeginFileOperation( HooksVaArgs_struct & args );

   virtual void readRecHook_EndFileOperation( HooksVaArgs_struct & args );
   virtual void writeRecHook_EndFileOperation( HooksVaArgs_struct & args );

   virtual void readRecHook_SendMsg( HooksVaArgs_struct & args );
   virtual void writeRecHook_SendMsg( HooksVaArgs_struct & args );

   virtual void readRecHook_RecvMsg( HooksVaArgs_struct & args );
   virtual void writeRecHook_RecvMsg( HooksVaArgs_struct & args );

   virtual void readRecHook_CollectiveOperation( HooksVaArgs_struct & args );
   virtual void writeRecHook_CollectiveOperation( HooksVaArgs_struct & args );

   virtual void readRecHook_BeginCollectiveOperation( HooksVaArgs_struct & args );
   virtual void writeRecHook_BeginCollectiveOperation( HooksVaArgs_struct & args );

   virtual void readRecHook_EndCollectiveOperation( HooksVaArgs_struct & args );
   virtual void writeRecHook_EndCollectiveOperation( HooksVaArgs_struct & args );

   virtual void readRecHook_RMAPut( HooksVaArgs_struct & args );
   virtual void writeRecHook_RMAPut( HooksVaArgs_struct & args );

   virtual void readRecHook_RMAPutRemoteEnd( HooksVaArgs_struct & args );
   virtual void writeRecHook_RMAPutRemoteEnd( HooksVaArgs_struct & args );

   virtual void readRecHook_RMAGet( HooksVaArgs_struct & args );
   virtual void writeRecHook_RMAGet( HooksVaArgs_struct & args );

   virtual void readRecHook_RMAEnd( HooksVaArgs_struct & args );
   virtual void writeRecHook_RMAEnd( HooksVaArgs_struct & args );

   virtual void readRecHook_Counter( HooksVaArgs_struct & args );
   virtual void writeRecHook_Counter( HooksVaArgs_struct & args );

   virtual void readRecHook_EventComment( HooksVaArgs_struct & args );
   virtual void writeRecHook_EventComment( HooksVaArgs_struct & args );

   // set thread num hook
   virtual void setThreadNumHook( const int & threadnum );

   // generic hook
   virtual void genericHook( const uint32_t & id, HooksVaArgs_struct & args );

   std::vector<void (HooksBase::*)(void)>                m_vecPhaseMethods;
   std::vector<void (HooksBase::*)(HooksVaArgs_struct&)> m_vecReadRecHookMethods;
   std::vector<void (HooksBase::*)(HooksVaArgs_struct&)> m_vecWriteRecHookMethods;

};

#endif // _VT_UNIFY_HOOKS_BASE_H_
