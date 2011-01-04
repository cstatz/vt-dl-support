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
#include "vt_unify_hooks_stats.h"

#include "vt_inttypes.h"

#include <iostream>

#include <assert.h>
#include <stdio.h>

//////////////////// class HooksStats ////////////////////

// public methods
//

HooksStats::HooksStats() : HooksBase(), m_lTimerRes(1)
{
   // Empty
}

HooksStats::~HooksStats()
{
   // Empty
}

// private methods
//

// initialization/finalization hooks
//

void
HooksStats::initHook()
{
   // Empty
}

void
HooksStats::finalizeHook( const bool & error )
{
   if( !error && isFuncStatAvail() )
   {
      // write summary function statistics to file
      printFuncStat( Params.stats_out_file );

      // print summary function statistics to stdout
      if( !Params.bequiet )
      {
         std::cout << std::endl;
         printFuncStat( "" );
         std::cout << std::endl
                   << "The complete function summary was written to file '"
                   << Params.stats_out_file << "'." << std::endl;
      }
   }
}

void
HooksStats::readRecHook_DefTimerResolution( HooksVaArgs_struct & args )
{
   assert( args.num() == 2 );

   // set timer resolution
   m_lTimerRes = *((uint64_t*)args[1]);
}

void
HooksStats::writeRecHook_DefFunction( HooksVaArgs_struct & args )
{
   assert( args.num() == 4 );

   uint32_t    func_id      = *((uint32_t*)args[0]);
   std::string func_name    = *((std::string*)args[1]);
   //uint32_t    func_group = *((uint32_t*)args[2]);
   //uint32_t    func_scl   = *((uint32_t*)args[3]);

   // add function
   addFunc( func_id, func_name );
}

void
HooksStats::writeRecHook_FunctionSummary( HooksVaArgs_struct & args )
{
   assert( args.num() == 6 );

   //uint64_t time      = *((uint64_t*)args[0]);
   uint32_t func_id = *((uint32_t*)args[1]);
   uint32_t proc_id = *((uint32_t*)args[2]);
   uint64_t count   = *((uint64_t*)args[3]);
   uint64_t excl    = *((uint64_t*)args[4]);
   uint64_t incl    = *((uint64_t*)args[5]);

   // add function statistics
   addFuncStat( proc_id, func_id, count, incl, excl );
}

bool
HooksStats::addFunc( const uint32_t funcId, const std::string& funcName )
{
   std::map<uint32_t, std::string>::iterator it =
      m_mapFuncIdName.find( funcId );

   if( it == m_mapFuncIdName.end() )
      m_mapFuncIdName.insert( std::make_pair( funcId, funcName ) );

   return true;
}

bool
HooksStats::addFuncStat( const uint32_t procId, const uint32_t funcId,
                         const uint64_t count, const uint64_t incl,
                         const uint64_t excl )
{
   // search function statistics map by process id
   //
   std::map<uint32_t, std::map<uint32_t, struct FuncStat_struct*>*>
      ::iterator proc_it = m_mapProcIdFuncStat.find( procId );
   // found ?
   if( proc_it == m_mapProcIdFuncStat.end() )
   {
      // no -> create function statistics map
      //
      std::map<uint32_t, struct FuncStat_struct*>* p_map_funcid_stat =
         new std::map<uint32_t, struct FuncStat_struct*>();

      m_mapProcIdFuncStat.insert( std::make_pair( procId,
                                                  p_map_funcid_stat ) );
      proc_it = m_mapProcIdFuncStat.find( procId );
      assert( proc_it != m_mapProcIdFuncStat.end() );
   }

   // search function statistics by function id
   //
   std::map<uint32_t, struct FuncStat_struct*>::iterator func_it =
      proc_it->second->find( funcId );
   // found ?
   if( func_it == proc_it->second->end() )
   {
      // no -> create function statistics
      //
      std::string func_name = getFuncNameById(funcId);
      assert( func_name != "" );
      struct FuncStat_struct* p_func_stat =
         new struct FuncStat_struct( funcId, func_name );

      proc_it->second->insert( std::make_pair( funcId, p_func_stat ) );
      func_it = proc_it->second->find( funcId );
      assert( func_it != proc_it->second->end() );
   }

   // overwrite function statistics values
   //
   func_it->second->count = (double)count;
   func_it->second->incl = incl;
   func_it->second->excl = excl;

   return true;
}

