CC=	cc
CFLAGS=	-g

PROGRAMS=	\
	dbase_to_sqlite	\
	get_shp_recs	\
	index_shp_file	\
	shp_dump	\
	shp_to_geojson

all:	$(PROGRAMS)

dbase_to_sqlite:	dbase_to_sqlite.o log.o args.o dbase.o
	$(CC) $(CFLAGS) -o dbase_to_sqlite dbase_to_sqlite.o log.o args.o dbase.o

get_shp_recs:	get_shp_recs.o log.o args.o file_io.o shape.o
	$(CC) $(CFLAGS) -o get_shp_recs get_shp_recs.o log.o args.o file_io.o shape.o

index_shp_file:	index_shp_file.o log.o args.o shape.o
	$(CC) $(CFLAGS) -o index_shp_file index_shp_file.o log.o args.o file_io.o shape.o

shp_dump:	shp_dump.o log.o args.o file_io.o shape.o dbase.o
	$(CC) $(CFLAGS) -o shp_dump shp_dump.o log.o args.o file_io.o shape.o dbase.o

shp_to_geojson:	shp_to_geojson.o log.o args.o file_io.o shape.o props.o fmap.o split.o
	$(CC) $(CFLAGS) -o shp_to_geojson shp_to_geojson.o log.o args.o file_io.o shape.o props.o fmap.o split.o

dbase_to_sqlite.o:	log.h args.h dbase.h

get_shp_recs.o:	log.h args.h shape.h

index_shp_file.o:	log.h args.h index.h shape.h

shp_dump.o:	log.h args.h shape.h dbase.h

shp_to_geojson.o:	log.h args.h props.h shape.h fmap.h

file_io.o:	log.h

fmap.o:		log.h fmap.h

shape.o:	log.h shape.h

dbase.o:	log.h dbase.h

props.o:	log.h props.h

install:	all
	mv $(PROGRAMS) ../bin

clean:
	rm -rf $(PROGRAMS) *.o core core.* junk.*
