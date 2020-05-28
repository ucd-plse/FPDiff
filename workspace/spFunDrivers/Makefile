CC = gcc
CFLAGS = -w
LDFLAGS = -L/usr/local/lib/gsl/gsl-library/lib -lgsl -lgslcblas -lm
INCLUDE = -I/usr/local/lib/gsl/gsl-library/include

DEPS =

sourcefiles=$(wildcard *.c)
targets=$(patsubst %.c,%.so,$(sourcefiles))

.PHONY: clean all

all : clean ${targets}

%.so : %.c $(DEPS)
	$(CC) $(INCLUDE) $(CFLAGS) -c -fpic $< -o temp.o $(LDFLAGS)
	$(CC) $(INCLUDE) $(CFLAGS) -shared temp.o -o $@ $(LDFLAGS)
	rm temp.o

% : %.c $(DEPS)
	$(CC) $(INCLUDE) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean :
	rm -rf $(targets)
