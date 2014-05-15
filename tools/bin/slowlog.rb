#!/usr/bin/ruby

REDIS_TOOLS = File.realpath(File.dirname(__FILE__) + "/..")
ENV['BUNDLE_GEMFILE'] ||= REDIS_TOOLS + "/Gemfile"
ENV['BUNDLE_PATH']    ||= REDIS_TOOLS + "/gems"
require 'rubygems'
require 'bundler/setup'

$LOAD_PATH << REDIS_TOOLS + "/lib"
require 'redis_tool'

class SlowLogger < RedisTool
  class Entry < Struct.new(:id, :timestamp, :duration, :command)
    def to_json(*args)
      to_h.to_json(*args)
    end
  end

  def initialize(*args)
    super(*args)

    @max_seen_id = 0
    unseen_slowlog_entries # discard previous entries prior to restart
  end

  def watch
    log("started")
    loop do
      output_slowlog
      sleep(5)
    end
  ensure
    log("stopped")
  end

  private

  def output_slowlog
    unseen_slowlog_entries.each { |entry| log("slowlog", :entry => entry) }
  end

  def unseen_slowlog_entries
    unseen  = @redis.slowlog('get', '1000').take_while { |id, *_| id > @max_seen_id }
    entries = unseen.reverse.map { |data| Entry.new(*data) }
    @max_seen_id = entries.last.id unless entries.empty?
    entries
  end
end

SlowLogger.from_argv(*ARGV).watch
