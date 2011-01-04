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

#include "vt_unify_hooks_tdb.h"

#include <iostream>
#include <fstream>

#include <sstream>

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef _SX
    #include <sys/socket.h> // needed on NEC SX platforms for gethostname()
#endif // _SX

#ifndef HOST_NAME_MAX
    #define HOST_NAME_MAX 256
#endif // HOST_NAME_MAX

#define GETPARAM(type, name, i) type name = (type) args[i];

#if defined(HAVE_OMP) && HAVE_OMP
    #define GET_THREAD_ID( name ) int name = omp_get_thread_num();
#else
    #define GET_THREAD_ID( name ) int name = 0;
#endif // HAVE_OMP


/* collop_class */

collop_class::collop_class():
    num(0), bytes_sent(0), bytes_recv(0) {}

collop_class::collop_class( uint64_t _num, uint64_t _bytes_sent, uint64_t _bytes_recv ):
    num(_num), bytes_sent(_bytes_sent), bytes_recv(_bytes_recv) {}

collop_class collop_class::operator+=(collop_class cs) {

    num += cs.num;
    bytes_sent += cs.bytes_sent;
    bytes_recv += cs.bytes_recv;

    return *this;

}

/* io_class */

io_class::io_class():
    num_read(0), bytes_read(0), num_written(0),
    bytes_written(0), num_open(0), num_close(0),
    num_seek(0) { }

io_class io_class::operator+=(io_class is) {

    num_read += is.num_read;
    bytes_read += is.bytes_read;
    num_written += is.num_written;
    bytes_written += is.bytes_written;
    num_open += is.num_open;
    num_close += is.num_close;
    num_seek += is.num_seek;

    return *this;

}

/* master_class */

MasterData::MasterData() {

    num_processes = 0;
    num_active_processes = 0;
    num_threads = 0;

    vt_flush_id = 0;

    create_output = false;

}

MasterData::~MasterData() {

}

uint64_t MasterData::calc_filesize() {

    struct stat buf;
    std::string fname;
    std::string suffix[5] = {".events", ".def", ".stats", ".snaps", ".marker"};
    std::map<uint32_t, uint32_t>::const_iterator cit;

    filesize = 0;

    fname = filename + ".otf";

    if( access( fname.c_str(), F_OK ) == 0 ) {

        stat( fname.c_str(), &buf );
        filesize += buf.st_size;

    }

    for( int i = 0; i < 5; i++ ) {

        fname = filename + ".";
        fname += intToHex( 0 ) + suffix[i];

        if( access( fname.c_str(), F_OK ) == 0 ) {

            stat( fname.c_str(), &buf );
            filesize += buf.st_size;

        }

    }

    for( cit = g_mapStreamIdUnifyCtlIdx.begin(); cit != g_mapStreamIdUnifyCtlIdx.end(); ++cit ) {

        for( int i = 0; i < 5; i++ ) {

            fname = filename + ".";
            fname += intToHex( cit->first ) + suffix[i];

            if( access( fname.c_str(), F_OK ) == 0 ) {

                stat( fname.c_str(), &buf );
                filesize += buf.st_size;

            }

        }

    }

    return  filesize;
 }

std::string MasterData::intToHex( int i ) {

    std::ostringstream oss;
    oss << std::hex << i;

    return oss.str();

}

std::string MasterData::safe_getcwd() {

    int max_path = 4096;
    char *buf = new char[max_path];

    while( 1 ) {

      if( ! getcwd( buf, max_path ) ) {

        if( errno == ERANGE ) {

            /* buf is too small */

            /* resize buf and try again */
            max_path += 1024;

            delete [] buf;
            buf = new char[max_path];

        } else {

            /* an error occurred */
            return NULL;

        }

      } else {

          /* all right */
          return std::string( buf );

      }

    }

}

int MasterData::set_filepath( std::string file_prefix ) {

    std::string current_dir;
    std::string target_dir = ".";
    std::string absolute_dir;
    std::string fname = file_prefix;

    size_t last_slash = 0;

    last_slash = file_prefix.find_last_of( '/' );

    if( last_slash != std::string::npos ) {

        target_dir = file_prefix.substr( 0, last_slash );
        fname = file_prefix.substr( last_slash + 1 );

    }

    current_dir = safe_getcwd();

    if( chdir( target_dir.c_str() ) )

        return 1;

    absolute_dir = safe_getcwd();

    if( chdir( current_dir.c_str() ) )
        return 1;


    filepath = absolute_dir;
    filename = fname;


    return 0;

}

int MasterData::set_hostname() {

    char name[HOST_NAME_MAX];

    if( gethostname( name, HOST_NAME_MAX ) ) {

        return 1;

    }

    hostname = std::string( name );


    return 0;

}

