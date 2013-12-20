include ../../common.mk

L += $(MYSQLLIBS) -lm -lpthread

MYLIBDIR = ${KENT_ROOT}/lib/${MACHTYPE}
MYCANCERLIBDIR = ../../lib/${MACHTYPE}
MYSQLITELIBDIR = /data/home/cszeto/bin/sqliteInterface/lib
MYLIBS =  $(MYCANCERLIBDIR)/jkhmap.a $(MYLIBDIR)/jkhgap.a $(MYLIBDIR)/jkweb.a ${MYSQLITELIBDIR}/libsqlite3.a

Gplusplus=g++

A = hgClassifications
O = hgClassifications.o ../lib/populateClassificationTables.o ../lib/classificationTables.o ../lib/MLmatrix.o ../lib/MLcrossValidation.o ../lib/MLio.o ../lib/MLreadConfig.o ../lib/MLfilters.o ../lib/MLextractCoefficients.o
H = hgClassifications.h ../inc/populateClassificationTables.h ../inc/classificationTables.h ../inc/MLmatrix.h ../inc/MLcrossValidation.h ../inc/MLio.h ../inc/MLreadConfig.h ../inc/MLfilters.h ../inc/MLextractCoefficients.h

A1 = loadClassificationResults
O1 = \
	loadClassificationResults.o \
	../lib/classificationTables.o \
	../lib/populateClassificationTables.o \
	../lib/sqlite_populateClassificationTables.o \
	../lib/MLmatrix.o \
	../lib/MLcrossValidation.o \
	../lib/MLreadConfig.o \
	../lib/MLio.o \
	../lib/MLextractCoefficients.o \
	../lib/MLfilters.o
H1 = \
    ../inc/classificationTables.h \
    ../inc/populateClassificationTables.h \
	../inc/sqlite_populateClassificationTables.h \
    ../inc/MLmatrix.h \
    ../inc/MLcrossValidation.h \
    ../inc/MLreadConfig.h \
    ../inc/MLio.h \
    ../inc/MLextractCoefficients.h \
    ../inc/MLfilters.h


HGFRONTEND = /usr/local/apache/htdocs/hgClassifications

include ${KENT_ROOT}/inc/cgi_build_rules.mk

my::
	rsync -a --exclude=CVS/ --delete frontend/* ${HGFRONTEND}-${USER}/

alpha::
	rsync -rlD --exclude=CVS/ --delete frontend/* ${HGFRONTEND}/ 

compile: $O ${O2}
	${Gplusplus} ${COPT} $O ${MYLIBS} ${L}
	mv ${AOUT} $A${EXE}

loadClassificationResults: $(O1) ${H1} ${MYLIBS}
	${Gplusplus} ${COPT} ${O1} ${MYLIBS} ${L}
	mv ${AOUT} ${A1}${EXE}

all: ${A} ${A1}

clean:: 
	rm -f ${O1} ${O} ${O2} ${A1}
