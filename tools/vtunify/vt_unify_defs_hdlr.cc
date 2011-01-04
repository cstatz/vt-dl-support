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
#include "vt_unify_defs.h"
#include "vt_unify_hooks.h"

#include "vt_inttypes.h"

#include "otf.h"

#include <vector>

#include <stdio.h>
#include <string.h>

int
Handle_DefinitionComment( std::vector<Definitions::DefRec_Base_struct*>*
			  p_vecLocDefRecs, uint32_t streamid,
			  const char* comment )
{
   std::string _comment(comment);

   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_DefinitionComment, 2,
				    &streamid, &_comment );

   static uint32_t orderidx = 4; // 0-3 reserved for time comments
   static bool first_user = true;

   // Start-time comment(s)
   if( _comment.length() >= 14 &&
       _comment.compare( 0, 13, "__STARTTIME__" ) == 0 )
   {
      uint64_t starttime;
      sscanf(_comment.substr(14).c_str(), "%llu",
	     (unsigned long long int*)&starttime);
      if( starttime < g_uMinStartTimeEpoch )
	 g_uMinStartTimeEpoch = starttime;
   }
   // Stop-time comment(s)
   else if( _comment.length() >= 13 &&
            _comment.compare( 0, 12, "__STOPTIME__" ) == 0 )
   {
      uint64_t stoptime;
      sscanf(_comment.substr(13).c_str(), "%llu",
	     (unsigned long long int*)&stoptime);
      if( stoptime > g_uMaxStopTimeEpoch )
	 g_uMaxStopTimeEpoch = stoptime;
   }
   // VampirTrace comments
   else if( _comment.length() >= 15 &&
	    _comment.compare( 0, 14, "__VT_COMMENT__" ) == 0 )
   {
      p_vecLocDefRecs->push_back( new Definitions::DefRec_DefinitionComment_struct(
				     orderidx++,
				     _comment.substr(15) ) );
   }
   // User comments
   else
   {
      // first user comment?
      if( first_user )
      {
	 // yes -> add headline for user comments to vector of
	 // local definitions
	 //
	 p_vecLocDefRecs->push_back(
	    new Definitions::DefRec_DefinitionComment_struct(
	       100,
	       "User Comments:" ) );
	 first_user = false;
      }

      // add user comment to vector of local definitions
      p_vecLocDefRecs->push_back(
	 new Definitions::DefRec_DefinitionComment_struct(
	    100 + orderidx++,
	    (std::string(" ") + _comment ) ) );
   }

   return OTF_RETURN_OK;
}

int
Handle_DefCreator( std::vector<Definitions::DefRec_Base_struct*>*
		   p_vecLocDefRecs, uint32_t streamid, const char* creator )
{
   static bool creator_read = false;

   if( !creator_read )
   {
      std::string _creator(creator);

      // trigger read record hook
      theHooks->triggerReadRecordHook( Hooks::Record_DefCreator, 2,
				       &streamid, &_creator );

      p_vecLocDefRecs->push_back( new Definitions::DefRec_DefCreator_struct(
				     _creator ) );
      creator_read = true;
   }

   return OTF_RETURN_OK;
}

int
Handle_DefTimerResolution( std::vector<Definitions::DefRec_Base_struct*>*
			   p_vecLocDefRecs, uint32_t streamid,
			   uint64_t ticksPerSecond )
{
   static bool timerres_read = false;

   if( !timerres_read )
   {
      // trigger read record hook
      theHooks->triggerReadRecordHook( Hooks::Record_DefTimerResolution, 2,
				       &streamid, &ticksPerSecond );

      p_vecLocDefRecs->push_back( new Definitions::DefRec_DefTimerResolution_struct(
				     ticksPerSecond ) );
      timerres_read = true;
   }

   return OTF_RETURN_OK;
}

int
Handle_DefProcessGroup( std::vector<Definitions::DefRec_Base_struct*>*
			p_vecLocDefRecs, uint32_t streamid, uint32_t deftoken,
			const char* name, uint32_t n, uint32_t* array )
{
   std::string _name(name);

   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_DefProcessGroup, 5,
				    &streamid, &deftoken, &_name, &n, array );

   Definitions::DefRec_DefProcessGroup_struct::ProcessGroupTypeT type;

   if( _name.compare( 0, 8, "__NODE__" ) == 0 )
      type = Definitions::DefRec_DefProcessGroup_struct::TYPE_NODE;
   else if( _name.compare( "__MPI_COMM_USER__" ) == 0 )
      type = Definitions::DefRec_DefProcessGroup_struct::TYPE_MPI_COMM_USER;
   else if( _name.compare( "__MPI_COMM_WORLD__" ) == 0 )
      type = Definitions::DefRec_DefProcessGroup_struct::TYPE_MPI_COMM_WORLD;
   else if( _name.compare( "__MPI_COMM_SELF__" ) == 0 )
      type = Definitions::DefRec_DefProcessGroup_struct::TYPE_MPI_COMM_SELF;
   else if( _name.compare( "__OMP_TEAM__" ) == 0 )
      type = Definitions::DefRec_DefProcessGroup_struct::TYPE_OMP_TEAM;
   else if( strcmp( name, "__GPU_COMM__" ) == 0 )
     type = Definitions::DefRec_DefProcessGroup_struct::TYPE_GPU_COMM;
   else if( strcmp( name, "__GPU_GROUP__" ) == 0 )
     type = Definitions::DefRec_DefProcessGroup_struct::TYPE_GPU_GROUP;
   else
      type = Definitions::DefRec_DefProcessGroup_struct::TYPE_OTHER;

   p_vecLocDefRecs->push_back( new Definitions::DefRec_DefProcessGroup_struct(
			streamid % 65536,
			deftoken,
			type,
			_name,
			n,
			array ) );

   return OTF_RETURN_OK;
}

