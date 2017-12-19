CC=	cc
CFLAGS=	-g

PROGRAMS=	\
	dbase_to_sqlite	\
	shp_dump	\
	shp_to_geojson

all:	$(PROGRAMS)

dbase_to_sqlite:	dbase_to_sqlite.o log.o args.o dbase.o
	$(CC) $(CFLAGS) -o dbase_to_sqlite dbase_to_sqlite.o log.o args.o dbase.o

shp_dump:	shp_dump.o log.o args.o file_io.o shape.o dbase.o
	$(CC) $(CFLAGS) -o shp_dump shp_dump.o log.o args.o file_io.o shape.o dbase.o

shp_to_geojson:	shp_to_geojson.o log.o args.o file_io.o shape.o
	$(CC) $(CFLAGS) -o shp_to_geojson shp_to_geojson.o log.o args.o file_io.o shape.o

dbase_to_sqlite.o:	log.h args.h dbase.h

shp_dump.o:	log.h args.h shape.h dbase.h

shp_to_geojson.o:	log.h args.h shape.h

file_io.o:	log.h

shape.o:	log.h shape.h

dbase.o:	log.h dbase.h

install:	all
	mv $(PROGRAMS) ../bin

clean:
	rm -rf $(PROGRAMS) *.o core core.* junk.*
