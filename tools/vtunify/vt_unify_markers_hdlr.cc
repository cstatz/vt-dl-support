/**
 * VampirTrace
 * http://www.tu-dresden.de/zih/vampirtrace
 *
 * Copyright (c) 2005-2010, ZIH, TU Dresden, Federal Republic of Germany
 *
 * Copyright (c) 1998-2005, Forschungszentrum Juelich GmbH, Federal
 * Republic of Germany
 *
 * See the file COPYRIGHT in the package base directory for details
 **/

#include "vt_unify.h"
#include "vt_unify_hooks.h"
#include "vt_unify_markers.h"
#include "vt_unify_tkfac.h"

#include "vt_inttypes.h"

#include "otf.h"

#include <string>
#include <vector>

#include <assert.h>

int
Handle_DefMarker( std::vector<Markers::DefRec_Marker_struct*>*
		  p_vecMarkerDefRecs,
		  uint32_t streamid, uint32_t deftoken, const char* name,
		  uint32_t type )
{
   std::string _name(name);

   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_DefMarker, 4,
				    &streamid, &deftoken, &_name, &type );

   uint32_t mcpuid = streamid % 65536;

   // get global token factory for DefMarker
   TokenFactory_DefMarker * p_tkfac_defmarker =
      static_cast<TokenFactory_DefMarker*>(theTokenFactory[TKFAC__DEF_MARKER]);

   // get global token
   uint32_t global_token =
      p_tkfac_defmarker->getGlobalToken( _name, type );

   // global token found ?
   if( global_token == 0 )
   {
      // no -> create it
      global_token =
	 p_tkfac_defmarker->createGlobalToken(
	    mcpuid,
	    deftoken, _name, type );

      // add new definition to vector of global definitions
      p_vecMarkerDefRecs->push_back( new Markers::DefRec_Marker_struct(
					0, global_token, _name, type ) );
   }
   else
   {
      // yes -> (global definition already exists in vector)

      // set translation for this local process id, if necessary
      //
      if( p_tkfac_defmarker->translateLocalToken( mcpuid, deftoken ) == 0 )
	 p_tkfac_defmarker->setTranslation( mcpuid, deftoken, global_token );
   }

   return OTF_RETURN_OK;
}

int Handle_Marker( OTF_WStream* wstream,
		   uint64_t time, uint32_t process, uint32_t token,
		   const char* text )
{
   std::string _text(text);

   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_Marker, 4,
				    &time, &process, &token, &_text );

   uint32_t mprocess = process % 65536;

   TokenFactory_DefMarker * p_tkfac_defmarker =
      static_cast<TokenFactory_DefMarker*>(theTokenFactory[TKFAC__DEF_MARKER]);

   uint32_t global_token =
      p_tkfac_defmarker->translateLocalToken( mprocess, token );
   assert( global_token != 0 );

   time = CorrectTime( mprocess, time );

   // trigger write record hook
   theHooks->triggerWriteRecordHook( Hooks::Record_Marker, 4,
				     &time, &process, &global_token, &_text );

   int wrrc = OTF_WStream_writeMarker( wstream,
				       time, process,
				       global_token,
				       _text.c_str() );
   return wrrc == 1 ? OTF_RETURN_OK : OTF_RETURN_ABORT;
}
