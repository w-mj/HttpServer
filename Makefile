all: server

sources = server.c Socket.c http.c

CPPFLAGS += -lpthread -g

server: server.o Socket.o http.o
	$(CC) $(CPPFLAGS) $^ -o $@

.PHONY: clean
-include $(sources:.c=.d)

%.d: %.c
	set -e; \
	rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	-rm -f server *.o *.d