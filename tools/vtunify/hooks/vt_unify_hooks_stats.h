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

#ifndef _VT_UNIFY_HOOKS_STATS_H_
#define _VT_UNIFY_HOOKS_STATS_H_

#include "vt_unify.h"
#include "vt_unify_hooks_base.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <vector>

//
// HooksStats class
//
class HooksStats : public HooksBase
{
public:

   //
   // function statistics sort flags
   //
   enum
   {
      STAT_SORT_FLAG_DIR_UP    = 0x1,
      STAT_SORT_FLAG_DIR_DOWN  = 0x2,
      STAT_SORT_FLAG_FUNCNAME  = 0x4,
      STAT_SORT_FLAG_COUNT     = 0x8,
      STAT_SORT_FLAG_INCL      = 0x10,
      STAT_SORT_FLAG_EXCL      = 0x20,
      STAT_SORT_FLAG_INCL_CALL = 0x40,
      STAT_SORT_FLAG_EXCL_CALL = 0x80

   };

   // contructor
   HooksStats();

   // destructor
   ~HooksStats();

private:

   //
   // function statistics structure
   //
   struct FuncStat_struct
   {
      FuncStat_struct()
         : funcid(0), funcname(""), count(0.0), incl(0), excl(0) {}

      FuncStat_struct(uint32_t _funcid, std::string _funcname)
         : funcid(_funcid), funcname(_funcname), count(0.0), incl(0), excl(0) {}

      FuncStat_struct(uint32_t _funcid, std::string _funcname, double _count,
                      uint64_t _incl, uint64_t _excl) 
         : funcid(_funcid), funcname(_funcname), count(_count),
           incl(_incl), excl(_excl) {}

      uint32_t    funcid;   // function identifier
      std::string funcname; // function name
      double      count;    // number of calls
      uint64_t    incl;     // inclusive time
      uint64_t    excl;     // exclusive time

      bool operator<(const struct FuncStat_struct & a) const
      {
         int flags = Params.stats_sort_flags;

         if( (flags & STAT_SORT_FLAG_FUNCNAME) &&
             (flags & STAT_SORT_FLAG_DIR_UP ) )
         {
            std::string _a = funcname, _b = a.funcname;
            uint32_t i;

            for( i = 0; i < funcname.length(); i++ )
               _a[i] = tolower( funcname[i] );
            for( i = 0; i < a.funcname.length(); i++ )
               _b[i] = tolower( a.funcname[i] );

            return _a < _b;
         }
         else if( (flags & STAT_SORT_FLAG_FUNCNAME) &&
                  (flags & STAT_SORT_FLAG_DIR_DOWN ) )
         {
            return funcname > a.funcname;
         }
         else if( (flags & STAT_SORT_FLAG_COUNT) &&
                  (flags & STAT_SORT_FLAG_DIR_UP ) )
         {
            return count < a.count;
         }
         else if( (flags & STAT_SORT_FLAG_COUNT) &&
                  (flags & STAT_SORT_FLAG_DIR_DOWN ) )
         {
            return count > a.count;
         }
         else if( (flags & STAT_SORT_FLAG_INCL) &&
                  (flags & STAT_SORT_FLAG_DIR_UP ) )
         {
            return incl < a.incl;
         }
         else if( (flags & STAT_SORT_FLAG_INCL) &&
                  (flags & STAT_SORT_FLAG_DIR_DOWN ) )
         {
            return incl > a.incl;
         }
         else if( (flags & STAT_SORT_FLAG_EXCL) &&
                  (flags & STAT_SORT_FLAG_DIR_UP ) )
         {
            return excl < a.excl;
         }
         else if( (flags & STAT_SORT_FLAG_EXCL) &&
                  (flags & STAT_SORT_FLAG_DIR_DOWN ) )
         {
            return excl > a.excl;
         }
         else if( (flags & STAT_SORT_FLAG_INCL_CALL) &&
                  (flags & STAT_SORT_FLAG_DIR_UP ) )
         {
            return incl / count < a.incl / a.count;
         }
         else if( (flags & STAT_SORT_FLAG_INCL_CALL) &&
                  (flags & STAT_SORT_FLAG_DIR_DOWN ) )
         {
            return incl / count > a.incl / a.count;
         }
         else if( (flags & STAT_SORT_FLAG_EXCL_CALL) &&
                  (flags & STAT_SORT_FLAG_DIR_UP ) )
         {
            return excl / count < a.excl / a.count;
         }
         else if( (flags & STAT_SORT_FLAG_EXCL_CALL) &&
                  (flags & STAT_SORT_FLAG_DIR_DOWN ) )
         {
            return excl / count > a.excl / a.count;
         }
         else
         {
            return true;
         }
      }

   };

   // class for compare function identifier
   //
   class FuncStat_funcId_eq :
      public std::unary_function<struct FuncStat_struct, bool>
   {
      uint32_t funcid;
   public:
      explicit FuncStat_funcId_eq(const uint32_t & _funcid)
         : funcid(_funcid) {}
      bool operator()(const struct FuncStat_struct & a) const 
      {
         return a.funcid == funcid;
      }

   };

   // hook methods
   //
   void initHook( void );
   void finalizeHook( const bool & error );
   void readRecHook_DefTimerResolution( HooksVaArgs_struct & args );
   void writeRecHook_DefFunction( HooksVaArgs_struct & args );
   void writeRecHook_FunctionSummary( HooksVaArgs_struct & args );

   // add function definiton
   // (called by writeRecHook_DefFunction)
   bool addFunc( const uint32_t funcId, const std::string& funcName );

   // add function statistics
   // (called by writeRecHook_FunctionSummary)
   bool addFuncStat( const uint32_t procId, const uint32_t funcId,
                     const uint64_t count, const uint64_t incl,
                     const uint64_t excl );

   // get vector of function statistics
   //
   std::vector<struct FuncStat_struct> getFuncStat();
   std::vector<struct FuncStat_struct> getFuncStat( uint32_t procId );

   // write function statistics to file
   //
   bool printFuncStat( std::string outFile );
   bool printFuncStat( std::string outFile, uint32_t procId );
   bool printFuncStat( std::string outFile,
                       std::vector<struct FuncStat_struct> & vecFuncStat );

   // check for available function statistics
   //
   bool isFuncStatAvail();
   bool isFuncStatAvail( uint32_t procId );

   // get function name by id
   std::string getFuncNameById( const uint32_t funcId );

   // trim function name (only for output on stdout)
   std::string shortName( const std::string & longName, uint32_t len = 20 );

   // convert timestamp to a human readable format
   std::string formatTime( uint64_t time );

   // map function id -> function name
   std::map<uint32_t, std::string> m_mapFuncIdName;

   // map process id -> map function id -> function statistics
   std::map<uint32_t, std::map<uint32_t, struct FuncStat_struct*>*>
      m_mapProcIdFuncStat;

   // timer resolution
   uint64_t m_lTimerRes;

};

#endif // _VT_UNIFY_HOOKS_STATS_H_
