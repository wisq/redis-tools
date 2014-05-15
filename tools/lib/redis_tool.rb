#!/usr/bin/ruby

REDIS_TOOLS = File.realpath(File.dirname(__FILE__) + "/..")
ENV['BUNDLE_GEMFILE'] ||= REDIS_TOOLS + "/Gemfile"
require 'rubygems'
require 'bundler/setup'

require 'redis'
require 'redis/connection/hiredis'
require 'json'

class RedisTool
  def self.usage(message = nil)
    $stderr.puts "#{$0}: #{message}" if message
    $stderr.puts
    $stderr.puts "Usage: #{$0} <host> [port]"
    $stderr.puts "  port defaults to 6379"
    $stderr.puts
    exit(1)
  end

  def self.from_argv(host, port = 6379, *junk_args)
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

  protected

  def log(event, params = {})
    $stdout.puts({:event => event}.merge(params).to_json)
    $stdout.flush
  end
end
