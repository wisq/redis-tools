all:
	$(MAKE) -C redistamp $@
	cd tools && bundle install --path=gems

clean:
	$(MAKE) -C redistamp $@
