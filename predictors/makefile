include ../../common.mk

L += $(MYSQLLIBS) -lm -lpthread

ifeq ($(HOSTNAME),tcga1)
    CLUSTERROOT=/inside/grotto/users
	MYSQLITELIBDIR = /inside/home/cszeto/bin/sqliteInterface/lib
else
    CLUSTERROOT=/data/home/common/topmodel_tools
	MYSQLITELIBDIR = /data/home/cszeto/bin/sqliteInterface/lib
endif

MYLIBDIR = ${KENT_ROOT}/lib/${MACHTYPE}
MYCANCERLIBDIR = ../../lib/${MACHTYPE}
MYLIBS = $(MYCANCERLIBDIR)/jkhmap.a $(MYLIBDIR)/jkhgap.a $(MYLIBDIR)/jkweb.a ${MYSQLITELIBDIR}/libsqlite3.a

A = MLsetup
O = MLsetup.o ../lib/MLmatrix.o ../lib/MLcrossValidation.o ../lib/MLreadConfig.o ../lib/MLio.o ../lib/MLextractCoefficients.o ../lib/MLfilters.o
H = ../inc/MLmatrix.h ../inc/MLcrossValidation.h ../inc/MLreadConfig.h ../inc/MLio.h ../inc/MLextractCoefficients.h ../inc/MLfilters.h

A1 = MLwriteResultsToRa
O1 = MLwriteResultsToRa.o ../lib/MLmatrix.o ../lib/MLcrossValidation.o ../lib/MLreadConfig.o ../lib/MLio.o ../lib/MLextractCoefficients.o ../lib/MLfilters.o
H1 = ../inc/MLmatrix.h ../inc/MLcrossValidation.h ../inc/MLreadConfig.h ../inc/MLio.h ../inc/MLextractCoefficients.h ../inc/MLfilters.h

A2 = MLreportFolds
O2 = MLreportFolds.o ../lib/MLio.o ../lib/MLmatrix.o ../lib/MLreadConfig.o ../lib/MLcrossValidation.o ../lib/MLextractCoefficients.o ../lib/MLfilters.o
H2 = ../inc/MLmatrix.h ../inc/MLio.h ../inc/MLreadConfig.h ../inc/MLcrossValidation.h ../inc/MLextractCoefficients.h ../inc/MLfilters.h

A3 = MLpermute
O3 = MLpermute.o ../lib/MLmatrix.o ../lib/MLcrossValidation.o ../lib/MLreadConfig.o ../lib/MLio.o ../lib/MLextractCoefficients.o ../lib/MLfilters.o
H3 = ../inc/MLmatrix.h ../inc/MLcrossValidation.h ../inc/MLreadConfig.h ../inc/MLio.h ../inc/MLextractCoefficients.h ../inc/MLfilters.h

A4 = MLcompareKOs
O4 = MLcompareKOs.o ../lib/MLmatrix.o ../lib/MLcrossValidation.o ../lib/MLreadConfig.o ../lib/MLio.o ../lib/MLextractCoefficients.o ../lib/MLfilters.o
H4 = ../inc/MLmatrix.h ../inc/MLcrossValidation.h ../inc/MLreadConfig.h ../inc/MLio.h ../inc/MLextractCoefficients.h ../inc/MLfilters.h

A5 = MLbatchSetup
O5 = MLbatchSetup.o ../lib/MLmatrix.o ../lib/MLcrossValidation.o ../lib/MLreadConfig.o ../lib/MLio.o ../lib/MLextractCoefficients.o ../lib/MLfilters.o
H5 = ../inc/MLmatrix.h ../inc/MLcrossValidation.h ../inc/MLreadConfig.h ../inc/MLio.h ../inc/MLextractCoefficients.h ../inc/MLfilters.h

A6 = MLclassify
O6 = MLclassify.o ../lib/MLmatrix.o ../lib/MLcrossValidation.o ../lib/MLreadConfig.o ../lib/MLio.o ../lib/MLextractCoefficients.o ../lib/MLfilters.o
H6 = ../inc/MLmatrix.h ../inc/MLcrossValidation.h ../inc/MLreadConfig.h ../inc/MLio.h ../inc/MLextractCoefficients.h ../inc/MLfilters.h

A7 = MLgetLinearCoefficients
O7 = MLgetLinearCoefficients.o ../lib/MLmatrix.o ../lib/MLcrossValidation.o ../lib/MLreadConfig.o ../lib/MLio.o ../lib/MLextractCoefficients.o ../lib/MLfilters.o
H7 = ../inc/MLmatrix.h ../inc/MLcrossValidation.h ../inc/MLreadConfig.h ../inc/MLio.h ../inc/MLextractCoefficients.h ../inc/MLfilters.h

