
SUBDIRS=$(wildcard phase2[a-d])

HDRS=phase2.h phase2Int.h

.PHONY: $(SUBDIRS) all clean install subdirs

all: $(SUBDIRS)

subdirs: $(SUBDIRS)

clean: $(SUBDIRS)
	rm -f p3/*.o term*.out

install: $(SUBDIRS)
	install $(HDRS) ~/include

tests: $(SUBDIRS)

tar:
	(cd ..; gnutar cvzf ~/Downloads/phase2-starter.tgz --exclude=.git --exclude="*.dSYM" phase2-starter)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)