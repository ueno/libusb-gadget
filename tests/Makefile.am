noinst_PROGRAMS = loopback loopback-test

loopback_SOURCES = loopback.c
loopback_LDADD = -L$(top_srcdir)/src -lusg -lpthread

loopback_test_SOURCES = loopback-test.c
loopback_test_LDADD = $(libusb_LIBS)