void MasterData::set_otf_version() {

    otf_version = OTF_VERSION_MAJOR + 48;
    otf_version += ".";
    otf_version += OTF_VERSION_MINOR + 48;
    otf_version += ".";
    otf_version += OTF_VERSION_SUB + 48;
    otf_version += " \"";
    otf_version += OTF_VERSION_STRING;
    otf_version += "\"";

}


/* thread_class */

ThreadData::ThreadData():
    num_events(0), num_enter(0), num_leave(0), num_sent(0),
    num_recv(0), bytes_sent(0), bytes_recv(0), num_rma(0),
    bytes_rma(0), num_marker(0), num_stats(0), num_snaps(0),
    num_vt_flushes(0) {}

ThreadData ThreadData::operator+=(ThreadData td) {

    std::map<uint32_t, collop_class>::iterator collop_it;
    std::map<uint32_t, io_class>::iterator io_it;

    num_events += td.num_events;
    num_enter += td.num_enter;
    num_leave += td.num_leave;
    num_sent += td.num_sent;
    num_recv += td.num_recv;
    bytes_sent += td.bytes_sent;
    bytes_recv += td.bytes_recv;
    num_rma += td.num_rma;
    bytes_rma += bytes_rma;
    num_marker += td.num_marker;
    num_stats += td.num_stats;
    num_snaps += td.num_snaps;
    num_vt_flushes += td.num_vt_flushes;

    for( collop_it = td.collop.begin(); collop_it != td.collop.end(); ++collop_it ) {

        collop[collop_it->first] += collop_it->second;

    }

    for( io_it = td.io.begin(); io_it != td.io.end(); ++io_it ) {

        io[io_it->first] += io_it->second;

    }

    return *this;

}

bool ThreadData::toBuffer( uint64_t *buf ) {

    int offset = 0;

    std::map<uint32_t, collop_class>::const_iterator collop_it;
    std::map<uint32_t, io_class>::const_iterator io_it;

    buf[offset++] = num_enter;
    buf[offset++] = num_leave;
    buf[offset++] = num_sent;
    buf[offset++] = num_recv;
    buf[offset++] = bytes_sent;
    buf[offset++] = bytes_recv;
    buf[offset++] = num_events;
    buf[offset++] = num_rma;
    buf[offset++] = bytes_rma;
    buf[offset++] = num_marker;
    buf[offset++] = num_stats;
    buf[offset++] = num_snaps;
    buf[offset++] = num_vt_flushes;

    for( collop_it = collop.begin(); collop_it != collop.end(); ++collop_it ) {

        buf[ offset++ ] = (uint64_t) collop_it->first;
        buf[ offset++ ] = collop_it->second.num;
        buf[ offset++ ] = collop_it->second.bytes_sent;
        buf[ offset++ ] = collop_it->second.bytes_recv;

    }

    for( io_it = io.begin(); io_it != io.end(); ++io_it ) {

        buf[ offset++ ] = (uint64_t) io_it->first;
        buf[ offset++ ] = io_it->second.num_read;
        buf[ offset++ ] = io_it->second.bytes_read;
        buf[ offset++ ] = io_it->second.num_written;
        buf[ offset++ ] = io_it->second.bytes_written;
        buf[ offset++ ] = io_it->second.num_open;
        buf[ offset++ ] = io_it->second.num_close;
        buf[ offset++ ] = io_it->second.num_seek;

    }

    return true;

}

ThreadData ThreadData::fromBuffer( const uint64_t *buf, const uint64_t *num_bytes ) {

    uint64_t offset = 0;
    int pos;

    uint64_t max_bytes = num_bytes[0];

    num_enter = buf[ offset++ ];
    num_leave = buf[ offset++ ];
    num_sent = buf[ offset++ ];
    num_recv = buf[ offset++ ];
    bytes_sent = buf[ offset++ ];
    bytes_recv = buf[ offset++ ];
    num_events = buf[ offset++ ];
    num_rma = buf[ offset++ ];
    bytes_rma = buf[ offset++ ];
    num_marker = buf[ offset++ ];
    num_stats = buf[ offset++ ];
    num_snaps = buf[ offset++ ];
    num_vt_flushes = buf[ offset++ ];

    max_bytes += num_bytes[1];

    while( offset < max_bytes ) {

        pos = offset++;

        collop[ buf[ pos ] ].num = buf[ offset++ ];
        collop[ buf[ pos ] ].bytes_sent = buf[ offset++ ];
        collop[ buf[ pos ] ].bytes_recv = buf[ offset++ ];

    }

    max_bytes += num_bytes[2];

    while( offset < max_bytes ) {

        pos = offset++;

        io[ buf[ pos ] ].num_read = buf[ offset++ ];
        io[ buf[ pos ] ].bytes_read = buf[ offset++ ];
        io[ buf[ pos ] ].num_written = buf[ offset++ ];
        io[ buf[ pos ] ].bytes_written = buf[ offset++ ];
        io[ buf[ pos ] ].num_open = buf[ offset++ ];
        io[ buf[ pos ] ].num_close = buf[ offset++ ];
        io[ buf[ pos ] ].num_seek = buf[ offset++ ];

    }

    return *this;

}