std::vector<struct HooksStats::FuncStat_struct>
HooksStats::getFuncStat()
{
   std::vector<struct FuncStat_struct> vec_sum_func_stat;

   std::map<uint32_t, std::map<uint32_t, struct FuncStat_struct*>*>::iterator
      proc_it;

   for( proc_it = m_mapProcIdFuncStat.begin();
        proc_it != m_mapProcIdFuncStat.end(); proc_it++ )
   {
      std::vector<struct FuncStat_struct> vec_func_stat =
         getFuncStat( proc_it->first );

      for( uint32_t i = 0; i < vec_func_stat.size(); i++ )
      {
         std::vector<struct FuncStat_struct>::iterator func_it =
            std::find_if( vec_sum_func_stat.begin(), vec_sum_func_stat.end(),
                          FuncStat_funcId_eq( vec_func_stat[i].funcid ) );
         if( func_it == vec_sum_func_stat.end() )
         {
            struct FuncStat_struct func_stat(vec_func_stat[i].funcid,
                                             vec_func_stat[i].funcname,
                                             vec_func_stat[i].count,
                                             vec_func_stat[i].incl,
                                             vec_func_stat[i].excl);
            vec_sum_func_stat.push_back( func_stat );
         }
         else
         {
            func_it->count += vec_func_stat[i].count;
            func_it->incl += vec_func_stat[i].incl;
            func_it->excl += vec_func_stat[i].excl;
         }
      }
   }

   if( m_mapProcIdFuncStat.size() > 1 )
   {
      uint32_t nprocs = m_mapProcIdFuncStat.size();
      for( uint32_t i = 0; i < vec_sum_func_stat.size(); i++ )
      {
         vec_sum_func_stat[i].count /= (double)nprocs;
         vec_sum_func_stat[i].incl /= nprocs;
         vec_sum_func_stat[i].excl /= nprocs;
      }
   }

   return vec_sum_func_stat;
}

std::vector<struct HooksStats::FuncStat_struct>
HooksStats::getFuncStat( uint32_t procId )
{
   std::vector<struct FuncStat_struct> vec_func_stat;

   // search function statistics map by process id
   //
   std::map<uint32_t, std::map<uint32_t, struct FuncStat_struct*>*>
      ::iterator proc_it = m_mapProcIdFuncStat.find( procId );
   assert( proc_it != m_mapProcIdFuncStat.end() );

   for( std::map<uint32_t, struct FuncStat_struct*>::iterator it =
           proc_it->second->begin(); it != proc_it->second->end(); it++ )
   {
      vec_func_stat.push_back(*(it->second)); 
   }

   return vec_func_stat;
}

bool
HooksStats::printFuncStat( std::string outFile )
{
   // get vector of function statistics
   std::vector<struct FuncStat_struct> vec_func_stat = 
      getFuncStat();

   return printFuncStat( outFile, vec_func_stat );
}

bool
HooksStats::printFuncStat( std::string outFile, uint32_t procId )
{
   // get vector of function statistics by process id
   std::vector<struct FuncStat_struct> vec_func_stat =
      getFuncStat( procId );

   return printFuncStat( outFile, vec_func_stat );
}

