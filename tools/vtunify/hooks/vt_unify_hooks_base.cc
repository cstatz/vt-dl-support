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

#include "vt_unify_hooks_base.h"

#include "vt_inttypes.h"

#include <assert.h>

//////////////////// class HooksBase ////////////////////

// public methods
//

HooksBase::HooksBase()
{
   // register pointer of hook methods
   //

   // phase hooks
   //

   m_vecPhaseMethods.resize( Hooks::Phase_Num, 0 );

   m_vecPhaseMethods[Hooks::Phase_GetParams_pre] =
      &HooksBase::phaseHook_GetParams_pre;
   m_vecPhaseMethods[Hooks::Phase_GetParams_post] =
      &HooksBase::phaseHook_GetParams_post;

   m_vecPhaseMethods[Hooks::Phase_GetUnifyControls_pre] =
      &HooksBase::phaseHook_GetUnifyControls_pre;
   m_vecPhaseMethods[Hooks::Phase_GetUnifyControls_post] =
      &HooksBase::phaseHook_GetUnifyControls_post;

   m_vecPhaseMethods[Hooks::Phase_GetMinStartTime_pre] =
      &HooksBase::phaseHook_GetMinStartTime_pre;
   m_vecPhaseMethods[Hooks::Phase_GetMinStartTime_post] =
      &HooksBase::phaseHook_GetMinStartTime_post;

   m_vecPhaseMethods[Hooks::Phase_UnifyDefinitions_pre] =
      &HooksBase::phaseHook_UnifyDefinitions_pre;
   m_vecPhaseMethods[Hooks::Phase_UnifyDefinitions_post] =
      &HooksBase::phaseHook_UnifyDefinitions_post;

   m_vecPhaseMethods[Hooks::Phase_UnifyMarkers_pre] =
      &HooksBase::phaseHook_UnifyMarkers_pre;
   m_vecPhaseMethods[Hooks::Phase_UnifyMarkers_post] =
      &HooksBase::phaseHook_UnifyMarkers_post;

   m_vecPhaseMethods[Hooks::Phase_UnifyStatistics_pre] =
      &HooksBase::phaseHook_UnifyStatistics_pre;
   m_vecPhaseMethods[Hooks::Phase_UnifyStatistics_post] =
      &HooksBase::phaseHook_UnifyStatistics_post;

   m_vecPhaseMethods[Hooks::Phase_UnifyEvents_pre] =
      &HooksBase::phaseHook_UnifyEvents_pre;
   m_vecPhaseMethods[Hooks::Phase_UnifyEvents_post] =
      &HooksBase::phaseHook_UnifyEvents_post;

   m_vecPhaseMethods[Hooks::Phase_WriteMasterControl_pre] =
      &HooksBase::phaseHook_WriteMasterControl_pre;
   m_vecPhaseMethods[Hooks::Phase_WriteMasterControl_post] =
      &HooksBase::phaseHook_WriteMasterControl_post;

   m_vecPhaseMethods[Hooks::Phase_CleanUp_pre] =
      &HooksBase::phaseHook_CleanUp_pre;
   m_vecPhaseMethods[Hooks::Phase_CleanUp_post] =
      &HooksBase::phaseHook_CleanUp_post;

   // record hooks
   //

   m_vecReadRecHookMethods.resize( Hooks::Record_Num, 0 );
   m_vecWriteRecHookMethods.resize( Hooks::Record_Num, 0 );

   // definition records

   m_vecReadRecHookMethods[Hooks::Record_DefinitionComment] =
      &HooksBase::readRecHook_DefinitionComment;
   m_vecWriteRecHookMethods[Hooks::Record_DefinitionComment] =
      &HooksBase::writeRecHook_DefinitionComment;

   m_vecReadRecHookMethods[Hooks::Record_DefCreator] =
      &HooksBase::readRecHook_DefCreator;
   m_vecWriteRecHookMethods[Hooks::Record_DefCreator] =
      &HooksBase::writeRecHook_DefCreator;

   m_vecReadRecHookMethods[Hooks::Record_DefTimerResolution] =
      &HooksBase::readRecHook_DefTimerResolution;
   m_vecWriteRecHookMethods[Hooks::Record_DefTimerResolution] =
      &HooksBase::writeRecHook_DefTimerResolution;

   m_vecReadRecHookMethods[Hooks::Record_DefProcessGroup] =
      &HooksBase::readRecHook_DefProcessGroup;
   m_vecWriteRecHookMethods[Hooks::Record_DefProcessGroup] =
      &HooksBase::writeRecHook_DefProcessGroup;

   m_vecReadRecHookMethods[Hooks::Record_DefProcess] =
      &HooksBase::readRecHook_DefProcess;
   m_vecWriteRecHookMethods[Hooks::Record_DefProcess] =
      &HooksBase::writeRecHook_DefProcess;

   m_vecReadRecHookMethods[Hooks::Record_DefSclFile] =
      &HooksBase::readRecHook_DefSclFile;
   m_vecWriteRecHookMethods[Hooks::Record_DefSclFile] =
      &HooksBase::writeRecHook_DefSclFile;

   m_vecReadRecHookMethods[Hooks::Record_DefScl] =
      &HooksBase::readRecHook_DefScl;
   m_vecWriteRecHookMethods[Hooks::Record_DefScl] =
      &HooksBase::writeRecHook_DefScl;

   m_vecReadRecHookMethods[Hooks::Record_DefFileGroup] =
      &HooksBase::readRecHook_DefFileGroup;
   m_vecWriteRecHookMethods[Hooks::Record_DefFileGroup] =
      &HooksBase::writeRecHook_DefFileGroup;

   m_vecReadRecHookMethods[Hooks::Record_DefFile] =
      &HooksBase::readRecHook_DefFile;
   m_vecWriteRecHookMethods[Hooks::Record_DefFile] =
      &HooksBase::writeRecHook_DefFile;

   m_vecReadRecHookMethods[Hooks::Record_DefFunctionGroup] =
      &HooksBase::readRecHook_DefFunctionGroup;
   m_vecWriteRecHookMethods[Hooks::Record_DefFunctionGroup] =
      &HooksBase::writeRecHook_DefFunctionGroup;

   m_vecReadRecHookMethods[Hooks::Record_DefFunction] =
      &HooksBase::readRecHook_DefFunction;
   m_vecWriteRecHookMethods[Hooks::Record_DefFunction] =
      &HooksBase::writeRecHook_DefFunction;

   m_vecReadRecHookMethods[Hooks::Record_DefCollectiveOperation] =
      &HooksBase::readRecHook_DefCollectiveOperation;
   m_vecWriteRecHookMethods[Hooks::Record_DefCollectiveOperation] =
      &HooksBase::writeRecHook_DefCollectiveOperation;

   m_vecReadRecHookMethods[Hooks::Record_DefCounterGroup] =
      &HooksBase::readRecHook_DefCounterGroup;
   m_vecWriteRecHookMethods[Hooks::Record_DefCounterGroup] =
      &HooksBase::writeRecHook_DefCounterGroup;

   m_vecReadRecHookMethods[Hooks::Record_DefCounter] =
      &HooksBase::readRecHook_DefCounter;
   m_vecWriteRecHookMethods[Hooks::Record_DefCounter] =
      &HooksBase::writeRecHook_DefCounter;

   // summary records

   m_vecReadRecHookMethods[Hooks::Record_FunctionSummary] =
      &HooksBase::readRecHook_FunctionSummary;
   m_vecWriteRecHookMethods[Hooks::Record_FunctionSummary] =
      &HooksBase::writeRecHook_FunctionSummary;

   m_vecReadRecHookMethods[Hooks::Record_MessageSummary] =
      &HooksBase::readRecHook_MessageSummary;
   m_vecWriteRecHookMethods[Hooks::Record_MessageSummary] =
      &HooksBase::writeRecHook_MessageSummary;

   m_vecReadRecHookMethods[Hooks::Record_CollopSummary] =
      &HooksBase::readRecHook_CollopSummary;
   m_vecWriteRecHookMethods[Hooks::Record_CollopSummary] =
      &HooksBase::writeRecHook_CollopSummary;

   m_vecReadRecHookMethods[Hooks::Record_FileOperationSummary] =
      &HooksBase::readRecHook_FileOperationSummary;
   m_vecWriteRecHookMethods[Hooks::Record_FileOperationSummary] =
      &HooksBase::writeRecHook_FileOperationSummary;

   // marker records

   m_vecReadRecHookMethods[Hooks::Record_DefMarker] =
      &HooksBase::readRecHook_DefMarker;
   m_vecWriteRecHookMethods[Hooks::Record_DefMarker] =
      &HooksBase::writeRecHook_DefMarker;

   m_vecReadRecHookMethods[Hooks::Record_Marker] =
      &HooksBase::readRecHook_Marker;
   m_vecWriteRecHookMethods[Hooks::Record_Marker] =
      &HooksBase::writeRecHook_Marker;

   // event records

   m_vecReadRecHookMethods[Hooks::Record_Enter] =
      &HooksBase::readRecHook_Enter;
   m_vecWriteRecHookMethods[Hooks::Record_Enter] =
      &HooksBase::writeRecHook_Enter;

   m_vecReadRecHookMethods[Hooks::Record_Leave] =
      &HooksBase::readRecHook_Leave;
   m_vecWriteRecHookMethods[Hooks::Record_Leave] =
      &HooksBase::writeRecHook_Leave;

   m_vecReadRecHookMethods[Hooks::Record_FileOperation] =
      &HooksBase::readRecHook_FileOperation;
   m_vecWriteRecHookMethods[Hooks::Record_FileOperation] =
      &HooksBase::writeRecHook_FileOperation;

   m_vecReadRecHookMethods[Hooks::Record_BeginFileOperation] =
      &HooksBase::readRecHook_BeginFileOperation;
   m_vecWriteRecHookMethods[Hooks::Record_BeginFileOperation] =
      &HooksBase::writeRecHook_BeginFileOperation;

   m_vecReadRecHookMethods[Hooks::Record_EndFileOperation] =
      &HooksBase::readRecHook_EndFileOperation;
   m_vecWriteRecHookMethods[Hooks::Record_EndFileOperation] =
      &HooksBase::writeRecHook_EndFileOperation;

   m_vecReadRecHookMethods[Hooks::Record_SendMsg] =
      &HooksBase::readRecHook_SendMsg;
   m_vecWriteRecHookMethods[Hooks::Record_SendMsg] =
      &HooksBase::writeRecHook_SendMsg;

   m_vecReadRecHookMethods[Hooks::Record_RecvMsg] =
      &HooksBase::readRecHook_RecvMsg;
   m_vecWriteRecHookMethods[Hooks::Record_RecvMsg] =
      &HooksBase::writeRecHook_RecvMsg;

   m_vecReadRecHookMethods[Hooks::Record_CollectiveOperation] =
      &HooksBase::readRecHook_CollectiveOperation;
   m_vecWriteRecHookMethods[Hooks::Record_CollectiveOperation] =
      &HooksBase::writeRecHook_CollectiveOperation;

   m_vecReadRecHookMethods[Hooks::Record_BeginCollectiveOperation] =
      &HooksBase::readRecHook_BeginCollectiveOperation;
   m_vecWriteRecHookMethods[Hooks::Record_BeginCollectiveOperation] =
      &HooksBase::writeRecHook_BeginCollectiveOperation;

   m_vecReadRecHookMethods[Hooks::Record_EndCollectiveOperation] =
      &HooksBase::readRecHook_EndCollectiveOperation;
   m_vecWriteRecHookMethods[Hooks::Record_EndCollectiveOperation] =
      &HooksBase::writeRecHook_EndCollectiveOperation;

   m_vecReadRecHookMethods[Hooks::Record_RMAPut] =
      &HooksBase::readRecHook_RMAPut;
   m_vecWriteRecHookMethods[Hooks::Record_RMAPut] =
      &HooksBase::writeRecHook_RMAPut;

   m_vecReadRecHookMethods[Hooks::Record_RMAPutRemoteEnd] =
      &HooksBase::readRecHook_RMAPutRemoteEnd;
   m_vecWriteRecHookMethods[Hooks::Record_RMAPutRemoteEnd] =
      &HooksBase::writeRecHook_RMAPutRemoteEnd;

   m_vecReadRecHookMethods[Hooks::Record_RMAGet] =
      &HooksBase::readRecHook_RMAGet;
   m_vecWriteRecHookMethods[Hooks::Record_RMAGet] =
      &HooksBase::writeRecHook_RMAGet;

   m_vecReadRecHookMethods[Hooks::Record_RMAEnd] =
      &HooksBase::readRecHook_RMAEnd;
   m_vecWriteRecHookMethods[Hooks::Record_RMAEnd] =
      &HooksBase::writeRecHook_RMAEnd;

   m_vecReadRecHookMethods[Hooks::Record_Counter] =
      &HooksBase::readRecHook_Counter;
   m_vecWriteRecHookMethods[Hooks::Record_Counter] =
      &HooksBase::writeRecHook_Counter;

   m_vecReadRecHookMethods[Hooks::Record_EventComment] =
      &HooksBase::readRecHook_EventComment;
   m_vecWriteRecHookMethods[Hooks::Record_EventComment] =
      &HooksBase::writeRecHook_EventComment;
}

