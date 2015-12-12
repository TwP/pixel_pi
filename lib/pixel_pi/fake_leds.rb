require "forwardable"

module PixelPi

  # Given a set of RGB values return a single 24-bit color value. The RGB values
  # are nubmers in the range 0..255.
  #
  # Returns a 24-bit RGB color value.
  def self.Color( red, green, blue )
    ((red & 0xFF) << 16) | ((green & 0xFF) << 8) | (blue & 0xFF)
  end

  class Leds
    extend Forwardable

    # call-seq:
    #    PixelPi::Leds.new( length, gpio, options = {} )
    #
    # Create a new PixelPi::Leds instance that can be used to control a string of
    # NeoPixels from a RaspberryPi. The length of the pixel string must be given as
    # well as the GPIO pin number used to control the string. The remaining options
    # have sensible defaults.
    #
    # length  - the nubmer of leds in the string
    # gpio    - the GPIO pin number
    # options - Hash of arguments
    #   :dma        - DMA channel defaults to 5
    #   :frequency  - output frequency defaults to 800,000 Hz
    #   :invert     - defaults to `false`
    #   :brightness - defaults to 255
    #
    def initialize( length, gpio, options = {} )
      @leds       = [0] * length
      @gpio       = gpio
      @dma        = options.fetch(:dma, 5)
      @frequency  = options.fetch(:frequency, 800_000)
      @invert     = options.fetch(:invert, false)
      @brightness = options.fetch(:brightness, 255)
      @debug      = options.fetch(:debug, false)

      if @debug
        require "rainbow"
        @debug = "◉ " unless @debug.is_a?(String) && !@debug.empty?
      end
    end

    attr_reader :gpio, :dma, :frequency, :invert, :brightness

    def_delegators :@leds, :length, :[], :reverse, :rotate

    # Set the pixel brightness. This is a value between 0 and 255. All pixels will
    # be scaled by this value. The hue is not affected; only the luminosity is
    # affected.
    #
    # Returns the new brightness.
    def brightness=( value )
      @brightness = Integer(value) & 0xFF;
    end

    # Update the display with the data from the LED buffer. This is a noop method
    # for the fake LEDs.
    def show
      if @debug
        ary = @leds.map { |value| Rainbow(@debug).color(*to_rgb(value)) }
        $stdout.print "\r#{ary.join}"
      end
      self
    end

    # Clear the display. This will set all values in the LED buffer to zero, and
    # then update the display. All pixels will be turned off by this method.
    def clear
      @leds.fill 0
      self
    end

    # Shutdown the NeoPixels connected to the DMA / PWM channel. After this method
    # the current PixelPi::Leds instance will no longer be usable; a new instance
    # will need to be created. This method is automatically invoked when the
    # instance is deallcoated by the Ruby garbage collector. It does not need to be
    # explicitly invoked.
    #
    # Returns `nil`.
    def close
      $stdout.puts if @debug
      @leds = nil
    end

    # Set the LED at position `num` to the provided 24-bit RGB color value.
    #
    # Returns the 24-bit RGB color value.
    def []=( num, value )
      if (num < 0 || num >= @leds.length)
        raise IndexError, "index #{num} is outside of LED range: 0...#{@leds.length-1}"
      end
      @leds[num] = to_color(value)
    end

    # Set the LED at position `num` to the given color. The color can be a single
    # 24-bit RGB `color` value or three separate color values - one for `red`, one
    # for `green`, and one for `blue`.
    #
    # Returns this PixelPi::Leds instance.
    def set_pixel( num, *args )
      self[num] = to_color(*args)
      self
    end

    # Takes the current list of 24-bit RGB values stored in the LED strings and
    # returns them as an Array. These colors might not be actively displayed; it
    # all depends if `show` has been called on the PixelPi::Leds instance.
    #
    # Returns an Array of 24-bit RGB values.
    def to_a
      @leds.dup
    end

    # Replace the LED colors with the 24-bit RGB color values found in the `ary`.
    # If the `ary` is longer than the LED string then the extra color values will
    # be ignored. If the `ary` is shorter than the LED string then only the LEDS
    # up to `ary.length` will be changed.
    #
    # You must call `show` for the new colors to be displayed.
    #
    # Returns this PixelPi::Leds instance.
    def replace( ary )
      @leds.length.times do |ii|
        @leds[ii] = Integer(ary[ii])
      end
      self
    end

    # Set the selected LEDs to the given `color`. The `color` msut be given as a
    # 24-bit RGB value. You can also supply a block that receives an LED index and
    # returns a 24-bit RGB color.
    #
    # Examples:
    #    leds.fill( 0x00FF00 )
    #    leds.fill( 0xFF0000, 2, 2 )
    #    leds.fill( 0x0000FF, (4...8) )
    #    leds.fill { |i| 256 << i }
    #
    # Returns this PixelPi::Leds instance.
    def fill( *args )
      if block_given?
        @leds.fill do |ii|
          value = yield(ii)
          to_color(value)
        end
      else
        value = to_color(args.shift)
        @leds.fill(value, *args)
      end
      self
    end

  private

    def to_color( *args )
      case args.length
      when 1; Integer(args.first) & 0xFFFFFF
      when 3; PixelPi::Color(*args)
      else
        raise ArgumentError, "expecting either 1 or 3 arguments: #{args.length}"
      end
    end

    def to_rgb( color )
      scale = (brightness & 0xFF) + 1
      [
        (((color >> 16) & 0xFF) * scale) >> 8,
        (((color >>  8) & 0xFF) * scale) >> 8,
        (( color        & 0xFF) * scale) >> 8
      ]
    end
  end
end
