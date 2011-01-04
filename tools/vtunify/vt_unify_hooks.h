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

#ifndef _VT_UNIFY_HOOKS_H_
#define _VT_UNIFY_HOOKS_H_

#include "vt_inttypes.h"

#include <vector>

#include <assert.h>

//
// variable arguments structure
//
struct HooksVaArgs_struct
{
   // maximum number of arguments
   static const int max = 10;

   // return number of currently stored arguments
   inline uint8_t num( void ) const
   {
      return argn;
   }

   // set arguments
   inline void set( const uint8_t & n,
                    void * a0, void * a1, void * a2, void * a3, void * a4,
                    void * a5, void * a6, void * a7, void * a8, void * a9 )
   {
      assert( n <= max );

      argn = n;
      argv[0] = a0; argv[1] = a1; argv[2] = a2; argv[3] = a3; argv[4] = a4;
      argv[5] = a5; argv[6] = a6; argv[7] = a7; argv[8] = a8; argv[9] = a9;
   }

   // get argument by index
   inline void *& get( const uint8_t & idx )
   {
      assert( idx < max );

      return argv[idx];
   }

   // set/get argument by access operator
   inline void *& operator[]( const uint8_t & idx )
   {
      return get( idx );
   }

   uint8_t argn;
   void * argv[max];
};

//
// Hooks class
//

class HooksBase; // forward declaration

class Hooks
{
public:

   // phase hooks
   //
   typedef enum {
      Phase_GetParams_pre, Phase_GetParams_post,
      Phase_GetUnifyControls_pre, Phase_GetUnifyControls_post,
      Phase_GetMinStartTime_pre, Phase_GetMinStartTime_post,
      Phase_UnifyDefinitions_pre, Phase_UnifyDefinitions_post,
      Phase_UnifyMarkers_pre, Phase_UnifyMarkers_post,
      Phase_UnifyStatistics_pre, Phase_UnifyStatistics_post,
      Phase_UnifyEvents_pre, Phase_UnifyEvents_post,
      Phase_WriteMasterControl_pre, Phase_WriteMasterControl_post,
      Phase_CleanUp_pre, Phase_CleanUp_post,
      Phase_Num
   } PhaseT;

   // record hooks
   //
   typedef enum {
      // definition records
      //
      Record_DefinitionComment,
      Record_DefCreator,
      Record_DefTimerResolution,
      Record_DefProcessGroup,
      Record_DefProcess,
      Record_DefSclFile,
      Record_DefScl,
      Record_DefFileGroup,
      Record_DefFile,
      Record_DefFunctionGroup,
      Record_DefFunction,
      Record_DefCollectiveOperation,
      Record_DefCounterGroup,
      Record_DefCounter,

      // summary records
      //
      Record_FunctionSummary,
      Record_MessageSummary,
      Record_CollopSummary,
      Record_FileOperationSummary,

      // marker records
      //
      Record_DefMarker,
      Record_Marker,

      // event records
      //
      Record_Enter,
      Record_Leave,
      Record_FileOperation,
      Record_BeginFileOperation,
      Record_EndFileOperation,
      Record_SendMsg,
      Record_RecvMsg,
      Record_CollectiveOperation,
      Record_BeginCollectiveOperation,
      Record_EndCollectiveOperation,
      Record_RMAPut,
      Record_RMAPutRemoteEnd,
      Record_RMAGet,
      Record_RMAEnd,
      Record_Counter,
      Record_EventComment,

      // number of records
      Record_Num
   } RecordT;

   // contructor
   Hooks();

   // destructor
   ~Hooks();

   void triggerInitHook( void );

   void triggerFinalizeHook( const bool & error );

   void triggerPhaseHook( const PhaseT & phase );

   void triggerReadRecordHook( const RecordT & rectype, const uint8_t & n,
                               void * a0 = 0, void * a1 = 0, void * a2 = 0,
                               void * a3 = 0, void * a4 = 0, void * a5 = 0,
                               void * a6 = 0, void * a7 = 0, void * a8 = 0,
                               void * a9 = 0 );

   void triggerWriteRecordHook( const RecordT & rectype, const uint8_t & n,
                                void * a0 = 0, void * a1 = 0, void * a2 = 0,
                                void * a3 = 0, void * a4 = 0, void * a5 = 0,
                                void * a6 = 0, void * a7 = 0, void * a8 = 0,
                                void * a9 = 0 );

   void triggerSetThreadNumHook( const int & threadnum );

   void triggerGenericHook( const uint32_t & id, const uint8_t & n,
                            void * a0 = 0, void * a1 = 0, void * a2 = 0,
                            void * a3 = 0, void * a4 = 0, void * a5 = 0,
                            void * a6 = 0, void * a7 = 0, void * a8 = 0,
                            void * a9 = 0 );

private:

  std::vector<HooksBase*> m_vecHooks;

};

// storage for variable hook arguments
extern HooksVaArgs_struct HooksVaArgs;

// instance of class Hooks
extern Hooks * theHooks;

#endif // _VT_UNIFY_HOOKS_H_
