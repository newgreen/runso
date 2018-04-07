.PHONY: all clean install uninstall package

CFLAGS += -g -rdynamic

target = runso.h librunso.so runso.exe rs_server.exe
script = ../script/rsinj.sh
package = runso.tgz

all: $(target)

clean:
	$(RM) *.o *.so *.exe
	$(RM) -r release $(package)

install: $(target) $(script)
	@install -v runso.h /usr/local/include
	@install -v librunso.so /usr/local/lib
	@install -v runso.exe /usr/local/bin
	@install -v rs_server.exe /usr/local/bin
	@install -v ../script/rsinj.sh /usr/local/bin

uninstall:
	-$(RM) /usr/local/include/runso.h
	-$(RM) /usr/local/lib/librunso.so
	-$(RM) /usr/local/bin/runso.exe
	-$(RM) /usr/local/bin/rs_server.exe
	-$(RM) /usr/local/bin/rsinj.sh

package: $(target) $(script)
	@mkdir release/
	@cp $^ release/
	@echo                                       \
		"#!/bin/bash\n\n"                       \
		"install -v *.h /usr/local/include\n"   \
		"install -v *.so /usr/local/lib\n"      \
		"install -v *.exe /usr/local/bin\n"     \
		"install -v rsinj.sh /usr/local/bin\n"  \
    > release/install.sh && chmod +x release/install.sh
	@echo                                       \
		"#!/bin/bash\n\n"                       \
		"rm -v /usr/local/include/runso.h\n"    \
		"rm -v /usr/local/lib/librunso.so\n"    \
		"rm -v /usr/local/bin/runso.exe\n"      \
		"rm -v /usr/local/bin/rs_server.exe\n"  \
		"rm -v /usr/local/bin/rsinj.sh\n"       \
    > release/uninstall.sh && chmod +x release/uninstall.sh
	@tar -cvzf $(package) release/

%.exe: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(filter %.c,$^) -o $@

runso.exe: rs_client.c rs_cs.h rs_msg.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(filter %.c,$^) -L. -lrunso -o $@

librunso.so: rs_msg.c rs_msg.h rs_cs.c rs_cs.h rs_runso.c rs_runso.h
	$(CC) -fPIC -shared $(CFLAGS) $(LDFLAGS) $(filter %.c,$^) -pthread -ldl -o $@
