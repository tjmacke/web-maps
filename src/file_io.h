#ifndef	_FILE_IO_H_
#define	_FILE_IO_H_

int
FIO_read_be_int4(FILE *, int *);

void
FIO_write_be_int4(FILE *, int);

int
FIO_read_le_int4(FILE *, int *);

void
FIO_write_le_int4(FILE *, int);

int
FIO_read_le_double(FILE *, double *);

int
FIO_write_le_double(FILE *, double);

#endif