HooksBase::~HooksBase()
{
   // Empty
}


// private methods
//

void
HooksBase::triggerPhaseHook( const Hooks::PhaseT & phase )
{
   assert( m_vecPhaseMethods.size() > (uint32_t)phase );
   assert( m_vecPhaseMethods[phase] != 0 );

   (this->*(m_vecPhaseMethods[phase]))();
}

void
HooksBase::triggerReadRecordHook( const Hooks::RecordT & rectype,
                                  HooksVaArgs_struct & args )
{
   assert( m_vecReadRecHookMethods.size() > (uint32_t)rectype );
   assert( m_vecReadRecHookMethods[rectype] != 0 );

   (this->*(m_vecReadRecHookMethods[rectype]))( args );
}

void
HooksBase::triggerWriteRecordHook( const Hooks::RecordT & rectype,
                                   HooksVaArgs_struct & args )
{
   assert( m_vecWriteRecHookMethods.size() > (uint32_t)rectype );
   assert( m_vecWriteRecHookMethods[rectype] != 0 );

   (this->*(m_vecWriteRecHookMethods[rectype]))( args );
}

// initialization/finalization hooks
// (declarated as abstract methods; must be defined in inheriting hook classes)
//

//void HooksBase::initHook() {}
//void HooksBase::finalizeHook( const bool & error ) {}

