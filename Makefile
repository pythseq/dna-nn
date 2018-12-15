CFLAGS=		-g -Wall -O2 -Wc++-compat #-Wextra
OBJS=		kautodiff.o kann.o dna-io.o
PROG=		gen-fq dna-cnn
LIBS=		-lm -lz -lpthread

.PHONY:all clean depend
.SUFFIXES:.c .o

.c.o:
		$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $< -o $@

all:$(PROG)

gen-fq:gen-fq.o
		$(CC) -o $@ $< $(LIBS)

dna-cnn:dna-cnn.o $(OBJS)
		$(CC) -o $@ $^ $(LIBS)

clean:
		rm -fr gmon.out *.o a.out $(PROG) *~ *.a *.dSYM

depend:
		(LC_ALL=C; export LC_ALL; makedepend -Y -- $(CFLAGS) $(CPPFLAGS) -- *.c)

# DO NOT DELETE

dna-cnn.o: kann.h kautodiff.h dna-io.h kseq.h
dna-io.o: dna-io.h kseq.h
gen-fq.o: ketopt.h kseq.h khash.h
kann.o: kann.h kautodiff.h
kautodiff.o: kautodiff.h