int
Handle_DefProcess( std::vector<Definitions::DefRec_Base_struct*>*
		   p_vecLocDefRecs, uint32_t streamid, uint32_t deftoken,
		   const char* name, uint32_t parent )
{
   std::string _name(name);

   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_DefProcess, 4,
				    &streamid, &deftoken, &_name, &parent );

   p_vecLocDefRecs->push_back( new Definitions::DefRec_DefProcess_struct(
				  deftoken,
				  _name,
				  parent ) );
   return OTF_RETURN_OK;
}

int
Handle_DefSclFile( std::vector<Definitions::DefRec_Base_struct*>*
		   p_vecLocDefRecs, uint32_t streamid, uint32_t deftoken,
		   const char* filename )
{
   std::string _filename(filename);

   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_DefSclFile, 3,
				    &streamid, &deftoken, &_filename );

   p_vecLocDefRecs->push_back( new Definitions::DefRec_DefSclFile_struct(
				  streamid % 65536,
				  deftoken,
				  _filename ) );

   return OTF_RETURN_OK;
}

int
Handle_DefScl( std::vector<Definitions::DefRec_Base_struct*>* p_vecLocDefRecs,
	       uint32_t streamid, uint32_t deftoken, uint32_t sclfile,
	       uint32_t sclline )
{
   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_DefScl, 4,
				    &streamid, &deftoken, &sclfile, &sclline );

   p_vecLocDefRecs->push_back( new Definitions::DefRec_DefScl_struct(
				  streamid % 65536,
				  deftoken,
				  sclfile,
				  sclline ) );

   return OTF_RETURN_OK;
}

int
Handle_DefFileGroup( std::vector<Definitions::DefRec_Base_struct*>*
		     p_vecLocDefRecs, uint32_t streamid, uint32_t deftoken,
		     const char* name )
{
   std::string _name(name);

   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_DefFileGroup, 3,
				    &streamid, &deftoken, &_name );

   p_vecLocDefRecs->push_back( new Definitions::DefRec_DefFileGroup_struct(
				  streamid % 65536,
				  deftoken,
				  _name ) );

   return OTF_RETURN_OK;
}

int
Handle_DefFile( std::vector<Definitions::DefRec_Base_struct*>* p_vecLocDefRecs,
		uint32_t streamid, uint32_t deftoken, const char* name,
		uint32_t group )
{
   std::string _name(name);

   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_DefFile, 4,
				    &streamid, &deftoken, &_name, &group );

   p_vecLocDefRecs->push_back( new Definitions::DefRec_DefFile_struct(
				  streamid % 65536,
				  deftoken,
				  _name,
				  group ) );

   return OTF_RETURN_OK;
}

int
Handle_DefFunctionGroup( std::vector<Definitions::DefRec_Base_struct*>*
			 p_vecLocDefRecs, uint32_t streamid, uint32_t deftoken,
			 const char* name )
{
   std::string _name(name);

   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_DefFunctionGroup, 3,
				    &streamid, &deftoken, &_name );

   p_vecLocDefRecs->push_back( new Definitions::DefRec_DefFunctionGroup_struct(
				  streamid % 65536,
				  deftoken,
				  _name ) );

   return OTF_RETURN_OK;
}

int
Handle_DefFunction( std::vector<Definitions::DefRec_Base_struct*>*
		    p_vecLocDefRecs, uint32_t streamid, uint32_t deftoken,
		    const char* name, uint32_t group, uint32_t scltoken )
{
   std::string _name(name);

   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_DefFunction, 5,
				    &streamid, &deftoken, &_name, &group,
				    &scltoken );

   p_vecLocDefRecs->push_back( new Definitions::DefRec_DefFunction_struct(
				  streamid % 65536,
				  deftoken,
				  _name,
				  group,
				  scltoken ) );

   return OTF_RETURN_OK;
}

int
Handle_DefCollectiveOperation( std::vector<Definitions::DefRec_Base_struct*>*
			       p_vecLocDefRecs, uint32_t streamid,
			       uint32_t collOp, const char* name,
			       uint32_t type )
{
   std::string _name(name);

   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_DefCollectiveOperation, 4,
				    &streamid, &collOp, &_name, &type );

   p_vecLocDefRecs->push_back( new Definitions::DefRec_DefCollectiveOperation_struct(
				  streamid % 65536,
				  collOp,
				  _name,
				  type ) );

   return OTF_RETURN_OK;
}

int
Handle_DefCounterGroup( std::vector<Definitions::DefRec_Base_struct*>*
			p_vecLocDefRecs, uint32_t streamid, uint32_t deftoken,
			const char* name )
{
   std::string _name(name);

   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_DefCounterGroup, 3,
				    &streamid, &deftoken, &_name );

   p_vecLocDefRecs->push_back( new Definitions::DefRec_DefCounterGroup_struct(
				  streamid % 65536,
				  deftoken,
				  _name ) );
   
   return OTF_RETURN_OK;
}

int
Handle_DefCounter( std::vector<Definitions::DefRec_Base_struct*>*
		   p_vecLocDefRecs, uint32_t streamid, uint32_t deftoken,
		   const char* name, uint32_t properties,
		   uint32_t countergroup, const char* unit )
{
   std::string _name(name);
   std::string _unit(unit);

   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_DefCounter, 6,
				    &streamid, &deftoken, &_name, &properties,
				    &countergroup, &unit );

   p_vecLocDefRecs->push_back( new Definitions::DefRec_DefCounter_struct(
				  streamid % 65536,
				  deftoken,
				  _name,
				  properties,
				  countergroup,
				  _unit ) );

   return OTF_RETURN_OK;
}
