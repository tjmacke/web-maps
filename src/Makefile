CC=	cc
CFLAGS=	-g

PROGRAMS=	\
	shp_dump

all:	$(PROGRAMS)

shp_dump:	shp_dump.o log.o args.o
	$(CC) $(CFLAGS) -o shp_dump shp_dump.o log.o args.o

shp_dump.o:	log.h args.h

clean:
	rm -rf $(PROGRAMS) *.o core core.* junk.*
