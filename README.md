# redis-tools

A small collection of tools to help monitor your Redis server.

## redistamp

Very tight client that connects to your server and stores the current date in a `redistamp` key every second.  When used in a replicated environment, this allows you to easily check for replication lag.

## Ruby tools

These use the `redis` and `hiredis` gems, and install these via the `bundler` gem.  By default, they install their gems into `tools/gems`; you can `cd` to tools and `bundle exec ./tool.rb` to run them.

Output is in JSON format, for easy integration with log monitoring solutions like Splunk.

### redis-slowlog

Monitors the Redis slow query log and outputs any new entries it sees.

### redis-usage

Traverses the Redis keyspace via `SCAN` and outputs guesstimates on memory usage.  It assumes that you use a colon-separated key hierarchy, e.g. "foo:bar:baz".

If any tree node gets too leafy, it will prune it and just keep stats rather than filling up memory with all the sub-key names; for example, if you have 100,000 keys like `a:*` and only 50 keys like `b:*`, you'll see the usage of each individual `b:*` key, but the `a:*` keys will be rolled up into a single size for `a`.  (This happens at any level except the root, so if you have too many unique root-level prefixes, you may use a lot of memory and get a lot of output.)

Since this puts a lot of load on Redis, it's meant to be run on a readonly slave rather than the master database, and will abort otherwise.
