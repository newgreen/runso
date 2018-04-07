.PHONY: all clean

target = test.so

all: $(target)

clean:
	$(RM) *.o *.so *.exe

test.so: test.c
	$(CC) -fPIC -shared -g -rdynamic $^ -lrunso -o $@
