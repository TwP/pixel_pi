require "pixel_pi/version"

begin
  require "pixel_pi/leds"
rescue LoadError
  $stderr.puts "C extension is not configured."
  $stderr.puts "Using fake LEDs instead."
  require "pixel_pi/fake_leds"
end
