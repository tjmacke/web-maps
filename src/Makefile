CC=	cc
CFLAGS=	-g

PROGRAMS=	\
	shp_dump	\
	shp_to_geojson

all:	$(PROGRAMS)

shp_dump:	shp_dump.o log.o args.o shape.o
	$(CC) $(CFLAGS) -o shp_dump shp_dump.o log.o args.o shape.o

shp_to_geojson:	shp_to_geojson.o log.o args.o
	$(CC) $(CFLAGS) -o shp_to_geojson shp_to_geojson.o log.o args.o

shp_dump.o:	log.h args.h

shp_to_geojson.o:	log.h args.h

shape.o:	log.h shape.h

clean:
	rm -rf $(PROGRAMS) *.o core core.* junk.*