//////////////////// class HooksTdb ////////////////////

// public methods
//

HooksTdb::HooksTdb() : HooksBase()
{
    // Empty
}

HooksTdb::~HooksTdb()
{
    // Empty
}

bool HooksTdb::isEnabled()
{
   bool enabled = false;

   char* createtdb_env = getenv( "VT_UNIFY_CREATE_TDB" );
   if( createtdb_env )
   {
      std::string tmp( createtdb_env );
      for( uint32_t i = 0; i < tmp.length(); i++ )
         tmp[i] = tolower( tmp[i] );

      if( tmp.compare( "yes" ) == 0 ||
          tmp.compare( "true" ) == 0 ||
          tmp.compare( "1" ) == 0 )
      {
         enabled = true;
      }
   }

   return enabled;
}

// private methods
//

// initialization/finalization hooks
//

void HooksTdb::initHook( void )
{

   /* create at least one element for the root process */
   ThreadVector.resize( 1 );

}

void HooksTdb::finalizeHook( const bool & error )
{

    if( error ) {

        std::cout << " ERROR IN HooksTdb::finalizeHook ABORT." << std::endl;
        return;

    }

    if( !RootData.create_output ) {

        /* do not output anything */
        /* --> this normally happens if
           vtunify prints the helptext */

        return;

    }

    std::map<uint32_t, collop_class>::iterator collop_it;

    /* collect data from other threads in master thread #0 */
    for( uint32_t i = 1; i < ThreadVector.size(); i++ ) {

           ThreadVector[0] += ThreadVector[i];

    }

#ifdef VT_MPI

#define NUM_SEND_PARTS 3

    uint64_t *recv_buffer = NULL;

    VT_MPI_INT *recv_count = NULL;
    uint64_t sum_recv_count = 0;
    uint64_t sum_sent_count = 0;
    VT_MPI_INT *displ = NULL;

    uint64_t *send_buffer = new uint64_t[NUM_SEND_PARTS];

    /* this is the number of properties in the ThreadData class */
    int num_attributes = 13;

    int collop_map_size = ThreadVector[0].collop.size() * 4;
    int io_map_size = ThreadVector[0].io.size() * 8;

    send_buffer[0] = (uint64_t) num_attributes;
    send_buffer[1] = (uint64_t) collop_map_size;
    send_buffer[2] = (uint64_t) io_map_size;

    for( int i = 0; i < NUM_SEND_PARTS; i++ ) {

        sum_sent_count += send_buffer[i];

    }

    if( g_iMPIRank != 0 ) {

        /* first gatherv */
        CALL_MPI( MPI_Gather( send_buffer, NUM_SEND_PARTS, MPI_LONG_LONG_INT,
                              recv_buffer, 0, MPI_LONG_LONG_INT, 0,
                              MPI_COMM_WORLD ) );

    } else {

        recv_count = new VT_MPI_INT[ g_iMPISize ];
        displ = new VT_MPI_INT[ g_iMPISize ];

        recv_buffer = new uint64_t[ g_iMPISize * NUM_SEND_PARTS ];

        CALL_MPI( MPI_Gather( send_buffer, NUM_SEND_PARTS, MPI_LONG_LONG_INT,
                              recv_buffer, NUM_SEND_PARTS, MPI_LONG_LONG_INT,
                              0,  MPI_COMM_WORLD ) );


        displ[0] = 0;
        sum_recv_count = 0;

        for( VT_MPI_INT j = 0; j < g_iMPISize; j++ ) {

            recv_count[j] = 0;

            for( int i = 0; i < NUM_SEND_PARTS; i++ ) {

                recv_count[j] += recv_buffer[j * NUM_SEND_PARTS + i];

            }

            displ[j] = sum_recv_count;
            sum_recv_count += recv_count[j];

        }

    }

    /* reuse send_buffer */
    delete [] send_buffer;

    send_buffer = new uint64_t[ sum_sent_count ];

    ThreadVector[0].toBuffer( send_buffer );

    if( g_iMPIRank != 0 ) {

        CALL_MPI( MPI_Gatherv( send_buffer, sum_sent_count, MPI_LONG_LONG_INT,
                               recv_buffer, recv_count, displ, MPI_LONG_LONG_INT,
                               0, MPI_COMM_WORLD ) );

    } else {

        uint64_t *recv_data_buffer = new uint64_t[ sum_recv_count ];
        uint64_t *p = recv_data_buffer;

        CALL_MPI( MPI_Gatherv( send_buffer, sum_sent_count, MPI_LONG_LONG_INT,
                               recv_data_buffer, recv_count, displ,
                               MPI_LONG_LONG_INT, 0, MPI_COMM_WORLD ) );

        ThreadData tmp_vector;

        p += recv_count[0];

        for( VT_MPI_INT j = 1; j < g_iMPISize; j++ ) {

            tmp_vector.fromBuffer( p, &(recv_buffer[ j * NUM_SEND_PARTS ]) );

            ThreadVector[0] += tmp_vector;

            p += recv_count[j];

        }

        uint32_t collop_type;

        for( collop_it = ThreadVector[0].collop.begin(); collop_it != ThreadVector[0].collop.end(); ++collop_it ) {

            collop_type = RootData.collop_def[ collop_it->first ];

            RootData.collop[collop_type].num += collop_it->second.num;
            RootData.collop[collop_type].bytes_sent += collop_it->second.bytes_sent;
            RootData.collop[collop_type].bytes_recv += collop_it->second.bytes_recv;

        }

    }


    /* OUTPUT */


    if( g_iMPIRank == 0 ) {

#endif /*VT_MPI*/

        RootData.set_filepath( Params.out_file_prefix );
        RootData.set_hostname();
        RootData.set_otf_version();

        /* assume that vtunify creates one stream per process */
        RootData.num_processes = RootData.num_streams = g_vecUnifyCtls.size();
        RootData.is_compressed = Params.docompress;
        RootData.starttime = g_uMinStartTimeEpoch / 1000000;
        RootData.runtime =  ( g_uMaxStopTimeEpoch - g_uMinStartTimeEpoch );

        RootData.calc_filesize();

        std::string tdb_file = RootData.filename + ".tdb";
        std::ofstream outfile( tdb_file.c_str() );

        outfile << "filename;" << RootData.filename << std::endl;
        outfile << "filepath;" << RootData.filepath << std::endl;
        outfile << "hostname;" << RootData.hostname << std::endl;
        outfile << "otf_version;" << RootData.otf_version << std::endl;
        outfile << "creator;" << RootData.creator << std::endl;
        outfile << "num_processes;" << RootData.num_processes << std::endl;
        outfile << "num_active_processes;" << RootData.num_active_processes << std::endl;
        outfile << "num_streams;" << RootData.num_streams << std::endl;
        outfile << "num_threads;" << RootData.num_threads << std::endl;
        outfile << "starttime;" << RootData.starttime << std::endl;
        outfile << "runtime;" << RootData.runtime << std::endl;
        outfile << "filesize;" << RootData.filesize << std::endl;
        outfile << "compression;" << RootData.is_compressed << std::endl;

        outfile << "num_events; " << ThreadVector[0].num_events << std::endl;
        outfile << "num_enter; " << ThreadVector[0].num_enter << std::endl;
        outfile << "num_leave; " << ThreadVector[0].num_leave << std::endl;
        outfile << "num_sent; " << ThreadVector[0].num_sent << std::endl;
        outfile << "num_recv; " << ThreadVector[0].num_recv << std::endl;
        outfile << "bytes_sent; " << ThreadVector[0].bytes_sent << std::endl;
        outfile << "bytes_recv; " << ThreadVector[0].bytes_recv << std::endl;
        outfile << "num_rma; " << ThreadVector[0].num_rma << std::endl;
        outfile << "bytes_rma; " << ThreadVector[0].bytes_rma << std::endl;

        outfile << "num_marker; " << ThreadVector[0].num_marker << std::endl;
        outfile << "num_stats; " << ThreadVector[0].num_stats << std::endl;
        outfile << "num_snaps; " << ThreadVector[0].num_snaps << std::endl;
        outfile << "num_vt_flushes; " << ThreadVector[0].num_vt_flushes << std::endl;


        std::map<uint32_t, std::string>::const_iterator fgroup_it;

        for( fgroup_it = RootData.fgroup.begin(); fgroup_it != RootData.fgroup.end(); ++fgroup_it ) {

            outfile << "function_group;" << fgroup_it->second << std::endl;

        }


        for( uint32_t i = 0; i < RootData.counter.size(); i++ ) {

            outfile << "counter;" << RootData.counter[i] << std::endl;

        }


        std::map<std::string, std::string>::const_iterator vt_cit;

        for( vt_cit = RootData.vt_env.begin(); vt_cit != RootData.vt_env.end(); ++vt_cit ) {

            outfile << "vt_env_vars;" << vt_cit->first << ";" << vt_cit->second << std::endl;

        }

        std::map<uint32_t,std::string> col_id_to_name;

        col_id_to_name[ OTF_COLLECTIVE_TYPE_UNKNOWN ] = "Unknown";
        col_id_to_name[ OTF_COLLECTIVE_TYPE_BARRIER ] = "Barrier";
        col_id_to_name[ OTF_COLLECTIVE_TYPE_ONE2ALL ] = "One2All";
        col_id_to_name[ OTF_COLLECTIVE_TYPE_ALL2ONE ] = "All2One";
        col_id_to_name[ OTF_COLLECTIVE_TYPE_ALL2ALL ] = "All2All";

        for( collop_it = RootData.collop.begin(); collop_it != RootData.collop.end(); ++collop_it ) {

            outfile << "collops;" << col_id_to_name[ collop_it->first ] << ";" << collop_it->second.num
            << ";" << collop_it->second.bytes_sent << ";" << collop_it->second.bytes_recv << std::endl;

        }

        std::map<uint32_t,std::string> ioflag_to_name;

        ioflag_to_name[ 0 ] = "all";
        ioflag_to_name[ 1 ] = "normal";
        ioflag_to_name[ OTF_IOFLAG_IOFAILED ] = "failed";
        ioflag_to_name[ OTF_IOFLAG_ASYNC ] = "async";
        ioflag_to_name[ OTF_IOFLAG_COLL ] = "coll";
        ioflag_to_name[ OTF_IOFLAG_DIRECT ] = "direct";
        ioflag_to_name[ OTF_IOFLAG_SYNC ] = "sync";
        ioflag_to_name[ OTF_IOFLAG_ISREADLOCK ] = "readlock";

        std::map<uint32_t, io_class>::iterator io_it;

        for( io_it = ThreadVector[0].io.begin(); io_it != ThreadVector[0].io.end(); ++io_it ) {

            outfile << "io;"
                    << ioflag_to_name[ io_it->first ] << ";"
                    << io_it->second.num_read << ";"
                    << io_it->second.bytes_read << ";"
                    << io_it->second.num_written << ";"
                    << io_it->second.bytes_written << ";"
                    << io_it->second.num_open << ";"
                    << io_it->second.num_close << ";"
                    << io_it->second.num_seek << std::endl;

        }

        outfile.close();

#ifdef VT_MPI
    }
#endif


}


