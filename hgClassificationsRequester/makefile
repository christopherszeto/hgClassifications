include ../../common.mk

L += $(MYSQLLIBS) -lm -lpthread

MYLIBDIR = ${KENT_ROOT}/lib/${MACHTYPE}
MYCANCERLIBDIR = ../../lib/${MACHTYPE}
MYSQLITELIBDIR = /data/home/cszeto/bin/sqliteInterface/lib
MYLIBS =  $(MYCANCERLIBDIR)/jkhmap.a $(MYLIBDIR)/jkhgap.a $(MYLIBDIR)/jkweb.a ${MYSQLITELIBDIR}/libsqlite3.a

Gplusplus=g++

A = hgClassificationsRequester
O = hgClassificationsRequester.o ../lib/populateClassificationTables.o ../lib/classificationTables.o ../lib/MLmatrix.o ../lib/MLcrossValidation.o ../lib/MLio.o ../lib/MLreadConfig.o ../lib/MLfilters.o ../lib/MLextractCoefficients.o
H = hgClassificationsRequester.h ../inc/populateClassificationTables.h ../inc/classificationTables.h ../inc/MLmatrix.h ../inc/MLcrossValidation.h ../inc/MLio.h ../inc/MLreadConfig.h ../inc/MLfilters.h ../inc/MLextractCoefficients.h

HGFRONTEND = /usr/local/apache/htdocs/hgClassificationsRequester

include ${KENT_ROOT}/inc/cgi_build_rules.mk

my::
	rsync -a --exclude=CVS/ --delete frontend/* ${HGFRONTEND}-${USER}/

alpha::
	rsync -rlD --exclude=CVS/ --delete frontend/* ${HGFRONTEND}/ 

compile: $O
	${Gplusplus} ${COPT} $O ${MYLIBS} ${L}
	mv ${AOUT} $A${EXE}

all: $A

clean:: 
	rm -f $O
