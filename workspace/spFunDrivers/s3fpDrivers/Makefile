CC = gcc
CFLAGS = -w
LDFLAGS = -L../../../../../../lib/gsl/gsl-library/lib -lgsl -lgslcblas -lm
INCLUDE = -I../../../../../../lib/gsl/gsl-library/include

DEPS =

sourcefiles=$(wildcard *.c)
targets=$(patsubst %.c,%,$(sourcefiles))

.PHONY: clean all

all : clean ${targets}

% : %.c $(DEPS)
	$(CC) $(INCLUDE) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean :
	rm -rf $(targets)

classClean:
	rm -rf *_class