// :TODO:
// @Johannes: decomment needed hook methods below

// phase hooks
//

// void HooksTdb::phaseHook_GetParams_pre() {  }
// void HooksTdb::phaseHook_GetParams_post() {  }
//
// void HooksTdb::phaseHook_GetUnifyControls_pre() {  }
// void HooksTdb::phaseHook_GetUnifyControls_post() {  }
//
// void HooksTdb::phaseHook_GetMinStartTime_pre() {  }
// void HooksTdb::phaseHook_GetMinStartTime_post() {  }
//


// void HooksTdb::phaseHook_UnifyDefinitions_pre() {  }


void HooksTdb::phaseHook_UnifyDefinitions_post() {

    /* thread-safe: this and all definition records are always called by thread 0 on rank 0 */

    std::map<uint32_t, uint32_t>::const_iterator flush_it;

    for( flush_it = RootData.flush_fgroup_ids.begin(); flush_it != RootData.flush_fgroup_ids.end(); ++flush_it ) {

        if( RootData.fgroup[ flush_it->second ] == "VT_API" ) {

            /* this is necessary because of a bug in VT
               --> a user-defined function named "flush" is
               assigned to group "VT_API" too */
            if( RootData.vt_flush_id == 0 )
                RootData.vt_flush_id = flush_it->first;

        }

    }

}


