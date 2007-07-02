CFLAGS = -I ../../varnish-cache/include -O2 -Wall -pedantic -std=c99 -D_GNU_SOURCE
LDFLAGS = -L /opt/varnish/lib -lvarnishapi

all: check_varnish

clean:
	-rm check_varnish