bool
HooksStats::printFuncStat( std::string outFile,
                           std::vector<struct FuncStat_struct> & vecFuncStat )
{
   const uint32_t max_lines_on_stdout = 10;

   FILE * out;

   // open statistics output file, if given
   //
   if( outFile.length() != 0 )
   {
      if( !( out = fopen( outFile.c_str(), "w" ) ) )
      {
         std::cerr << ExeName << ": Error: "
                   << "Could not open file " << outFile << std::endl;
         return false;
      }
   }
   // otherwise, print on stdout
   else
   {
      out = stdout;
   }

   // sort function statistics vector
   //
   int sortFlags = Params.stats_sort_flags;
   std::sort( vecFuncStat.begin(), vecFuncStat.end(),
              std::less<struct FuncStat_struct>() );

   // print out function statistics
   //
   fprintf( out, "                                   %cexcl. time %cincl. time\n",
            (sortFlags & STAT_SORT_FLAG_EXCL_CALL) ? '*' : ' ',
            (sortFlags & STAT_SORT_FLAG_INCL_CALL) ? '*' : ' ' );

   fprintf( out, "%cexcl. time %cincl. time      calls      / call      / call %cname\n",
            (sortFlags & STAT_SORT_FLAG_EXCL) ? '*' : ' ',
            (sortFlags & STAT_SORT_FLAG_INCL) ? '*' : ' ',
            (sortFlags & STAT_SORT_FLAG_FUNCNAME) ? '*' : ' ' );

   // reduce output lines, if necessary
   //
   uint32_t size = vecFuncStat.size();
   if( out == stdout && size > max_lines_on_stdout )
      size = max_lines_on_stdout;

   for( uint32_t i = 0; i < size; i++ )
   {
      std::string str_excl = formatTime( vecFuncStat[i].excl );
      std::string str_incl = formatTime( vecFuncStat[i].incl );
      std::string str_excl_call =
         formatTime( (uint64_t)((double)vecFuncStat[i].excl / vecFuncStat[i].count) );
      std::string str_incl_call =
         formatTime( (uint64_t)((double)vecFuncStat[i].incl / vecFuncStat[i].count) );
      std::string str_funcname = vecFuncStat[i].funcname;

      if( out == stdout ) str_funcname = shortName( vecFuncStat[i].funcname ); 

      fprintf( out,
               "%11s %11s %10.*f %11s %11s  %s\n",
               str_excl.c_str(),
               str_incl.c_str(),
               ((double)((uint64_t)vecFuncStat[i].count) ==
                vecFuncStat[i].count) ? 0 : 2,
               vecFuncStat[i].count,
               str_excl_call.c_str(),
               str_incl_call.c_str(),
               str_funcname.c_str() );
   }

   if( out == stdout && size < vecFuncStat.size() )
   {
      fprintf( out, "Displayed %u from %u functions.\n",
               size, (uint32_t)vecFuncStat.size() );
   }

   // close statistics output file, if necessary
   if( out != stdout ) fclose( out );

   return true;
}

bool
HooksStats::isFuncStatAvail()
{
   // get vector of function statistics
   std::vector<struct FuncStat_struct> vec_func_stat =
      getFuncStat();

   return vec_func_stat.size() > 0;
}

bool
HooksStats::isFuncStatAvail( uint32_t procId )
{
   // get vector of function statistics by process id
   std::vector<struct FuncStat_struct> vec_func_stat = 
      getFuncStat( procId );

   return vec_func_stat.size() > 0;
}

std::string
HooksStats::getFuncNameById( const uint32_t funcId )
{
   // search function name by function id
   //
   std::map<uint32_t, std::string>::iterator it =
      m_mapFuncIdName.find( funcId );
   if( it != m_mapFuncIdName.end() )
      return it->second;       // return function name, if found
   else
      return std::string("");  // otherwise return ""
}

std::string
HooksStats::shortName( const std::string & longName, uint32_t len )
{
   assert( len >= 5 );

   std::string short_name;

   if( longName.length() <= len ) 
   {
      short_name = longName;
   }
   else
   {
      std::string f, b;

      f = longName.substr( 0, (len-3) / 2 ) + "...";
      b = longName.substr( longName.length()-(len-f.length()));
      short_name = f+b;
   }

   return short_name;
}

std::string
HooksStats::formatTime( uint64_t time )
{
   char str[20];
   double d_time = (double)time;
   double d_res = (double)m_lTimerRes;
   double sec = d_time / d_res;

   static const char unit[4][3] = { "s ", "ms", "us", "ns" };

   for( uint32_t i = 0; i < 4; i++ )
   {
      if( i == 3 || sec >= 0.1 )
      {
         snprintf( str, sizeof( str ) - 1, "%.3f%s", sec, unit[i] );
         break;
      }
      sec *= 1000.0;
   }

   return std::string( str );
}
