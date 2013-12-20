include ../../../common.mk

L += $(MYSQLLIBS) -lm -lpthread

ifeq ($(HOSTNAME),tcga1)
    CLUSTERROOT=/inside/grotto/users
    MYSQLITELIBDIR = /inside/home/cszeto/bin/sqliteInterface/lib
else
    CLUSTERROOT=/data/home/common/topmodel_tools
    MYSQLITELIBDIR = /data/home/cszeto/bin/sqliteInterface/lib
endif
MYLIBDIR = ${KENT_ROOT}/lib/${MACHTYPE}
MYCANCERLIBDIR = ../../../lib/${MACHTYPE}
MYLIBS = $(MYCANCERLIBDIR)/jkhmap.a $(MYLIBDIR)/jkhgap.a $(MYLIBDIR)/jkweb.a ${MYSQLITELIBDIR}/libsqlite3.a

A1 = NMFpredictor_train
O1 = NMFpredictor_train.o NMF.o ../../lib/MLmatrix.o ../../lib/MLcrossValidation.o ../../lib/MLio.o ../../lib/MLreadConfig.o ../../lib/MLextractCoefficients.o ../../lib/MLfilters.o
H1 = NMF.h ../../inc/MLmatrix.h ../../inc/MLcrossValidation.h ../../inc/MLio.h ../../inc/MLreadConfig.h ../../inc/MLextractCoefficients.h ../../inc/MLfilters.h

A2 = NMFpredictor_classify
O2 = NMFpredictor_classify.o ../../lib/MLmatrix.o ../../lib/MLcrossValidation.o ../../lib/MLio.o ../../lib/MLreadConfig.o ../../lib/MLextractCoefficients.o ../../lib/MLfilters.o
H2 = ../../inc/MLmatrix.h ../../inc/MLcrossValidation.h ../../inc/MLio.h ../../inc/MLreadConfig.h ../../inc/MLextractCoefficients.h ../../inc/MLfilters.h

A3 = NMFpredictor_project
O3 = NMFpredictor_project.o NMF.o ../../lib/MLmatrix.o ../../lib/MLcrossValidation.o ../../lib/MLio.o ../../lib/MLreadConfig.o ../../lib/MLextractCoefficients.o ../../lib/MLfilters.o
H3 = NMF.h ../../inc/MLmatrix.h ../../inc/MLcrossValidation.h ../../inc/MLio.h ../../inc/MLreadConfig.h ../../inc/MLextractCoefficients.h ../../inc/MLfilters.h

all: ${A1} ${A2} ${A3}

NMFpredictor_train: ${O1} ${H1} ${MYLIBS}
	${CC} ${COPT} ${CFLAGS} ${O1} ${MYLIBS} ${L}
	mv ${AOUT} ${BINDIR}/${A1}${EXE}
	cp ${BINDIR}/${A1}${EXE} ${CLUSTERROOT}/${A1}${EXE}

NMFpredictor_classify: ${O2} ${H2} ${MYLIBS}
	${CC} ${COPT} ${CFLAGS} ${O2} ${MYLIBS} ${L}
	mv ${AOUT} ${BINDIR}/${A2}${EXE}
	cp ${BINDIR}/${A2}${EXE} ${CLUSTERROOT}/${A2}${EXE}

NMFpredictor_project: ${O3} ${H3} ${MYLIBS}
	${CC} ${COPT} ${CFLAGS} ${O3} ${MYLIBS} ${L}
	mv ${AOUT} ${BINDIR}/${A3}${EXE}
	cp ${BINDIR}/${A3}${EXE} ${CLUSTERROOT}/${A3}${EXE}

clean::
	rm -f ${A1} ${A2} ${A3} ${A4} ${O1} ${O2} ${O3} ${O4}
