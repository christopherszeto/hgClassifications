include ../../common.mk

L += $(MYSQLLIBS) -lm

MYLIBDIR = ${KENT_ROOT}/lib/${MACHTYPE}
MYCANCERLIBDIR = ../../lib/${MACHTYPE}
MYSQLITELIBDIR = /data/home/cszeto/bin/sqliteInterface/lib
MYLIBS = $(MYCANCERLIBDIR)/jkhmap.a $(MYLIBDIR)/jkhgap.a $(MYLIBDIR)/jkweb.a ${MYSQLITELIBDIR}/libsqlite3.a

O = MLcrossValidation.o MLextractCoefficients.o MLfilters.o MLio.o MLmatrix.o MLreadConfig.o classificationTables.o populateClassificationTables.o sqlite_populateClassificationTables.o

compile: ${O}

clean::
	rm -f $O

