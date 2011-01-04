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

#include "vt_unify.h"
#include "vt_unify_hooks_raw.h"

#include "vt_inttypes.h"

#include <iostream>
#include <string>

// set __func__ to a dummy for compilers not supporting this macro
//
#ifndef __func__
#  define __func__ "<unknown function>"
#endif // __func__

//////////////////// class HooksRaw ////////////////////

// public methods
//

HooksRaw::HooksRaw() : HooksBase()
{
   // Empty
}

HooksRaw::~HooksRaw()
{
   // Empty
}

// private methods
//

#define DOSOMETHING \
   PVPrint( 3, "Triggerd raw hook %s\n", __func__ )

// initialization/finalization hooks
//

void HooksRaw::initHook( void ) { DOSOMETHING; }
void HooksRaw::finalizeHook( const bool & error ) { DOSOMETHING; }

// phase hooks
//

void HooksRaw::phaseHook_GetParams_pre() { DOSOMETHING; }
void HooksRaw::phaseHook_GetParams_post() { DOSOMETHING; }

void HooksRaw::phaseHook_GetUnifyControls_pre() { DOSOMETHING; }
void HooksRaw::phaseHook_GetUnifyControls_post() { DOSOMETHING; }

void HooksRaw::phaseHook_GetMinStartTime_pre() { DOSOMETHING; }
void HooksRaw::phaseHook_GetMinStartTime_post() { DOSOMETHING; }

void HooksRaw::phaseHook_UnifyDefinitions_pre() { DOSOMETHING; }
void HooksRaw::phaseHook_UnifyDefinitions_post() { DOSOMETHING; }

void HooksRaw::phaseHook_UnifyMarkers_pre() { DOSOMETHING; }
void HooksRaw::phaseHook_UnifyMarkers_post() { DOSOMETHING; }

void HooksRaw::phaseHook_UnifyStatistics_pre() { DOSOMETHING; }
void HooksRaw::phaseHook_UnifyStatistics_post() { DOSOMETHING; }

void HooksRaw::phaseHook_UnifyEvents_pre() { DOSOMETHING; }
void HooksRaw::phaseHook_UnifyEvents_post() { DOSOMETHING; }

void HooksRaw::phaseHook_WriteMasterControl_pre() { DOSOMETHING; }
void HooksRaw::phaseHook_WriteMasterControl_post() { DOSOMETHING; }

void HooksRaw::phaseHook_CleanUp_pre() { DOSOMETHING; }
void HooksRaw::phaseHook_CleanUp_post() { DOSOMETHING; }

// record hooks
//

// definition records

void HooksRaw::readRecHook_DefinitionComment( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefinitionComment( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_DefCreator( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefCreator( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_DefTimerResolution( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefTimerResolution( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_DefProcessGroup( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefProcessGroup( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_DefProcess( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefProcess( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_DefSclFile( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefSclFile( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_DefScl( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefScl( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_DefFileGroup( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefFileGroup( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_DefFile( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefFile( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_DefFunctionGroup( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefFunctionGroup( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_DefFunction( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefFunction( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_DefCollectiveOperation( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefCollectiveOperation( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_DefCounterGroup( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefCounterGroup( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_DefCounter( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefCounter( HooksVaArgs_struct & args ) { DOSOMETHING; }

// summary records

void HooksRaw::readRecHook_FunctionSummary( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_FunctionSummary( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_MessageSummary( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_MessageSummary( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_CollopSummary( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_CollopSummary( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_FileOperationSummary( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_FileOperationSummary( HooksVaArgs_struct & args ) { DOSOMETHING; }

// marker records

void HooksRaw::readRecHook_DefMarker( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_DefMarker( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_Marker( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_Marker( HooksVaArgs_struct & args ) { DOSOMETHING; }

// event records

void HooksRaw::readRecHook_Enter( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_Enter( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_Leave( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_Leave( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_FileOperation( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_FileOperation( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_BeginFileOperation( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_BeginFileOperation( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_EndFileOperation( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_EndFileOperation( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_SendMsg( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_SendMsg( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_RecvMsg( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_RecvMsg( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_CollectiveOperation( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_CollectiveOperation( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_BeginCollectiveOperation( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_BeginCollectiveOperation( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_EndCollectiveOperation( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_EndCollectiveOperation( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_RMAPut( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_RMAPut( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_RMAPutRemoteEnd( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_RMAPutRemoteEnd( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_RMAGet( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_RMAGet( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_RMAEnd( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_RMAEnd( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_Counter( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_Counter( HooksVaArgs_struct & args ) { DOSOMETHING; }

void HooksRaw::readRecHook_EventComment( HooksVaArgs_struct & args ) { DOSOMETHING; }
void HooksRaw::writeRecHook_EventComment( HooksVaArgs_struct & args ) { DOSOMETHING; }

// generic hook
void HooksRaw::genericHook( const uint32_t & id, HooksVaArgs_struct & args ) { DOSOMETHING; }