//
// void HooksTdb::phaseHook_UnifyMarkers_pre() {  }
// void HooksTdb::phaseHook_UnifyMarkers_post() {  }
//
// void HooksTdb::phaseHook_UnifyStatistics_pre() {  }
// void HooksTdb::phaseHook_UnifyStatistics_post() {  }
//

void HooksTdb::phaseHook_UnifyEvents_pre() {

    /* if this is not called, an error occurred or
       vtunify just printed the helptext */

    GET_THREAD_ID( thread_id );

    if( thread_id == 0 ) {

        RootData.create_output = true;

#ifdef VT_MPI

        CALL_MPI( MPI_Bcast( &(RootData.vt_flush_id), 1, MPI_UNSIGNED, 0,
                             MPI_COMM_WORLD ) );

#endif /* VT_MPI */

    }

}
// void HooksTdb::phaseHook_UnifyEvents_post() {  }
//
// void HooksTdb::phaseHook_WriteMasterControl_pre() {  }
// void HooksTdb::phaseHook_WriteMasterControl_post() {  }
//
// void HooksTdb::phaseHook_CleanUp_pre() {  }
// void HooksTdb::phaseHook_CleanUp_post() {  }

// record hooks
//

// definition records

// void HooksTdb::readRecHook_DefinitionComment( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_DefinitionComment( HooksVaArgs_struct & args ) {

    /* thread-safe */

    /* get comment-string */
    GETPARAM( std::string*, comment, 0 );

    //std::string *comment = (std::string*) args[0];

    std::string vt_comment = *comment;
    std::string vt_env_name;
    std::string vt_env_value;
    int start_pos;
    int pos;

    start_pos = vt_comment.find_first_not_of( ' ', 0 );

    vt_comment.erase( 0, start_pos );

    if( vt_comment.substr( 0, 3 ) == "VT_" ) {

        pos = vt_comment.find( ": ", 0 );
        vt_env_name = vt_comment.substr( 0, pos );
        vt_env_value = vt_comment.substr( pos + 2, vt_comment.length() );

        RootData.vt_env[vt_env_name] = vt_env_value;

    }

}

