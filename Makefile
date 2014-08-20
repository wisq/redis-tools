PREFIX=/usr/local/redis-tools

all:
	$(MAKE) -C redistamp $@
	cd tools && rm -f .bundle/config && bundle install --frozen --path=gems

clean:
	$(MAKE) -C redistamp $@

install:
	mkdir -vp $(PREFIX)/bin $(PREFIX)/lib $(PREFIX)/example $(PREFIX)/gems $(PREFIX)/.bundle
	rsync -rt --delete-after tools/gems/ $(PREFIX)/gems/
	install -m 644 tools/Gemfile tools/Gemfile.lock $(PREFIX)
	install -m 644 tools/.bundle/config $(PREFIX)/.bundle
	cd $(PREFIX) && bundle check --path=$(PREFIX)/gems
	install -m 644 tools/lib/*.rb $(PREFIX)/lib
	install -m 755 redistamp/bin/redistamp tools/bin/redis-* $(PREFIX)/bin
	install -m 755 tools/example/redis-* $(PREFIX)/example
	ln -nsf $(PREFIX)/bin/redis* /usr/local/bin
