AC_DEFUN([ACVT_CUDARTWRAP],
[
	cudartwrap_error="no"
	have_cudartwrap="no"

	CUDARTDIR=
	CUDARTINCDIR=
	CUDARTLIBDIR=
	CUDARTLIB=

	cudartlib_pathname=

	AC_ARG_VAR(NVCC, [NVIDIA CUDA compiler command])

	AC_ARG_WITH(cudart-dir,
		AC_HELP_STRING([--with-cudart-dir=CUDARTDIR],
		[give the path for CUDA Toolkit, default: /usr/local/cuda]),
	[CUDARTDIR="$withval/"], [CUDARTDIR="/usr/local/cuda/"])

	AC_ARG_WITH(cudart-inc-dir,
		AC_HELP_STRING([--with-cudart-inc-dir=CUDARTINCDIR],
		[give the path for CUDA Toolkit-include files, default: CUDARTDIR/include]),
	[CUDARTINCDIR="-I$withval/"],
	[AS_IF([test x"$CUDARTDIR" != x], [CUDARTINCDIR="-I$CUDARTDIR"include/])])

	AC_ARG_WITH(cudart-lib-dir,
		AC_HELP_STRING([--with-cudart-lib-dir=CUDARTLIBDIR],
		[give the path for CUDA Toolkit-libraries, default: CUDARTDIR/lib]),
	[CUDARTLIBDIR="-L$withval/"],
	[AS_IF([test x"$CUDARTDIR" != x], [CUDARTLIBDIR="-L$CUDARTDIR"lib/])])

	AC_ARG_WITH(cudart-lib,
		AC_HELP_STRING([--with-cudart-lib=CUDARTLIB], [use given cudart lib, default: -lcudart]),
	[CUDARTLIB="$withval"])

	AC_ARG_WITH(cudart-shlib,
		AC_HELP_STRING([--with-cudart-shlib=CUDARTSHLIB], [give the pathname for the shared CUDA runtime library, default: automatically by configure]),
	[
		AS_IF([test x"$withval" = "xyes" -o x"$withval" = "xno"],
		[AC_MSG_ERROR([value of '--with-cudart-shlib' not properly set])])
                cudartlib_pathname=$withval
	])

	AC_MSG_CHECKING([whether we are using the GNU C compiler])
	
	AS_IF([test x"$ac_cv_c_compiler_gnu" = "xyes"],
	[
		AC_MSG_RESULT([yes])
	],
	[
		AC_MSG_RESULT([no])
		AC_MSG_NOTICE([error: C compiler is not GNU compatible!])
		cudartwrap_error="yes"
	])

	AS_IF([test x"$cudartwrap_error" = "xno"],
	[
		sav_CPPFLAGS=$CPPFLAGS
		CPPFLAGS="$CPPFLAGS $CUDARTINCDIR"
		AC_CHECK_HEADER([cuda_runtime_api.h], [],
		[
			AC_MSG_NOTICE([error: no cuda_runtime_api.h found; check path for CUDA Toolkit first...])
			cudartwrap_error="yes"
		])
		CPPFLAGS=$sav_CPPFLAGS
	])

	AS_IF([test x"$CUDARTLIB" = x -a x"$cudartwrap_error" = "xno"],
	[
		sav_LIBS=$LIBS
		LIBS="$LIBS $CUDARTLIBDIR -lcudart"
		AC_MSG_CHECKING([whether linking with -lcudart works])
		AC_TRY_LINK([],[],
		[AC_MSG_RESULT([yes]); CUDARTLIB=-lcudart],[AC_MSG_RESULT([no])])
		LIBS=$sav_LIBS
	])

	AS_IF([test x"$CUDARTLIB" = x -a x"$cudartwrap_error" = "xno"],
	[
		AC_MSG_NOTICE([error: no libcudart found; check path for CUDA Toolkit first...])
		cudartwrap_error="yes"
	])

	AS_IF([test x"$cudartwrap_error" = "xno"],
	[
		AC_MSG_CHECKING([whether CUDA runtime version >= 2.2])

		sav_CPPFLAGS=$CPPFLAGS
		CPPFLAGS="$CPPFLAGS $CUDARTINCDIR"
		AC_TRY_COMPILE([#include "cuda_runtime_api.h"],
[
#ifndef CUDART_VERSION
#  error "CUDART_VERSION not defined"
#elif CUDART_VERSION < 2020
#  error "CUDART_VERSION < 2020"
#endif
],
		[AC_MSG_RESULT([yes])],
		[
			AC_MSG_RESULT([no])
			AC_MSG_NOTICE([error: CUDA runtime version could not be determined and/or is incompatible (< 2.2)
See \`config.log' for more details.])
			cudartwrap_error="yes"
                ])
                CPPFLAGS=$sav_CPPFLAGS
	])

	AS_IF([test x"$cudartwrap_error" = "xno"],
	[
		AC_MSG_CHECKING([for pathname of CUDA runtime library])

		AS_IF([test x"$cudartlib_pathname" != x],
		[
			AC_MSG_RESULT([skipped (--with-cudart-shlib=$cudartlib_pathname)])
		],
		[
			AS_IF([test x"$have_rtld_next" = "xyes"],
			[
				AC_MSG_RESULT([not needed])
			],
			[
				AS_IF([test x"$CUDARTLIBDIR" != x],
				[cudartlib_dir=`echo $CUDARTLIBDIR | sed s/\-L//`])
				cudartlib_pathname=$cudartlib_dir`echo $CUDARTLIB | sed s/\-l/lib/`".so"

				AS_IF([! test -f $cudartlib_pathname],
				[
					AC_MSG_RESULT([unknown])
					AC_MSG_NOTICE([error: could not determine pathname of CUDA runtime library!])
					cudartwrap_error="yes"
				],
				[
					AC_MSG_RESULT([$cudartlib_pathname])
				])
			])
		])
	])

	AS_IF([test x"$cudartwrap_error" = "xno"],
	[
		AC_CHECK_PROG(NVCC, nvcc, nvcc)

		AS_IF([test x"$cudartlib_pathname" != x],
		[
			AC_DEFINE_UNQUOTED([DEFAULT_CUDARTLIB_PATHNAME],
			["$cudartlib_pathname"], [pathname of CUDA runtime library])
		])

		have_cudartwrap="yes"
	])

	AC_SUBST(CUDARTINCDIR)
	AC_SUBST(CUDARTLIBDIR)
	AC_SUBST(CUDARTLIB)
])

