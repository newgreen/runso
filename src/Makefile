.PHONY: all clean install uninstall

CFLAGS += -g -rdynamic

target = librunso.so runso.exe rs_server.exe

all: $(target)

clean:
	$(RM) *.o *.so *.exe

install: $(target)
	@install -v librunso.so /usr/local/lib
	@install -v runso.exe /usr/local/bin
	@install -v rs_server.exe /usr/local/bin

uninstall:
	-$(RM) /usr/local/lib/librunso.so
	-$(RM) /usr/local/bin/runso.exe

%.exe: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(filter %.c,$^) -o $@

runso.exe: rs_client.c rs_cs.h rs_msg.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(filter %.c,$^) -L. -lrunso -o $@

librunso.so: rs_msg.c rs_msg.h rs_cs.c rs_cs.h
	$(CC) -fPIC -shared $(CFLAGS) $(LDFLAGS) $(filter %.c,$^) -pthread -o $@
