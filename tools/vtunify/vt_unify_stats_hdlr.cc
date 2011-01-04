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
#include "vt_unify_tkfac.h"

#include "vt_inttypes.h"

#include "otf.h"

#include <assert.h>

int
Handle_FunctionSummary( OTF_WStream* wstream, 
			uint64_t time, uint32_t function, uint32_t process,
			uint64_t invocations, uint64_t exclTime,
			uint64_t inclTime )
{
   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_FunctionSummary, 6,
				    &time, &function, &process, &invocations,
				    &exclTime, &inclTime );

   uint32_t mprocess = process % 65536;

   TokenFactory_DefFunction * p_tkfac_deffunction =
      static_cast<TokenFactory_DefFunction*>(theTokenFactory[TKFAC__DEF_FUNCTION]);

   uint32_t global_function =
      p_tkfac_deffunction->translateLocalToken( mprocess, function );
   assert( global_function != 0 );

   time = CorrectTime( mprocess, time );

   // trigger write record hook
   theHooks->triggerWriteRecordHook( Hooks::Record_FunctionSummary, 6,
				     &time, &global_function, &process,
				     &invocations, &exclTime, &inclTime );

   int wrrc = OTF_WStream_writeFunctionSummary( wstream,
						time, global_function,
						process, invocations,
						exclTime, inclTime );
   return wrrc == 1 ? OTF_RETURN_OK : OTF_RETURN_ABORT;
}

int
Handle_MessageSummary( OTF_WStream* wstream,
		       uint64_t time, uint32_t process, uint32_t peer,
		       uint32_t comm, uint32_t type, uint64_t sentNumber,
		       uint64_t receivedNumber, uint64_t sentBytes,
		       uint64_t receivedBytes )
{
   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_MessageSummary, 9,
				    &time, &process, &peer, &comm, &type,
				    &sentNumber, &receivedNumber, &sentBytes,
				    &receivedBytes );

   uint32_t mprocess = process % 65536;

   TokenFactory_DefProcessGroup * p_tkfac_defprocessgroup =
      static_cast<TokenFactory_DefProcessGroup*>(theTokenFactory[TKFAC__DEF_PROCESS_GROUP]);

   uint32_t global_comm = comm;

   if( comm != 0 )
   {
      global_comm =
	 p_tkfac_defprocessgroup->translateLocalToken( mprocess, comm );
      assert( global_comm != 0 );
   }

   time = CorrectTime( mprocess, time );

   // trigger write record hook
   theHooks->triggerWriteRecordHook( Hooks::Record_MessageSummary, 8,
				     &time, &process, &peer, &global_comm,
				     &sentNumber, &receivedNumber, &sentBytes,
				     &receivedBytes );

   int wrrc = OTF_WStream_writeMessageSummary( wstream,
					       time, process, peer,
					       global_comm,
					       type, sentNumber,
					       receivedNumber, sentBytes,
					       receivedBytes );

   return wrrc == 1 ? OTF_RETURN_OK : OTF_RETURN_ABORT;
}

int
Handle_CollopSummary( OTF_WStream* wstream, 
		      uint64_t time, uint32_t process, uint32_t comm,
		      uint32_t collective, uint64_t sentNumber,
		      uint64_t receivedNumber, uint64_t sentBytes,
		      uint64_t receivedBytes )
{
   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_CollopSummary, 8,
				    &time, &process, &comm, &collective,
				    &sentNumber, &receivedNumber, &sentBytes,
				    &receivedBytes );

   uint32_t mprocess = process % 65536;

   TokenFactory_DefCollectiveOperation * p_tkfac_defcollop =
      static_cast<TokenFactory_DefCollectiveOperation*>(theTokenFactory[TKFAC__DEF_COLL_OP]);

   TokenFactory_DefProcessGroup * p_tkfac_defprocessgroup =
      static_cast<TokenFactory_DefProcessGroup*>(theTokenFactory[TKFAC__DEF_PROCESS_GROUP]);

   uint32_t global_collective = collective;

   if( collective != 0 )
   {
      global_collective =
	 p_tkfac_defcollop->translateLocalToken( mprocess, collective );
      assert( global_collective != 0 );
   }

   uint32_t global_comm = comm;

   if( comm != 0 )
   {
      global_comm =
	 p_tkfac_defprocessgroup->translateLocalToken( mprocess, comm );
      assert( global_comm != 0 );
   }

   time = CorrectTime( mprocess, time );

   // trigger write record hook
   theHooks->triggerWriteRecordHook( Hooks::Record_CollopSummary, 8,
				     &time, &process, &global_comm,
				     &global_collective, &sentNumber,
				     &receivedNumber, &sentBytes,
				     &receivedBytes );

   int wrrc = OTF_WStream_writeCollopSummary( wstream,
					      time, process,
					      global_comm,
					      global_collective,
					      sentNumber, receivedNumber,
					      sentBytes, receivedBytes );

   return wrrc == 1 ? OTF_RETURN_OK : OTF_RETURN_ABORT;
}

int
Handle_FileOperationSummary( OTF_WStream* wstream,
			     uint64_t time, uint32_t fileid, uint32_t process,
			     uint64_t nopen, uint64_t nclose, uint64_t nread,
			     uint64_t nwrite, uint64_t nseek,
			     uint64_t bytesread, uint64_t byteswrite )
{
   // trigger read record hook
   theHooks->triggerReadRecordHook( Hooks::Record_FileOperationSummary, 10,
				    &time, &fileid, &process, &nopen, &nclose,
				    &nread, &nwrite, &nseek, &bytesread,
				    &byteswrite );

   uint32_t mprocess = process % 65536;

   TokenFactory_DefFile * p_tkfac_deffile =
      static_cast<TokenFactory_DefFile*>(theTokenFactory[TKFAC__DEF_FILE]);

   uint32_t global_fileid =
      p_tkfac_deffile->translateLocalToken( mprocess, fileid );
   assert( global_fileid != 0 );

   time = CorrectTime( mprocess, time );

   // trigger write record hook
   theHooks->triggerWriteRecordHook( Hooks::Record_FileOperationSummary, 10,
				     &time, &global_fileid, &process, &nopen,
				     &nclose, &nread, &nwrite, &nseek,
				     &bytesread, &byteswrite );

   int wrrc = OTF_WStream_writeFileOperationSummary( wstream,
						     time, global_fileid,
						     process, nopen,
						     nclose, nread,
						     nwrite, nseek,
						     bytesread, byteswrite );

   return wrrc == 1 ? OTF_RETURN_OK : OTF_RETURN_ABORT;
}
