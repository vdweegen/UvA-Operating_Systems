CC=gcc
CFLAGS=-Wall -ansi -O2 -lm -g
WORST=mem_test2-64.o worst-fit.c
FIRST=mem_test2-64.o first-fit.c
PROGWORST=worst-fit
PROGFIRST=first-fit

worst: $(FILES)
	 $(CC) $(CFLAGS) -o  $(PROGWORST) $(WORST)
	 
first: $(FILES)
	 $(CC) $(CFLAGS) -o  $(PROGFIRST) $(FIRST)