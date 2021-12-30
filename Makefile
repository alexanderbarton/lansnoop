default: all
.PHONY: default

all clean depend:
	for D in events common deserializer snoop viewer ; do make --no-print-directory -C $$D $@ ; done
.PHONY: all clean depend
