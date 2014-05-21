#!/usr/bin/ruby

REDIS_TOOLS = File.dirname(File.dirname(__FILE__))
ENV['BUNDLE_GEMFILE'] ||= REDIS_TOOLS + "/Gemfile"
require 'rubygems'
require 'bundler/setup'

require 'redis'
require 'redis/connection/hiredis'

class RedisTool
  def self.usage(message = nil)
    $stderr.puts "#{$0}: #{message}" if message
    $stderr.puts
    $stderr.puts "Usage: #{$0} <host> [port]"
    $stderr.puts "  port defaults to 6379"
    $stderr.puts
    exit(1)
  end

  def self.from_argv(host = nil, port = 6379, *junk_args)
    port = port.to_i

    usage("No host given") if host.nil?
    usage("Port #{port} is out of range") unless (1..65535).include?(port)
    usage unless junk_args.empty?

    new(host, port)
  end

  def initialize(host, port)
    log("connecting")
    @redis = Redis.new(:host => host, :port => port)
    @redis.ping
    log("connected")
  end

  def run
    log("started")
    run_wrapped
  rescue SignalException => e
    log("signal", :name => e.message, :number => e.signo, :location => e.backtrace.first)
  rescue Exception => e
    log("exception", :class => e.class, :message => e.message, :backtrace => e.backtrace)
  ensure
    log("stopped")
  end

  protected

  def log(event, params = {})
    data = {:event => event}.merge(params)
    $stdout.puts(data.map { |k, v| "#{k}=#{log_stringify(v)}" }.join(" "))
    $stdout.flush
  end

  def log_stringify(value)
    string = value.inspect
    string = string.inspect unless string =~ /\A[0-9"]/ # double-inspect for arrays/hashes etc.
    string
  end
end
