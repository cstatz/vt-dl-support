AC_DEFUN([ACVT_CUDAWRAP],
[
	cudawrap_error="no"
	have_cudawrap="no"

	CUDADIR=
	CUDAINCDIR=
	CUDALIBDIR=
	CUDALIB=

	cudalib_pathname=

	AC_ARG_WITH(cuda-dir,
		AC_HELP_STRING([--with-cuda-dir=CUDADIR],
		[give the path for CUDA driver, default: /usr]),
	[CUDADIR="$withval/"])

	AC_ARG_WITH(cuda-inc-dir,
		AC_HELP_STRING([--with-cuda-inc-dir=CUDAINCDIR],
		[give the path for CUDA driver-include files, default: CUDADIR/include/cuda]),
	[CUDAINCDIR="-I$withval/"],
	[AS_IF([test x"$CUDADIR" != x], [CUDAINCDIR="-I$CUDADIR"include/cuda/], [CUDAINCDIR="-I/usr/include/cuda/"])])

	AC_ARG_WITH(cuda-lib-dir,
		AC_HELP_STRING([--with-cuda-lib-dir=CUDALIBDIR],
		[give the path for CUDA driver-libraries, default: CUDADIR/lib]),
	[CUDALIBDIR="-L$withval/"],
	[AS_IF([test x"$CUDADIR" != x], [CUDALIBDIR="-L$CUDADIR"lib/])])

	AC_ARG_WITH(cuda-lib,
		AC_HELP_STRING([--with-cuda-lib=CUDALIB], [use given cuda lib, default: -lcuda]),
	[CUDALIB="$withval"])

	AC_ARG_WITH(cuda-shlib,
		AC_HELP_STRING([--with-cuda-shlib=CUDASHLIB], [give the pathname for the shared CUDA runtime library, default: automatically by configure]),
	[
		AS_IF([test x"$withval" = "xyes" -o x"$withval" = "xno"],
		[AC_MSG_ERROR([value of '--with-cuda-shlib' not properly set])])
		cudalib_pathname=$withval
	])

	AS_IF([test x"$cudawrap_error" = "xno"],
	[
		sav_CPPFLAGS=$CPPFLAGS
		CPPFLAGS="$CPPFLAGS $CUDAINCDIR"
		AC_CHECK_HEADER([cuda.h], [],
		[
			AC_MSG_NOTICE([error: no cuda.h found; check path for CUDA driver first...])
			cudawrap_error="yes"
		])
		CPPFLAGS=$sav_CPPFLAGS
	])

	AS_IF([test x"$CUDALIB" = x -a x"$cudawrap_error" = "xno"],
	[
		sav_LIBS=$LIBS
		LIBS="$LIBS $CUDALIBDIR -lcuda"
		AC_MSG_CHECKING([whether linking with -lcuda works])
		AC_TRY_LINK([],[],
		[AC_MSG_RESULT([yes]); CUDALIB=-lcuda],[AC_MSG_RESULT([no])])
		LIBS=$sav_LIBS
	])

	AS_IF([test x"$CUDALIB" = x -a x"$cudawrap_error" = "xno"],
	[
		AC_MSG_NOTICE([error: no libcuda found; check path for CUDA driver first...])
		cudawrap_error="yes"
	])

	AS_IF([test x"$cudawrap_error" = "xno"],
	[
		AC_MSG_CHECKING([whether CUDA driver version >= 3.0])

		sav_CPPFLAGS=$CPPFLAGS
		CPPFLAGS="$CPPFLAGS $CUDAINCDIR"
		AC_TRY_COMPILE([#include "cuda.h"],
[
#ifndef CUDA_VERSION
#  error "CUDA_VERSION not defined"
#elif CUDA_VERSION < 3000
#  error "CUDA_VERSION < 3000"
#endif
],
		[AC_MSG_RESULT([yes])],
		[
			AC_MSG_RESULT([no])
			AC_MSG_NOTICE([error: CUDA driver version could not be determined and/or is incompatible (< 3.0)
 See \`config.log' for more details.])
			cudawrap_error="yes"
                ])
                CPPFLAGS=$sav_CPPFLAGS
	])

	AS_IF([test x"$cudawrap_error" = "xno"],
	[
		AC_MSG_CHECKING([for pathname of CUDA driver library])

		AS_IF([test x"$cudalib_pathname" != x],
		[
			AC_MSG_RESULT([skipped (--with-cuda-shlib=$cudalib_pathname)])
		],
		[
			AS_IF([test x"$have_rtld_next" = "xyes"],
			[
				AC_MSG_RESULT([not needed])
			],
			[
				AS_IF([test x"$CUDALIBDIR" != x],
				[cudalib_dir=`echo $CUDALIBDIR | sed s/\-L//`])
				cudalib_pathname=$cudalib_dir`echo $CUDALIB | sed s/\-l/lib/`".so"

				AS_IF([! test -f $cudalib_pathname],
				[
					AC_MSG_RESULT([unknown])
					AC_MSG_NOTICE([error: could not determine pathname of CUDA driver library!])
					cudawrap_error="yes"
				],
				[
					AC_MSG_RESULT([$cudalib_pathname])
				])
			])
		])
	])

	AS_IF([test x"$cudawrap_error" = "xno"],
	[
		AS_IF([test x"$cudalib_pathname" != x],
		[
			AC_DEFINE_UNQUOTED([DEFAULT_CUDALIB_PATHNAME],
			["$cudalib_pathname"], [pathname of CUDA driver library])
		])

		have_cudawrap="yes"
	])

	AC_SUBST(CUDAINCDIR)
	AC_SUBST(CUDALIBDIR)
	AC_SUBST(CUDALIB)
])

