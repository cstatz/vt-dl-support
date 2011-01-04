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
#include "vt_unify_stats.h"
#include "vt_unify_stats_hdlr.h"

#include "vt_inttypes.h"

#include "otf.h"

#include <iostream>
#include <string>

#include <assert.h>

Statistics * theStatistics; // instance of class Statistics

//////////////////// class Statistics ////////////////////

// public methods
//

Statistics::Statistics()
{
   // Empty
}

Statistics::~Statistics()
{
   // Empty
}

bool
Statistics::run()
{
   VPrint( 1, "Unifying statistics\n" );

   bool error = false;

   // trigger phase pre hook
   theHooks->triggerPhaseHook( Hooks::Phase_UnifyStatistics_pre );

   std::string tmp_out_file_prefix =
      Params.out_file_prefix + TmpFileSuffix;

   for( uint32_t i = 0; i < g_vecUnifyCtls.size(); i++ )
   {
      // open file manager for reader stream
      OTF_FileManager * p_org_stats_manager =
         OTF_FileManager_open( 1 );
      assert( p_org_stats_manager );

      // open stream for reading
      OTF_RStream * p_org_stats_rstream =
         OTF_RStream_open( Params.in_file_prefix.c_str(),
                           g_vecUnifyCtls[i]->streamid,
                           p_org_stats_manager );
      assert( p_org_stats_rstream );

      VPrint( 2, " Opened OTF reader stream [namestub %s id %x]\n",
              Params.in_file_prefix.c_str(),
              g_vecUnifyCtls[i]->streamid );

      if( !OTF_RStream_getStatsBuffer( p_org_stats_rstream ) )
      {
         VPrint( 2, "  No statistics found in this OTF reader stream "
                    "- Ignored\n" );
      }
      else
      {
         // close statistics buffer
         OTF_RStream_closeStatsBuffer( p_org_stats_rstream );

         // open file manager for writer stream
         OTF_FileManager * p_uni_stats_manager =
            OTF_FileManager_open( 1 );
         assert( p_uni_stats_manager );

         // open stream for writing
         OTF_WStream * p_uni_stats_wstream =
            OTF_WStream_open( tmp_out_file_prefix.c_str(),
                              g_vecUnifyCtls[i]->streamid,
                              p_uni_stats_manager );
         assert( p_uni_stats_wstream );

         VPrint( 2, " Opened OTF writer stream [namestub %s id %x]\n",
                 tmp_out_file_prefix.c_str(),
                 g_vecUnifyCtls[i]->streamid );

         // create record handler
         OTF_HandlerArray * p_handler_array =
            OTF_HandlerArray_open();
         assert( p_handler_array );

         // set record handler and first handler argument for ...
         //

         // ... OTF_FUNCTIONSUMMARY_RECORD
         OTF_HandlerArray_setHandler( p_handler_array,
            (OTF_FunctionPointer*)Handle_FunctionSummary,
                                   OTF_FUNCTIONSUMMARY_RECORD );
         OTF_HandlerArray_setFirstHandlerArg( p_handler_array,
            p_uni_stats_wstream, OTF_FUNCTIONSUMMARY_RECORD );

         // ... OTF_MESSAGESUMMARY_RECORD
         OTF_HandlerArray_setHandler( p_handler_array,
            (OTF_FunctionPointer*)Handle_MessageSummary,
                                   OTF_MESSAGESUMMARY_RECORD );
         OTF_HandlerArray_setFirstHandlerArg( p_handler_array,
            p_uni_stats_wstream, OTF_MESSAGESUMMARY_RECORD );

         // ... OTF_COLLOPSUMMARY_RECORD
         OTF_HandlerArray_setHandler( p_handler_array,
            (OTF_FunctionPointer*)Handle_CollopSummary,
                                   OTF_COLLOPSUMMARY_RECORD );
         OTF_HandlerArray_setFirstHandlerArg( p_handler_array,
            p_uni_stats_wstream, OTF_COLLOPSUMMARY_RECORD );

         // ... OTF_FILEOPERATIONSUMMARY_RECORD
         OTF_HandlerArray_setHandler( p_handler_array,
            (OTF_FunctionPointer*)Handle_FileOperationSummary,
                                   OTF_FILEOPERATIONSUMMARY_RECORD );
         OTF_HandlerArray_setFirstHandlerArg( p_handler_array,
            p_uni_stats_wstream, OTF_FILEOPERATIONSUMMARY_RECORD );


         // set file compression
         if( Params.docompress )
         {
            OTF_WStream_setCompression( p_uni_stats_wstream,
                                        OTF_FILECOMPRESSION_COMPRESSED );
         }

         // read statistics
         if( OTF_RStream_readStatistics( p_org_stats_rstream, p_handler_array )
             == OTF_READ_ERROR )
         {
            std::cerr << ExeName << ": Error: "
                      << "Could not read statistics of OTF stream [namestub "
                      << Params.in_file_prefix << " id "
                      << std::hex << g_vecUnifyCtls[i]->streamid << "]"
                      << std::dec << std::endl;
            error = true;
         }

         // close record handler
         OTF_HandlerArray_close( p_handler_array );
         // close writer stream
         OTF_WStream_close( p_uni_stats_wstream );
         // close file manager for writer stream
         OTF_FileManager_close( p_uni_stats_manager );

         VPrint( 2, " Closed OTF writer stream [namestub %s id %x]\n",
                 tmp_out_file_prefix.c_str(),
                 g_vecUnifyCtls[i]->streamid );
      }

      // close reader stream
      OTF_RStream_close( p_org_stats_rstream );
      // close file manager for reader stream
      OTF_FileManager_close( p_org_stats_manager );

      VPrint( 2, " Closed OTF reader stream [namestub %s id %x]\n",
              Params.in_file_prefix.c_str(),
              g_vecUnifyCtls[i]->streamid );

      if( error ) break;
   }

   if( error )
   {
      std::cerr << ExeName << ": "
                << "An error occurred during unifying statistics. Aborting"
                << std::endl;
   }
   else
   {
      // trigger phase post hook
      theHooks->triggerPhaseHook( Hooks::Phase_UnifyStatistics_post );
   }

   return !error;
}
