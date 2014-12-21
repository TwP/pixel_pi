require "rubygems"
require "rake/extensiontask"

lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)

spec = Gem::Specification.load("pixel_pi.gemspec")

Gem::PackageTask.new(spec)

Rake::ExtensionTask.new("pixel_pi", spec) do |ext|
  ext.name    = "leds"
  ext.ext_dir = "ext/pixel_pi"
  ext.lib_dir = "lib/pixel_pi"
end