//
// void HooksTdb::readRecHook_DefCreator( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_DefCreator( HooksVaArgs_struct & args ) {

    /* thread-safe */

    /* get creator-string */
    GETPARAM( std::string*, creator, 0 );

    RootData.creator = *creator;

}

//
// void HooksTdb::readRecHook_DefTimerResolution( HooksVaArgs_struct & args ) {  }
// void HooksTdb::writeRecHook_DefTimerResolution( HooksVaArgs_struct & args ) {  }
//
// void HooksTdb::readRecHook_DefProcessGroup( HooksVaArgs_struct & args ) {  }
// void HooksTdb::writeRecHook_DefProcessGroup( HooksVaArgs_struct & args ) {  }
//

// void HooksTdb::readRecHook_DefProcess( HooksVaArgs_struct & args ) {  }
void
HooksTdb::writeRecHook_DefProcess( HooksVaArgs_struct & args ) {

    /* thread-safe */

    /* get parent */
    GETPARAM( uint32_t*, parent, 2 );

    if( *parent > 0 ) {

        RootData.num_threads++;

    } else {

        RootData.num_active_processes++;

    }

}

//
// void HooksTdb::readRecHook_DefSclFile( HooksVaArgs_struct & args ) {  }
// void HooksTdb::writeRecHook_DefSclFile( HooksVaArgs_struct & args ) {  }
//
// void HooksTdb::readRecHook_DefScl( HooksVaArgs_struct & args ) {  }
// void HooksTdb::writeRecHook_DefScl( HooksVaArgs_struct & args ) {  }
//
// void HooksTdb::readRecHook_DefFileGroup( HooksVaArgs_struct & args ) {  }
// void HooksTdb::writeRecHook_DefFileGroup( HooksVaArgs_struct & args ) {  }
//
// void HooksTdb::readRecHook_DefFile( HooksVaArgs_struct & args ) {  }
// void HooksTdb::writeRecHook_DefFile( HooksVaArgs_struct & args ) {  }
//

// void HooksTdb::readRecHook_DefFunctionGroup( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_DefFunctionGroup( HooksVaArgs_struct & args ) {

    /* thread-safe */

    /* get fgroup */
    GETPARAM( uint32_t*, fgroup, 0 );

    /* get fgroup_name */
    GETPARAM( std::string*, fgroup_name, 1 );

    RootData.fgroup[ *fgroup ] = *fgroup_name;


}

//
// void HooksTdb::readRecHook_DefFunction( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_DefFunction( HooksVaArgs_struct & args ) {

    /* thread-safe */


    GETPARAM( uint32_t*, function, 0 );

    GETPARAM( std::string*, name, 1 );

    GETPARAM( uint32_t*, fgroup, 2 );

    if( *name == "flush" ) {

        RootData.flush_fgroup_ids[ *function ] = *fgroup;

    }

}


//
// void HooksTdb::readRecHook_DefCollectiveOperation( HooksVaArgs_struct & args ) {  }

void HooksTdb::writeRecHook_DefCollectiveOperation( HooksVaArgs_struct & args ) {

    /* thread-safe */

    /* get collop */
    GETPARAM( uint32_t*, collop, 0 );

    /* get type */
    GETPARAM( uint32_t*, type, 2 );

    RootData.collop_def[*collop] = *type;

}

//
// void HooksTdb::readRecHook_DefCounterGroup( HooksVaArgs_struct & args ) {  }
// void HooksTdb::writeRecHook_DefCounterGroup( HooksVaArgs_struct & args ) {  }
//

// void HooksTdb::readRecHook_DefCounter( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_DefCounter( HooksVaArgs_struct & args ) {

    /* thread-safe */

    /* get counter_name */
    GETPARAM( std::string*, counter_name, 1 );

    RootData.counter.push_back( *counter_name );

}



