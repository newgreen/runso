.PHONY: all clean install uninstall

CFLAGS += -g -rdynamic

target = rsinj.sh

all:

clean:

install: $(target)
	@install -v rsinj.sh /usr/local/bin

uninstall:
	-$(RM) /usr/local/bin/rsinj.sh
