CC=gcc

# uncomment to compile in 32bits mode (require gcc-*-multilib packages
# on Debian/Ubuntu)
#HOST32= -m32

CFLAGS+= $(HOST32) -Wall -Werror -std=c99 -g -D_GNU_SOURCE
CFLAGS+= -DDEBUG
CFLAGS += -DMEMORY_SIZE=128000
# pour tester avec ls
CFLAGS+= -fPIC
LDFLAGS= $(HOST32)
TESTS+=test_init
PROGRAMS= memshell $(TESTS)
tests=$(wildcard *.t)
tests_bis=$(tests:.t=.test)

.PHONY: clean all test_ls

all: $(PROGRAMS)
	for file in $(TESTS);do ./$$file; done

%.o: %.c
	$(CC) -c $(CFLAGS) -MMD -MF .$@.deps -o $@ $<

# dépendences des binaires
$(PROGRAMS) libmalloc.so: %: mem.o common.o

-include $(wildcard .*.deps)

# seconde partie du sujet
libmalloc.so: malloc_stub.o
	$(CC) -shared -Wl,-soname,$@ $^ -o $@

test_ls: libmalloc.so
	LD_PRELOAD=./libmalloc.so ls

tests: rm_tests $(tests_bis)
	cat result_tests

%.test: %.t
	echo "\n\nTest $* : " >> result_tests & ./memshell < $< >> result_tests

rm_tests:
	rm -rf result_tests
	

# nettoyage
clean:
	$(RM) *.o $(PROGRAMS) libmalloc.so .*.deps result_tests
