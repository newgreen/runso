.PHONY: all clean

CFLAGS  += -g -rdynamic -I../src
LDFLAGS += -L../src -lrunso

stub     = ut_stub.c
except   = ut_target.c
src      = $(filter-out $(stub) $(except),$(wildcard *.c))

target   = $(patsubst %.c,%_send.exe,$(src))
target  += $(patsubst %.c,%_recv.exe,$(src))
target  += ut_clrmsg.exe ut_cmd_recvloop.exe ut_target.exe ut_target_v1.exe

all: $(target)

clean:
	$(RM) *.o *.so *.exe

%_send.exe: %.c
	$(CC) -DRS_SEND $(CFLAGS) $^ $(stub) $(LDFLAGS) -o $@

%_recv.exe: %.c
	$(CC) -DRS_RECV $(CFLAGS) $^ $(stub) $(LDFLAGS) -o $@

%.exe: %.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

ut_cmd_recvloop.exe: ut_cmd.c
	$(CC) -DRS_RECV -DRS_RECV_LOOP $(CFLAGS) $^ $(stub) $(LDFLAGS) -o $@

ut_clrmsg.exe: ut_cmd.c
	$(CC) -DRS_CLRMSG $(CFLAGS) $^ $(stub) $(LDFLAGS) -o $@
