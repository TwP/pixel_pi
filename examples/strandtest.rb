require "rubygems"

begin
  require "pixel_pi"
rescue LoadError
  lib = File.expand_path('../../lib', __FILE__)
  raise if $LOAD_PATH.include?(lib)
  $LOAD_PATH.unshift(lib)
  retry
end

LED_COUNT      = 8       # Number of LED pixels.
LED_PIN        = 18      # GPIO pin connected to the pixels (must support PWM!).
LED_FREQ_HZ    = 800000  # LED signal frequency in hertz (usually 800khz)
LED_DMA        = 5       # DMA channel to use for generating signal (try 5)
LED_BRIGHTNESS = 255     # Scale the brightness of the pixels (0 to 255)
LED_INVERT     = false   # True to invert the signal (when using NPN transistor level shift)

module StrandTest

  attr_accessor :wait_ms

  # Wipe color across display a pixel at a time.
  #
  # color - The 24-bit RGB color value
  # opts  - The options Hash
  #   :wait_ms - sleep time between pixel updates
  #
  # Returns this PixelPi::Leds instance.
  def color_wipe( color, opts = {} )
    wait_ms = opts.fetch(:wait_ms, self.wait_ms)

    self.length.times do |num|
      self[num] = color
      self.show
      sleep(wait_ms / 1000.0)
    end

    self
  end

  # Movie theater light style chaser animation.
  #
  # color - The 24-bit RGB color value
  # opts  - The options Hash
  #   :wait_ms    - sleep time between pixel updates
  #   :iterations - number of iterations (defaults to 10)
  #   :spacing    - spacing between lights (defaults to 3)
  #
  # Returns this PixelPi::Leds instance.
  def theater_chase( color, opts = {} )
    wait_ms    = opts.fetch(:wait_ms, self.wait_ms)
    iterations = opts.fetch(:iterations, 10)
    spacing    = opts.fetch(:spacing, 3)

    iterations.times do
      spacing.times do |jj|
        (0...self.length).step(spacing) { |ii| self[ii+jj] = color }
        self.show
        sleep(wait_ms / 1000.0)
        (0...self.length).step(spacing) { |ii| self[ii+jj] = 0 }
      end
    end

    self
  end

  # Generate rainbow colors across 0-255 positions.
  #
  # pos - Positoin between 0 and 255
  #
  # Returns a 24-bit RGB color value.
  def wheel( pos )
    if pos < 85
      return PixelPi::Color(pos * 3, 255 - pos * 3, 0)
    elsif pos < 170
      pos -= 85
      return PixelPi::Color(255 - pos * 3, 0, pos * 3)
    else
      pos -= 170
      return PixelPi::Color(0, pos * 3, 255 - pos * 3)
    end
  end

  # Draw rainbow that fades across all pixels at once.
  #
  # opts - The options Hash
  #   :wait_ms    - sleep time between pixel updates
  #   :iterations - number of iterations (defaults to 1)
  #
  # Returns this PixelPi::Leds instance.
  def rainbow( opts = {} )
    wait_ms    = opts.fetch(:wait_ms, self.wait_ms)
    iterations = opts.fetch(:iterations, 1)

    (0...256*iterations).each do |jj|
      self.length.times { |ii| self[ii] = wheel((ii+jj) & 0xff) }
      self.show
      sleep(wait_ms / 1000.0)
    end

    self
  end

  # Draw rainbow that uniformly distributes itself across all pixels.
  #
  # opts - The options Hash
  #   :wait_ms    - sleep time between pixel updates
  #   :iterations - number of iterations (defaults to 5)
  #
  # Returns this PixelPi::Leds instance.
  def rainbow_cycle( opts = {} )
    wait_ms    = opts.fetch(:wait_ms, self.wait_ms)
    iterations = opts.fetch(:iterations, 5)

    (0...256*iterations).each do |jj|
      self.length.times { |ii| self[ii] = wheel(((ii * 256 / self.length) + jj) & 0xff) }
      self.show
      sleep(wait_ms / 1000.0)
    end

    self
  end

  # Rainbow moview theather light style chaser animation.
  #
  # opts - The options Hash
  #   :wait_ms - sleep time between pixel updates
  #   :spacing - spacing between lights (defaults to 3)
  #
  # Returns this PixelPi::Leds instance.
  def theater_chase_rainbow( opts = {} )
    wait_ms = opts.fetch(:wait_ms, self.wait_ms)
    spacing = opts.fetch(:spacing, 3)

    256.times do |jj|
      spacing.times do |sp|
        (0...self.length).step(spacing) { |ii| self[ii+sp] = wheel((ii+jj) % 255) }
        self.show
        sleep(wait_ms / 1000.0)
        (0...self.length).step(spacing) { |ii| self[ii+sp] = 0 }
      end
    end

    self
  end

end


strip = PixelPi::Leds.new \
    LED_COUNT, LED_PIN,
    :frequency  => LED_FREQ_HZ,
    :dma        => LED_DMA,
    :brightness => LED_BRIGHTNESS,
    :invert     => LED_INVERT

strip.extend StrandTest

trap("SIGINT") do
  strip.clear
  strip.close  # not explicitly needed - the finalizer will gracefully shutdown
  exit         # the PWM channel and release the DMA memory
end

STDOUT.puts "Press Ctrl-C to quit."

loop do
  # Color wipe animations
  strip.wait_ms = 75
  strip.color_wipe(PixelPi::Color(255, 0, 0))  # red color wipe
  strip.color_wipe(PixelPi::Color(0, 255, 0))  # green color wipe
  strip.color_wipe(PixelPi::Color(0, 0, 255))  # blue color wipe

  # Theater chase animations
  strip.wait_ms = 100
  strip.clear
  strip.theater_chase(PixelPi::Color(255, 255, 255))  # white theater chase
  strip.theater_chase(PixelPi::Color(255,   0,   0))  # red theater chase
  strip.theater_chase(PixelPi::Color(  0,   0, 255))  # blue theater chase

  strip.wait_ms = 20
  strip.rainbow
  strip.rainbow_cycle
  strip.theater_chase_rainbow(:wait_ms => 75)
end
