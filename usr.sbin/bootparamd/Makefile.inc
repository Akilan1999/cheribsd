#	@(#)Makefile.inc	5.1 (Berkeley) 5/11/90

NO_WCAST_ALIGN=
CWARNFLAGS.clang+=	-Wno-incompatible-pointer-types-discards-qualifiers
CWARNFLAGS.gcc+=	-Wno-error=discarded-qualifiers

.include "../Makefile.inc"