A8 = MLfive3batchsetup
O8 = MLfive3batchsetup.o ../lib/MLmatrix.o ../lib/MLcrossValidation.o ../lib/MLreadConfig.o ../lib/MLio.o ../lib/MLextractCoefficients.o ../lib/MLfilters.o
H8 = ../inc/MLmatrix.h ../inc/MLcrossValidation.h ../inc/MLreadConfig.h ../inc/MLio.h ../inc/MLextractCoefficients.h ../inc/MLfilters.h

A9 = tmp
O9 = tmp.o ../lib/MLmatrix.o ../lib/MLcrossValidation.o ../lib/MLreadConfig.o ../lib/MLio.o ../lib/MLextractCoefficients.o ../lib/MLfilters.o ../lib/MLsignificance.o
H9 = ../inc/MLmatrix.h ../inc/MLcrossValidation.h ../inc/MLreadConfig.h ../inc/MLio.h ../inc/MLextractCoefficients.h ../inc/MLfilters.h ../inc/MLsignificance.h

all: ${A} ${A1} ${A2} ${A3} ${A4} ${A5} ${A6} ${A7} ${A8}

MLsetup: $O $H ${MYLIBS}
	${CC} ${COPT} ${CFLAGS} $O ${MYLIBS} ${L}
	mv ${AOUT} ${BINDIR}/${A}${EXE}

MLwriteResultsToRa: ${O1} ${H1} ${MYLIBS}
	${CC} ${COPT} ${CFLAGS} ${O1} ${MYLIBS} ${L}
	mv ${AOUT} ${BINDIR}/${A1}${EXE}

MLreportFolds: ${O2} ${H2} ${MYLIBS}
	${CC} ${COPT} ${CFLAGS} ${O2} ${MYLIBS} ${L}
	mv ${AOUT} ${BINDIR}/${A2}${EXE}

MLpermute: ${O3} ${H3} ${MYLIBS}
	${CC} ${COPT} ${CFLAGS} ${O3} ${MYLIBS} ${L}
	mv ${AOUT} ${BINDIR}/${A3}${EXE}

MLcompareKOs: ${O4} ${H4} ${MYLIBS}
	${CC} ${COPT} ${CFLAGS} ${O4} ${MYLIBS} ${L}
	mv ${AOUT} ${BINDIR}/${A4}${EXE}

MLbatchSetup: ${O5} ${H5} ${MYLIBS}
	${CC} ${COPT} ${CFLAGS} ${O5} ${MYLIBS} ${L}
	mv ${AOUT} ${BINDIR}/${A5}${EXE}

MLfive3batchsetup: ${O8} ${H8} ${MYLIBS}
	${CC} ${COPT} ${CFLAGS} ${O8} ${MYLIBS} ${L}
	mv ${AOUT} ${BINDIR}/${A8}${EXE}

MLclassify: ${O6} ${H6} ${MYLIBS}
	${CC} ${COPT} ${CFLAGS} ${O6} ${MYLIBS} ${L}
	mv ${AOUT} ${BINDIR}/${A6}${EXE}

MLgetLinearCoefficients: ${O7} ${H7} ${MYLIBS}
	${CC} ${COPT} ${CFLAGS} ${O7} ${MYLIBS} ${L}
	mv ${AOUT} ${BINDIR}/${A7}${EXE}

tmp: ${O9} ${H9} ${MYLIBS}
	${CC} ${COPT} ${CFLAGS} ${O9} ${MYLIBS} ${L}
	mv ${AOUT} ${A9}${EXE}

push:
	cp ${BINDIR}/${A}${EXE} ${CLUSTERROOT}/${A}
	cp ${BINDIR}/${A1}${EXE} ${CLUSTERROOT}/${A1}
	cp ${BINDIR}/${A2}${EXE} ${CLUSTERROOT}/${A2}
	cp ${BINDIR}/${A3}${EXE} ${CLUSTERROOT}/${A3}
	cp ${BINDIR}/${A4}${EXE} ${CLUSTERROOT}/${A4}
	cp ${BINDIR}/${A6}${EXE} ${CLUSTERROOT}/${A6}
	
clean:: 
	rm -f ${O} ${O1} ${O2} ${O3} ${O4} ${O5} ${O6} ${O7} ${O8} ${BINDIR}/${A}${EXE} ${BINDIR}/${A1}${EXE} ${BINDIR}/${A2}${EXE} ${BINDIR}/${A3}${EXE} ${BINDIR}/${A4}${EXE} ${BINDIR}/${A5}${EXE} ${BINDIR}/${A6}${EXE} ${BINDIR}/${A7}${EXE} ${BINDIR}/${A8}${EXE}

distclean::
	rm -f ${CLUSTERROOT}/${A1} ${CLUSTERROOT}/${A2} ${CLUSTERROOT}/${A} ${CLUSTERROOT}/${A3} ${CLUSTERROOT}/${A4} ${CLUSTERROOT}/${A5}
