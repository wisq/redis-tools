PREFIX=/usr/local/redis-tools

all:
	$(MAKE) -C redistamp $@
	cd tools && bundle install --path=gems

clean:
	$(MAKE) -C redistamp $@
	rm -rf tools/.bundle

install:
	mkdir -vp $(PREFIX)/bin $(PREFIX)/lib $(PREFIX)/gems
	rsync -rt --delete-after tools/gems $(PREFIX)/gems
	install -m 644 tools/lib/*.rb $(PREFIX)/lib
	install -m 644 tools/Gemfile tools/Gemfile.lock $(PREFIX)
	install -m 755 redistamp/bin/redistamp tools/bin/*.rb $(PREFIX)/bin
