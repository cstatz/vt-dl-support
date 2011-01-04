/**
 * VampirTrace
 * http://www.tu-dresden.de/zih/vampirtrace
 *
 * Copyright (c) 2005-2009, ZIH, TU Dresden, Federal Republic of Germany
 *
 * Copyright (c) 1998-2005, Forschungszentrum Juelich, Juelich Supercomputing
 *                          Centre, Federal Republic of Germany
 *
 * See the file COPYING in the package base directory for details
 **/

#ifndef _VT_CUDARTWRAP_H_
#define _VT_CUDARTWRAP_H_

#ifdef __cplusplus
# define EXTERN extern "C"
#else
# define EXTERN extern
#endif

#if (defined(VT_CUDARTWRAP))

EXTERN void vt_cudartwrap_finalize(void);

#endif

#endif /* _VT_CUDARTWRAP_H_ */
