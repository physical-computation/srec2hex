all:
	cc -Wall -Werror -o srec2hex srec2hex.c

install: all
	cp srec2hex /usr/local/bin/

clean:
	rm -f srec2hex
