AUTOMAKE_OPTIONS = -Wno-gnu

if AMBUILDBINARIES

bin_PROGRAMS = @CROSS_PREFIX@vtwrapper

if AMHAVECSFS
bin_LINK_vtCC = @CROSS_PREFIX@vtCC$(EXEEXT)
else
bin_LINK_vtCC =
endif

if AMBUILDVTNVCC
bin_LINK_vtnvcc = @CROSS_PREFIX@vtnvcc$(EXEEXT)
else
bin_LINK_vtnvcc =
endif

bin_LINKS = \
	@CROSS_PREFIX@vtcc$(EXEEXT) \
	@CROSS_PREFIX@vtc++$(EXEEXT) \
	@CROSS_PREFIX@vtcxx$(EXEEXT) \
	$(bin_LINK_vtCC) \
	@CROSS_PREFIX@vtf77$(EXEEXT) \
	@CROSS_PREFIX@vtf90$(EXEEXT) \
	$(bin_LINK_vtnvcc)

install-exec-hook:
	(cd $(DESTDIR)$(bindir); \
	for l in $(bin_LINKS); do \
		rm -f $$l; $(LN_S) @CROSS_PREFIX@vtwrapper$(EXEEXT) $$l; \
	done)

endif # AMBUILDBINARIES

if AMBUILDVTNVCC
vtdata_DATA_vtnvcc = @CROSS_PREFIX@vtnvcc-wrapper-data.txt
else
vtdata_DATA_vtnvcc =
endif

if AMHAVETAUINST
vtdata_DATA_tauinst_spec = TAUINST.SPEC
else
vtdata_DATA_tauinst_spec =
endif

vtdata_DATA = \
	@CROSS_PREFIX@vtcc-wrapper-data.txt \
	@CROSS_PREFIX@vtc++-wrapper-data.txt \
	@CROSS_PREFIX@vtf77-wrapper-data.txt \
	@CROSS_PREFIX@vtf90-wrapper-data.txt \
	$(vtdata_DATA_vtnvcc) \
	$(vtdata_DATA_tauinst_spec)
vtdatadir = $(datadir)

install-data-hook-always:
	(cd $(DESTDIR)$(datadir); \
	rm -f @CROSS_PREFIX@vtcxx-wrapper-data.txt; \
	$(LN_S) @CROSS_PREFIX@vtc++-wrapper-data.txt @CROSS_PREFIX@vtcxx-wrapper-data.txt)

if AMHAVECSFS
install-data-hook: install-data-hook-always
	(cd $(DESTDIR)$(datadir); \
	rm -f @CROSS_PREFIX@vtCC-wrapper-data.txt; \
	$(LN_S) @CROSS_PREFIX@vtc++-wrapper-data.txt @CROSS_PREFIX@vtCC-wrapper-data.txt)
else # AMHAVECSFS
install-data-hook: install-data-hook-always
endif # AMHAVECSFS

if AMHAVEF77
HAVEF77CXXFLAGS = -DHAVE_F77
else
HAVEF77CXXFLAGS =
endif

if AMHAVEF90
HAVEF90CXXFLAGS = -DHAVE_F90
else
HAVEF90CXXFLAGS =
endif

if AMHAVEMPI
HAVEMPICXXFLAGS = -DHAVE_MPI
else
HAVEMPICXXFLAGS =
endif

if AMHAVEFMPI
HAVEFMPICXXFLAGS = -DHAVE_FMPI
else
HAVEFMPICXXFLAGS =
endif

if AMHAVETHREADS
HAVETHREADSCXXFLAGS = -DHAVE_THREADS
if AMHAVEOMP
HAVEOMPCXXFLAGS = -DHAVE_OMP
endif
else
HAVETHREADSCXXFLAGS =
HAVEOMPCXXFLAGS =
endif

if AMHAVECROSSPREFIX
@CROSS_PREFIX@vtcc-wrapper-data.txt: vtcc-wrapper-data.txt
	cp vtcc-wrapper-data.txt @CROSS_PREFIX@vtcc-wrapper-data.txt
@CROSS_PREFIX@vtc++-wrapper-data.txt: vtc++-wrapper-data.txt
	cp vtc++-wrapper-data.txt @CROSS_PREFIX@vtc++-wrapper-data.txt
@CROSS_PREFIX@vtf77-wrapper-data.txt: vtf77-wrapper-data.txt
	cp vtf77-wrapper-data.txt @CROSS_PREFIX@vtf77-wrapper-data.txt
@CROSS_PREFIX@vtf90-wrapper-data.txt: vtf90-wrapper-data.txt
	cp vtf90-wrapper-data.txt @CROSS_PREFIX@vtf90-wrapper-data.txt
@CROSS_PREFIX@vtnvcc-wrapper-data.txt: vtnvcc-wrapper-data.txt
	cp vtnvcc-wrapper-data.txt @CROSS_PREFIC@vtnvcc-wrapper-data.txt

CROSSPREFIXCLEANFILES = \
	@CROSS_PREFIX@vtcc-wrapper-data.txt \
	@CROSS_PREFIX@vtc++-wrapper-data.txt \
	@CROSS_PREFIX@vtf77-wrapper-data.txt \
	@CROSS_PREFIX@vtf90-wrapper-data.txt \
	@CROSS_PREFIX@vtnvcc-wrapper-data.txt
else # AMHAVECROSSPREFIX
CROSSPREFIXCLEANFILES =
endif # AMHAVECROSSPREFIX

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir)/util -I$(top_builddir)/util

CC = $(CC_FOR_BUILD)
CFLAGS = $(CFLAGS_FOR_BUILD)
CPP = $(CPP_FOR_BUILD)
CXX = $(CXX_FOR_BUILD)
CXXFLAGS = $(CXXFLAGS_FOR_BUILD)
CXXCPP = $(CXXCPP_FOR_BUILD)
CPPFLAGS = $(CPPFLAGS_FOR_BUILD)
LDFLAGS = $(LDFLAGS_FOR_BUILD)
LIBS = $(LIBS_FOR_BUILD)

@CROSS_PREFIX@vtwrapper_SOURCES = \
	vt_wrapper.cc \
	vt_wrapper.h \
	$(top_srcdir)/util/installdirs.c \
	$(top_srcdir)/util/util.c

@CROSS_PREFIX@vtwrapper_CXXFLAGS = \
	$(HAVEF77CXXFLAGS) \
	$(HAVEF90CXXFLAGS) \
	$(HAVEMPICXXFLAGS) \
	$(HAVEFMPICXXFLAGS) \
	$(HAVETHREADSCXXFLAGS) \
	$(HAVEOMPCXXFLAGS)

CLEANFILES = $(CROSSPREFIXCLEANFILES)

EXTRA_DIST = TAUINST.SPEC