// phase hooks
//

void HooksBase::phaseHook_GetParams_pre() {}
void HooksBase::phaseHook_GetParams_post() {}

void HooksBase::phaseHook_GetUnifyControls_pre() {}
void HooksBase::phaseHook_GetUnifyControls_post() {}

void HooksBase::phaseHook_GetMinStartTime_pre() {}
void HooksBase::phaseHook_GetMinStartTime_post() {}

void HooksBase::phaseHook_UnifyDefinitions_pre() {}
void HooksBase::phaseHook_UnifyDefinitions_post() {}

void HooksBase::phaseHook_UnifyMarkers_pre() {}
void HooksBase::phaseHook_UnifyMarkers_post() {}

void HooksBase::phaseHook_UnifyStatistics_pre() {}
void HooksBase::phaseHook_UnifyStatistics_post() {}

void HooksBase::phaseHook_UnifyEvents_pre() {}
void HooksBase::phaseHook_UnifyEvents_post() {}

void HooksBase::phaseHook_WriteMasterControl_pre() {}
void HooksBase::phaseHook_WriteMasterControl_post() {}

void HooksBase::phaseHook_CleanUp_pre() {}
void HooksBase::phaseHook_CleanUp_post() {}

// record hooks
//

// definition records

void HooksBase::readRecHook_DefinitionComment( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefinitionComment( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_DefCreator( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefCreator( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_DefTimerResolution( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefTimerResolution( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_DefProcessGroup( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefProcessGroup( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_DefProcess( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefProcess( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_DefSclFile( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefSclFile( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_DefScl( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefScl( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_DefFileGroup( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefFileGroup( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_DefFile( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefFile( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_DefFunctionGroup( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefFunctionGroup( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_DefFunction( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefFunction( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_DefCollectiveOperation( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefCollectiveOperation( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_DefCounterGroup( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefCounterGroup( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_DefCounter( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefCounter( HooksVaArgs_struct & args ) { (void)args; }

// summary records

void HooksBase::readRecHook_FunctionSummary( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_FunctionSummary( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_MessageSummary( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_MessageSummary( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_CollopSummary( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_CollopSummary( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_FileOperationSummary( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_FileOperationSummary( HooksVaArgs_struct & args ) { (void)args; }

// marker records

void HooksBase::readRecHook_DefMarker( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_DefMarker( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_Marker( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_Marker( HooksVaArgs_struct & args ) { (void)args; }

// event records

void HooksBase::readRecHook_Enter( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_Enter( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_Leave( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_Leave( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_FileOperation( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_FileOperation( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_BeginFileOperation( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_BeginFileOperation( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_EndFileOperation( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_EndFileOperation( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_SendMsg( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_SendMsg( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_RecvMsg( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_RecvMsg( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_CollectiveOperation( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_CollectiveOperation( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_BeginCollectiveOperation( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_BeginCollectiveOperation( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_EndCollectiveOperation( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_EndCollectiveOperation( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_RMAPut( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_RMAPut( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_RMAPutRemoteEnd( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_RMAPutRemoteEnd( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_RMAGet( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_RMAGet( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_RMAEnd( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_RMAEnd( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_Counter( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_Counter( HooksVaArgs_struct & args ) { (void)args; }

void HooksBase::readRecHook_EventComment( HooksVaArgs_struct & args ) { (void)args; }
void HooksBase::writeRecHook_EventComment( HooksVaArgs_struct & args ) { (void)args; }

// set thread num hook
void HooksBase::setThreadNumHook( const int & threadnum ) { (void)threadnum; }

// generic hook
void HooksBase::genericHook( const uint32_t & id, HooksVaArgs_struct & args ) { (void)args; }
