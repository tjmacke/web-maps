CC=	cc
CFLAGS=	-g

PROGRAMS=	\
	shp_dump	\
	shp_to_geojson

all:	$(PROGRAMS)

shp_dump:	shp_dump.o log.o args.o file_io.o shape.o dbase.o
	$(CC) $(CFLAGS) -o shp_dump shp_dump.o log.o args.o file_io.o shape.o dbase.o

shp_to_geojson:	shp_to_geojson.o log.o args.o
	$(CC) $(CFLAGS) -o shp_to_geojson shp_to_geojson.o log.o args.o

shp_dump.o:	log.h args.h shape.h dbase.h

shp_to_geojson.o:	log.h args.h

file_io.o:	log.h

shape.o:	log.h shape.h

dbase.o:	log.h dbase.h

clean:
	rm -rf $(PROGRAMS) *.o core core.* junk.*
