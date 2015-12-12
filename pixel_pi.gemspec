# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'pixel_pi/version'

Gem::Specification.new do |spec|
  spec.name          = "pixel_pi"
  spec.version       = PixelPi::VERSION
  spec.authors       = ["Tim Pease"]
  spec.email         = ["tim.pease@gmail.com"]
  spec.summary       = %q{A library for controlling NeoPixels via Ruby on the RaspberryPi}
  spec.description   = %q{NeoPixels are fun! Ruby is fun! RaspberryPi is fun! FUN^3}
  spec.homepage      = "https://github.com/TwP/pixel_pi"
  spec.license       = "MIT"
  spec.files         = `git ls-files -z`.split("\x0")
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.extensions    = spec.files.grep(%r{/extconf\.rb$})
  spec.require_paths = %w{lib}

  spec.add_development_dependency "rake-compiler", "~> 0.9"
  spec.add_development_dependency "rainbow",       "~> 2.0"
end