// summary records

// void HooksTdb::readRecHook_FunctionSummary( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_FunctionSummary( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    ThreadVector[thread_id].num_stats++;

}

//
// void HooksTdb::readRecHook_MessageSummary( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_MessageSummary( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    ThreadVector[thread_id].num_stats++;

}

//
// void HooksTdb::readRecHook_CollopSummary( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_CollopSummary( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    ThreadVector[thread_id].num_stats++;

}

//
// void HooksTdb::readRecHook_FileOperationSummary( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_FileOperationSummary( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    ThreadVector[thread_id].num_stats++;

}




// marker records

// void HooksTdb::readRecHook_DefMarker( HooksVaArgs_struct & args ) {  }
// void HooksTdb::writeRecHook_DefMarker( HooksVaArgs_struct & args ) {  }
//
// void HooksTdb::readRecHook_Marker( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_Marker( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    ThreadVector[thread_id].num_marker++;

}





// event records

// void HooksTdb::readRecHook_Enter( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_Enter( HooksVaArgs_struct & args )
{

    GET_THREAD_ID( thread_id );

    GETPARAM( uint32_t*, function, 1 );

    if( *function == RootData.vt_flush_id ) {

        ThreadVector[thread_id].num_vt_flushes++;

    }

    ThreadVector[thread_id].num_enter++;

    ThreadVector[thread_id].num_events++;

}
//
// void HooksTdb::readRecHook_Leave( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_Leave( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    ThreadVector[thread_id].num_leave++;

    ThreadVector[thread_id].num_events++;

}

//
// void HooksTdb::readRecHook_FileOperation( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_FileOperation( HooksVaArgs_struct & args ) {

    /* get operation */
    GETPARAM( uint32_t*, operation, 4 );

    /* get bytes */
    GETPARAM( uint64_t*, bytes, 5 );


    handleFileOperation( operation, bytes );

}

//
// void HooksTdb::readRecHook_BeginFileOperation( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_BeginFileOperation( HooksVaArgs_struct & args )
{

    GET_THREAD_ID( thread_id );

    ThreadVector[thread_id].num_events++;

}

//
// void HooksTdb::readRecHook_EndFileOperation( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_EndFileOperation( HooksVaArgs_struct & args ) {

    /* get operation */
    GETPARAM( uint32_t*, operation, 5 );

    /* get bytes */
    GETPARAM( uint64_t*, bytes, 6 );


    handleFileOperation( operation, bytes );

}

void HooksTdb::handleFileOperation( uint32_t *operation, uint64_t *bytes) {

    GET_THREAD_ID( thread_id );

    uint32_t fileop;
    uint32_t ioflags;

#define NFLAGS 6

    uint32_t avail_flags[NFLAGS] = { OTF_IOFLAG_IOFAILED, OTF_IOFLAG_ASYNC, OTF_IOFLAG_COLL,
                                     OTF_IOFLAG_DIRECT, OTF_IOFLAG_SYNC, OTF_IOFLAG_ISREADLOCK };

    std::vector<uint32_t> set_flags;

    /* OTF_FILEOP_{OPEN,CLOSE,READ,WRITE,SEEK...} */
    fileop = *operation & OTF_FILEOP_BITS;

    /* OTF_IOFLAGS_{ASYNC,DIRECT,SYNC,COLL...} */
    ioflags = *operation & OTF_IOFLAGS_BITS;

    for( uint32_t i = 0; i < NFLAGS; i++ ) {

        if( avail_flags[i] & ioflags ) {

            set_flags.push_back( avail_flags[i] );

        }

    }

    /* check if it has no special ioflag */
    if( set_flags.size() == 0 ) {

        /* save io without ioflag in 1 */
        set_flags.push_back(1);

    }

    /* sum all io_activity in 0 */
    set_flags.push_back(0);


    for( uint32_t i = 0; i < set_flags.size(); i++ ) {


        switch( fileop ) {

          case OTF_FILEOP_OPEN:

                ThreadVector[thread_id].io[ set_flags[i] ].num_open++;

                break;

          case OTF_FILEOP_CLOSE:

                ThreadVector[thread_id].io[ set_flags[i] ].num_close++;

                break;

          case OTF_FILEOP_READ:

                ThreadVector[thread_id].io[ set_flags[i] ].num_read++;
                ThreadVector[thread_id].io[ set_flags[i] ].bytes_read += *bytes;

                break;

          case OTF_FILEOP_WRITE:

                ThreadVector[thread_id].io[ set_flags[i] ].num_written++;
                ThreadVector[thread_id].io[ set_flags[i] ].bytes_written += *bytes;

                break;

          case OTF_FILEOP_SEEK:

                ThreadVector[thread_id].io[ set_flags[i] ].num_seek++;

                break;

        }

    }

    ThreadVector[thread_id].num_events++;

}


