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

#include "vt_unify_hooks.h"

#include "hooks/vt_unify_hooks_base.h"

#ifdef VT_UNIFY_HOOKS_RAW
# include "hooks/vt_unify_hooks_raw.h"
#endif // VT_UNIFY_HOOKS_RAW
#ifdef VT_UNIFY_HOOKS_STATS
# include "hooks/vt_unify_hooks_stats.h"
#endif // VT_UNIFY_HOOKS_STATS
#ifdef VT_UNIFY_HOOKS_TDB
# include "hooks/vt_unify_hooks_tdb.h"
#endif // VT_UNIFY_HOOKS_TDB

// storage for variable hook arguments
HooksVaArgs_struct HooksVaArgs;
#if defined(HAVE_OMP) && HAVE_OMP
#  pragma omp threadprivate(HooksVaArgs)
#endif // HAVE_OMP

Hooks * theHooks; // instance of class Hooks

//////////////////// class Hooks ////////////////////

// public methods
//
#include <iostream>
Hooks::Hooks()
{
   // "register" hook classes

#ifdef VT_UNIFY_HOOKS_RAW
   m_vecHooks.push_back( new HooksRaw() );
#endif // VT_UNIFY_HOOKS_RAW
#ifdef VT_UNIFY_HOOKS_STATS
   m_vecHooks.push_back( new HooksStats() );
#endif // VT_UNIFY_HOOKS_STATS
#ifdef VT_UNIFY_HOOKS_TDB
   if( HooksTdb::isEnabled() )
      m_vecHooks.push_back( new HooksTdb() );
#endif // VT_UNIFY_HOOKS_TDB
}

Hooks::~Hooks()
{
   // destruct registered hook classes
   for( uint32_t i = 0; i < m_vecHooks.size(); i++ )
      delete m_vecHooks[i];
}

void
Hooks::triggerInitHook( void )
{
   if( m_vecHooks.size() == 0 ) return;

   // forward this hook to all registered hook classes
   for( uint32_t i = 0; i < m_vecHooks.size(); i++ )
      m_vecHooks[i]->initHook();
}

void
Hooks::triggerFinalizeHook( const bool & error )
{
   if( m_vecHooks.size() == 0 ) return;

   // forward this hook to all registered hook classes
   for( uint32_t i = 0; i < m_vecHooks.size(); i++ )
      m_vecHooks[i]->finalizeHook( error );
}

void
Hooks::triggerPhaseHook( const PhaseT & phase )
{
   if( m_vecHooks.size() == 0 ) return;

   // forward this hook to all registered hook classes
   for( uint32_t i = 0; i < m_vecHooks.size(); i++ )
      m_vecHooks[i]->triggerPhaseHook( phase );
}

void
Hooks::triggerReadRecordHook( const RecordT & rectype, const uint8_t & n,
                              void * a0, void * a1, void * a2, void * a3,
                              void * a4, void * a5, void * a6, void * a7,
                              void * a8, void * a9 )
{
   if( m_vecHooks.size() == 0 ) return;

   // put arguments to structure
   HooksVaArgs.set( n, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9 );

   // forward this hook to all registered hook classes
   for( uint32_t i = 0; i < m_vecHooks.size(); i++ )
      m_vecHooks[i]->triggerReadRecordHook( rectype, HooksVaArgs );
}

void
Hooks::triggerWriteRecordHook( const RecordT & rectype, const uint8_t & n,
                               void * a0, void * a1, void * a2, void * a3,
                               void * a4, void * a5, void * a6, void * a7,
                               void * a8, void * a9 )
{
   if( m_vecHooks.size() == 0 ) return;

   // put arguments to structure
   HooksVaArgs.set( n, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9 );

   // forward this hook to all registered hook classes
   for( uint32_t i = 0; i < m_vecHooks.size(); i++ )
      m_vecHooks[i]->triggerWriteRecordHook( rectype, HooksVaArgs );
}

void
Hooks::triggerSetThreadNumHook( const int & threadnum )
{
   if( m_vecHooks.size() == 0 ) return;

   for( uint32_t i = 0; i < m_vecHooks.size(); i++ )
      m_vecHooks[i]->setThreadNumHook( threadnum );
}

void
Hooks::triggerGenericHook( const uint32_t & id, const uint8_t & n,
                           void * a0, void * a1, void * a2, void * a3,
                           void * a4, void * a5, void * a6, void * a7,
                           void * a8, void * a9 )
{
   if( m_vecHooks.size() == 0 ) return;

   // put arguments to structure
   HooksVaArgs.set( n, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9 );

   // forward this hook to all registered hook classes
   for( uint32_t i = 0; i < m_vecHooks.size(); i++ )
      m_vecHooks[i]->genericHook( id, HooksVaArgs );
}
