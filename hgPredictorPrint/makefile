include ../../common.mk

L += $(MYSQLLIBS) -lm -lpthread

MYLIBDIR = ${KENT_ROOT}/lib/${MACHTYPE}
MYCANCERLIBDIR = ../../lib/${MACHTYPE}
MYSQLITELIBDIR = /data/home/cszeto/bin/sqliteInterface/lib
MYLIBS =  $(MYCANCERLIBDIR)/jkhmap.a $(MYLIBDIR)/jkhgap.a $(MYLIBDIR)/jkweb.a ${MYSQLITELIBDIR}/libsqlite3.a

#Gplusplus=g++

A = hgPredictorPrint
O = \
	hgPredictorPrint.o \
	../lib/populateClassificationTables.o \
	../lib/classificationTables.o \
	../lib/MLmatrix.o \
	../lib/MLcrossValidation.o \
	../lib/MLio.o \
	../lib/MLreadConfig.o \
	../lib/MLfilters.o \
	../lib/MLextractCoefficients.o
H = \
	hgPredictorPrint.h \
	../inc/populateClassificationTables.h \
	../inc/classificationTables.h \
	../inc/MLmatrix.h \
	../inc/MLcrossValidation.h \
	../inc/MLio.h \
	../inc/MLreadConfig.h \
	../inc/MLfilters.h \
	../inc/MLextractCoefficients.h

A1 = MLbuildBackgroundTables
O1 = \
	MLbuildBackgroundTables.o \
	../lib/MLmatrix.o \
	../lib/MLcrossValidation.o \
	../lib/MLreadConfig.o \
	../lib/MLio.o \
	../lib/MLextractCoefficients.o \
	../lib/MLfilters.o \
	../lib/MLsignificance.o
H1 = \
	../inc/MLmatrix.h \
	../inc/MLcrossValidation.h \
	../inc/MLreadConfig.h \
	../inc/MLio.h \
	../inc/MLextractCoefficients.h \
	../inc/MLfilters.h \
	../inc/MLsignificance.h

A2 = MLrunBackgroundAnalysis
O2 = \
	MLrunBackgroundAnalysis.o \
	../lib/MLmatrix.o \
	../lib/MLcrossValidation.o \
	../lib/MLreadConfig.o \
	../lib/MLio.o \
	../lib/MLextractCoefficients.o \
	../lib/MLfilters.o \
	../lib/MLsignificance.o
H2 = \
	../inc/MLmatrix.h \
	../inc/MLcrossValidation.h \
	../inc/MLreadConfig.h \
	../inc/MLio.h \
	../inc/MLextractCoefficients.h \
	../inc/MLfilters.h \
	../inc/MLsignificance.h

A3 = loadBackgroundResults
O3 = \
    loadBackgroundResults.o \
    ../lib/classificationTables.o \
    ../lib/populateClassificationTables.o \
    ../lib/sqlite_populateClassificationTables.o \
    ../lib/MLmatrix.o \
    ../lib/MLcrossValidation.o \
    ../lib/MLreadConfig.o \
    ../lib/MLio.o \
    ../lib/MLextractCoefficients.o \
    ../lib/MLfilters.o
H3 = \
    ../inc/classificationTables.h \
    ../inc/populateClassificationTables.h \
    ../inc/sqlite_populateClassificationTables.h \
    ../inc/MLmatrix.h \
    ../inc/MLcrossValidation.h \
    ../inc/MLreadConfig.h \
    ../inc/MLio.h \
    ../inc/MLextractCoefficients.h \
    ../inc/MLfilters.h

HGFRONTEND = /usr/local/apache/htdocs/hgPredictorPrint

include ${KENT_ROOT}/inc/cgi_build_rules.mk

my::
	rsync -a --exclude=CVS/ --delete frontend/* ${HGFRONTEND}-${USER}/

alpha::
	rsync -rlD --exclude=CVS/ --delete frontend/* ${HGFRONTEND}/ 

compile: $O ${O2}
	${CC} ${COPT} $O ${MYLIBS} ${L}
	mv ${AOUT} $A${EXE}

hgPredictorPrint: $(O) ${H} ${MYLIBS}
	${CC} ${COPT} ${O} ${MYLIBS} ${L}
	mv ${AOUT} ${A}${EXE}

MLbuildBackgroundTables: $(O1) ${H1} ${MYLIBS}
	${CC} ${COPT} ${O1} ${MYLIBS} ${L}
	mv ${AOUT} ${BINDIR}/${A1}${EXE}

MLrunBackgroundAnalysis: $(O2) ${H2} ${MYLIBS}
	${CC} ${COPT} ${O2} ${MYLIBS} ${L}
	mv ${AOUT} ${BINDIR}/${A2}${EXE}

loadBackgroundResults: $(O3) ${H3} ${MYLIBS}
	${CC} ${COPT} ${O3} ${MYLIBS} ${L}
	mv ${AOUT} ${A3}${EXE}

all: ${A} ${A1} ${A2} ${A3}
	mv ${A1}${EXE} ${BINDIR}/${A1}${EXE}
	mv ${A2}${EXE} ${BINDIR}/${A2}${EXE}

clean:: 
	rm -f ${O} ${O1} ${O2} ${O3} ${A} ${BINDIR}/${A1}${EXE} ${BINDIR}/${A2}${EXE} ${A3}${EXE}
