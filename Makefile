all:
	$(MAKE) -C tws
	$(MAKE) -C pibl

test-unit:
	$(MAKE) -C tws test
	$(MAKE) -C pibl test

clean:
	$(MAKE) -C tws clean
	$(MAKE) -C pibl clean