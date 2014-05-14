#!/usr/bin/ruby

require 'redis'
require 'redis/connection/hiredis'
require 'json'

class SlowLogger
  class Entry < Struct.new(:id, :timestamp, :duration, :command)
    def to_json(*args)
      to_h.to_json(*args)
    end
  end

  def initialize(host, port)
    log("connecting")
    @redis = Redis.new(:host => host, :port => port)
    @redis.ping
    log("connected")

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

  def log(event, params = {})
    $stdout.puts(params.merge(:event => event).to_json)
    $stdout.flush
  end

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

def usage(message = nil)
  $stderr.puts "#{$0}: #{message}" if message
  $stderr.puts
  $stderr.puts "Usage: #{$0} <host> [port]"
  $stderr.puts "  port defaults to 6379"
  $stderr.puts
  exit(1)
end

host, port, *junk = ARGV
port = (port || 6379).to_i

usage("No host given") if host.nil?
usage("Port #{port} is out of range") unless (1..65535).include?(port)
usage unless junk.empty?

SlowLogger.new(host, port).watch
