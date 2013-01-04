PROGRAM=shell-fm.1

.PHONY	: clean

manual	: shell-fm.1.gz

shell-fm.1.gz : $(PROGRAM)
	gzip -9 -c $< > $@

clean	: $(PROGRAM)
	rm -f $<.gz
