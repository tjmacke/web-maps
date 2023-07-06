UNAME_S := $(shell uname -s)

CC=	cc
ifeq ($(UNAME_S),Darwin)
  CFLAGS=       -g -I/usr/local/ssl/include -I../third-party
else
  CFLAGS=       -g -I../third-party
endif

CSV_LIBS=-L/usr/local/lib -lcsv
GPC_LIBS=../third-party/gpc.o
SSL_LIBS=-L/usr/local/ssl/lib -lcrypto

#tm PROGRAMS=	\
#tm 	csv2tsv			\
#tm 	dbase_to_sqlite		\
#tm 	find_addrs_in_rect	\
#tm 	get_shp_recs		\
#tm 	gpc_driver		\
#tm 	index_shp_file		\
#tm 	make_shx_from_shp_file	\
#tm 	shp_dump		\
#tm 	shp_get			\
#tm 	shp_to_geojson

PROGRAMS=	\
	dbase_to_sqlite		\
	find_addrs_in_rect	\
	get_shp_recs		\
	index_shp_file		\
	make_shx_from_shp_file	\
	shp_dump		\
	shp_get			\
	shp_to_geojson

all:	$(PROGRAMS)

csv2tsv:	csv2tsv.o log.o args.o
	$(CC) $(CFLAGS) -o csv2tsv csv2tsv.o log.o args.o $(CSV_LIBS)

dbase_to_sqlite:	dbase_to_sqlite.o log.o args.o dbase.o
	$(CC) $(CFLAGS) -o dbase_to_sqlite dbase_to_sqlite.o log.o args.o dbase.o

find_addrs_in_rect:	find_addrs_in_rect.o log.o args.o adata.o
	$(CC) $(CFLAGS) -o find_addrs_in_rect find_addrs_in_rect.o log.o args.o adata.o -lm

get_shp_recs:	get_shp_recs.o log.o args.o file_io.o shape.o
	$(CC) $(CFLAGS) -o get_shp_recs get_shp_recs.o log.o args.o file_io.o shape.o

gpc_driver:	gpc_driver.o log.o args.o
	$(CC) $(CFLAGS) -o gpc_driver gpc_driver.o log.o args.o $(GPC_LIBS)

index_shp_file:	index_shp_file.o log.o args.o shape.o
	$(CC) $(CFLAGS) -o index_shp_file index_shp_file.o log.o args.o file_io.o shape.o

make_shx_from_shp_file:	make_shx_from_shp_file.o log.o args.o shape.o file_io.o
	$(CC) $(CFLAGS) -o make_shx_from_shp_file make_shx_from_shp_file.o log.o args.o shape.o file_io.o

shp_dump:	shp_dump.o log.o args.o file_io.o shape.o dbase.o
	$(CC) $(CFLAGS) -o shp_dump shp_dump.o log.o args.o file_io.o shape.o dbase.o

shp_get:	shp_get.o log.o args.o file_io.o shape.o s2g_input.o props.o fmap.o split.o
	$(CC) $(CFLAGS) -o shp_get shp_get.o log.o args.o file_io.o shape.o s2g_input.o props.o fmap.o split.o $(SSL_LIBS)

shp_to_geojson:	shp_to_geojson.o log.o args.o file_io.o shape.o s2g_input.o props.o fmap.o split.o
	$(CC) $(CFLAGS) -o shp_to_geojson shp_to_geojson.o log.o args.o file_io.o shape.o s2g_input.o props.o fmap.o split.o $(SSL_LIBS)

csv2tsv.o:	log.h args.h

dbase_to_sqlite.o:	log.h args.h dbase.h

find_addrs_in_rect.o:	log.h args.h shape.h adata.h

get_shp_recs.o:	log.h args.h shape.h

gpc_driver.o:	log.h args.h

index_shp_file.o:	log.h args.h index.h shape.h

make_shx_from_shp_file.o:	log.h args.h shape.h file_io.h

s2g_input.o:	log.h shape.h s2g_input.h

shp_dump.o:	log.h args.h shape.h dbase.h

shp_get.o:	log.h args.h props.h shape.h s2g_input.h

shp_to_geojson.o:	log.h args.h props.h shape.h s2g_input.h

file_io.o:	log.h

fmap.o:		log.h fmap.h

shape.o:	log.h shape.h

dbase.o:	log.h dbase.h

props.o:	log.h props.h

install:	all
	mv $(PROGRAMS) ../bin

clean:
	rm -rf $(PROGRAMS) *.o core core.* junk.*