//
// void HooksTdb::readRecHook_SendMsg( HooksVaArgs_struct & args ) {  }

void HooksTdb::writeRecHook_SendMsg( HooksVaArgs_struct & args )
{

    GET_THREAD_ID( thread_id );

    ThreadVector[thread_id].num_sent++;

    /* get msglength */
    GETPARAM( uint32_t*, msglength, 5 );

    ThreadVector[thread_id].bytes_sent += *msglength;

    ThreadVector[thread_id].num_events++;

}

//
// void HooksTdb::readRecHook_RecvMsg( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_RecvMsg( HooksVaArgs_struct & args )
{

    GET_THREAD_ID( thread_id );

    ThreadVector[thread_id].num_recv++;

    /* get msglength */
    GETPARAM( uint32_t*, msglength, 5 );

    ThreadVector[thread_id].bytes_recv += *msglength;

    ThreadVector[thread_id].num_events++;

}

//
// void HooksTdb::readRecHook_CollectiveOperation( HooksVaArgs_struct & args ) {  }

void HooksTdb::writeRecHook_CollectiveOperation( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    /* get collective */
    GETPARAM( uint32_t*, collop, 2 );

    /* get sent */
    GETPARAM( uint32_t*, sent, 5 );

    /* get received */
    GETPARAM( uint32_t*, recv, 6 );


    ThreadVector[thread_id].collop[*collop].num++;
    ThreadVector[thread_id].collop[*collop].bytes_sent += *sent;
    ThreadVector[thread_id].collop[*collop].bytes_recv += *recv;


    ThreadVector[thread_id].num_events++;

}

//  void readRecHook_BeginCollectiveOperation( HooksVaArgs_struct & args ) {}
void HooksTdb::writeRecHook_BeginCollectiveOperation( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    /* get collective */
    GETPARAM( uint32_t*, collop, 2 );

    /* get sent */
    GETPARAM( uint64_t*, sent, 6 );

    /* get received */
    GETPARAM( uint64_t*, recv, 7 );


    ThreadVector[thread_id].collop[*collop].num++;
    ThreadVector[thread_id].collop[*collop].bytes_sent += *sent;
    ThreadVector[thread_id].collop[*collop].bytes_recv += *recv;


    ThreadVector[thread_id].num_events++;

}

//  void readRecHook_EndCollectiveOperation( HooksVaArgs_struct & args ) {}
void HooksTdb::writeRecHook_EndCollectiveOperation( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    ThreadVector[thread_id].num_events++;

}

//
// void HooksTdb::readRecHook_RMAPut( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_RMAPut( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    GETPARAM( uint64_t*, bytes, 6 );

    ThreadVector[thread_id].num_rma++;
    ThreadVector[thread_id].bytes_rma += *bytes;

    ThreadVector[thread_id].num_events++;

}

//
// void HooksTdb::readRecHook_RMAPutRemoteEnd( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_RMAPutRemoteEnd( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    GETPARAM( uint64_t*, bytes, 6 );

    ThreadVector[thread_id].num_rma++;
    ThreadVector[thread_id].bytes_rma += *bytes;

    ThreadVector[thread_id].num_events++;

}

//
// void HooksTdb::readRecHook_RMAGet( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_RMAGet( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    GETPARAM( uint64_t*, bytes, 6 );

    ThreadVector[thread_id].num_rma++;
    ThreadVector[thread_id].bytes_rma += *bytes;

    ThreadVector[thread_id].num_events++;

}

//
// void HooksTdb::readRecHook_RMAEnd( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_RMAEnd( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    ThreadVector[thread_id].num_events++;

}

//
// void HooksTdb::readRecHook_Counter( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_Counter( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    ThreadVector[thread_id].num_events++;

}

//
// void HooksTdb::readRecHook_EventComment( HooksVaArgs_struct & args ) {  }
void HooksTdb::writeRecHook_EventComment( HooksVaArgs_struct & args ) {

    GET_THREAD_ID( thread_id );

    ThreadVector[thread_id].num_events++;

}




// set thread num hook
void
HooksTdb::setThreadNumHook( const int & threadnum )
{
  /* add first element in init_hook, because if omp is not set, this is never called */

    //std::cout << "Threads: " << threadnum << std::endl;

    // create datastructure
    if( ThreadVector.size() > 1 ) {

        // error

    } else {

        ThreadVector.resize( threadnum );

    }

}

// generic hook
// void HooksTdb::genericHook( const uint32_t & id, HooksVaArgs_struct & args ) {  }

