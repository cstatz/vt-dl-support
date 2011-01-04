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

#ifndef _VT_UNIFY_STATS_H_
#define _VT_UNIFY_STATS_H_

//
// Statistics class
//
class Statistics
{
public:

   // contructor
   Statistics();

   // destructor
   ~Statistics();

   bool run();

};

// instance of class Statistics
extern Statistics * theStatistics;

#endif // _VT_UNIFY_STATS_H_
