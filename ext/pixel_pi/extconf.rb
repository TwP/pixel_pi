require 'mkmf'
require 'rbconfig'

pixel_pi_path = File.expand_path("../", __FILE__)
ws2811_path   = File.expand_path("../../ws2811", __FILE__)

if RbConfig::CONFIG["arch"].to_s =~ /\Aarm-linux/i
  ws2811_files  = %w[
    clk.h
    dma.h
    gpio.h
    pwm.h
    ws2811.h
    dma.c
    pwm.c
    ws2811.c
  ]
  ws2811_files.map! { |name| "#{ws2811_path}/#{name}" }
  FileUtils.cp(ws2811_files, pixel_pi_path)

  create_makefile("pixel_pi/leds")
else
  File.open("#{pixel_pi_path}/Makefile", "w") do |fd|
    fd.write <<-MAKEFILE
all: ;
install: ;
static: ;
install-so: ;
install-rb: ;
clean: ;
clean-so: ;
clean-static: ;
clean-rb: ;
    MAKEFILE
  end
end
