#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	finger
SRCS=	finger.c lprint.c net.c sprint.c util.c
MAN=	finger.1 finger.conf.5

CFLAGS+=	-I${SRCTOP}/contrib/uthash/src

.include <bsd.prog.mk>
