# The model checked with TLC: `make check` for the runtime as fixed, with
# and without an onError handler (Request.cfg, NoOnError.cfg), `make bugs`
# for one run per bug of the runtime as it was, each of which TLC has to
# find (check-bugs.sh), `make deep` for the larger bounds of Deep.cfg
# (half an hour), `make current` for all the bugs at once (Current.cfg,
# which fails on the first).
TLC = tlc -workers auto
export TLC

.PHONY: check bugs deep current clean

check: out/Request.log out/NoOnError.log

bugs: out/Request.log
	./check-bugs.sh

deep: out/Deep.log

current:
	mkdir -p out
	-$(TLC) -metadir out/Current -config Current.cfg Request.tla | tee out/Current.log

out/%.log: %.cfg Request.tla
	mkdir -p out
	$(TLC) -metadir out/$* -config $< Request.tla | tee $@

clean:
	rm -rf out
