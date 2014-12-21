#include <ruby.h>
#include "ws2811.h"

#define RGB2COLOR(r,g,b) ((((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff))

VALUE mPixelPi;
VALUE cLeds;
VALUE ePixelPiError;

static VALUE sym_dma, sym_frequency, sym_invert, sym_brightness;

/* ======================================================================= */

static void
pp_leds_free( void *ptr )
{
  ws2811_t *ledstring;
  if (NULL == ptr) return;

  ledstring= (ws2811_t*) ptr;
  if (ledstring->device) ws2811_fini( ledstring );
  xfree( ledstring );
}

static VALUE
pp_leds_allocate( VALUE klass )
{
  int ii;
  ws2811_t *ledstring;

  ledstring = ALLOC_N( ws2811_t, 1 );
  if (!ledstring) {
    rb_raise(rb_eNoMemError, "could not allocate PixelPi::Leds instance");
  }

  ledstring->freq   = WS2811_TARGET_FREQ;
  ledstring->dmanum = 5;
  ledstring->device = NULL;

  for (ii=0; ii<RPI_PWM_CHANNELS; ii++) {
    ledstring->channel[ii].gpionum    = 0;
    ledstring->channel[ii].count      = 0;
    ledstring->channel[ii].invert     = 0;
    ledstring->channel[ii].brightness = 255;
    ledstring->channel[ii].leds       = NULL;
  }

  return Data_Wrap_Struct( klass, NULL, pp_leds_free, ledstring );
}

static ws2811_t*
pp_leds_struct( VALUE self )
{
  ws2811_t *ledstring;

  if (TYPE(self) != T_DATA
  ||  RDATA(self)->dfree != (RUBY_DATA_FUNC) pp_leds_free) {
    rb_raise( rb_eTypeError, "expecting a PixelPi::Leds object" );
  }
  Data_Get_Struct( self, ws2811_t, ledstring );

  if (!ledstring->device) {
    rb_raise( ePixelPiError, "Leds are not initialized" );
  }

  return ledstring;
}

static int
pp_rgb_to_color( VALUE red, VALUE green, VALUE blue )
{
  int r = NUM2INT(red);
  int g = NUM2INT(green);
  int b = NUM2INT(blue);
  return RGB2COLOR(r, g, b);
}

/* ======================================================================= */
/* call-seq:
 *    PixelPi::Leds.new( length, gpio, options = {} )
 *
 * Create a new PixelPi::Leds instance that can be used to control a string of
 * NeoPixels from a RaspberryPi. The length of the pixel string must be given as
 * well as the GPIO pin number used to control the string. The remaining options
 * have sensible defaults.
 *
 * length  - the nubmer of leds in the string
 * gpio    - the GPIO pin number
 * options - Hash of arguments
 *   :dma        - DMA channel defaults to 5
 *   :frequency  - output frequency defaults to 800,000 Hz
 *   :invert     - defaults to `false`
 *   :brightness - defaults to 255
 */
static VALUE
pp_leds_initialize( int argc, VALUE* argv, VALUE self )
{
  ws2811_t *ledstring;
  VALUE length, gpio, opts, tmp;
  int resp;

  if (TYPE(self) != T_DATA
  ||  RDATA(self)->dfree != (RUBY_DATA_FUNC) pp_leds_free) {
    rb_raise( rb_eTypeError, "expecting a PixelPi::Leds object" );
  }
  Data_Get_Struct( self, ws2811_t, ledstring );

  /* parse out the length, gpio, and optional arguments if given */
  rb_scan_args( argc, argv, "21", &length, &gpio, &opts );

  /* get the number of pixels */
  if (TYPE(length) == T_FIXNUM) {
    ledstring->channel[0].count = NUM2INT(length);
    if (ledstring->channel[0].count < 0) {
      rb_raise( rb_eArgError, "length cannot be negative: %d", ledstring->channel[0].count );
    }
  } else {
    rb_raise( rb_eTypeError, "length must be a number: %s", rb_obj_classname(length) );
  }

  /* get the GPIO number */
  if (TYPE(gpio) == T_FIXNUM) {
    ledstring->channel[0].gpionum = NUM2INT(gpio);
    if (ledstring->channel[0].gpionum < 0) {
      rb_raise( rb_eArgError, "GPIO cannot be negative: %d", ledstring->channel[0].gpionum );
    }
  } else {
    rb_raise( rb_eTypeError, "GPIO must be a number: %s", rb_obj_classname(gpio) );
  }

  if (!NIL_P(opts)) {
    Check_Type( opts, T_HASH );

    /* get the DMA channel */
    tmp = rb_hash_lookup( opts, sym_dma );
    if (!NIL_P(tmp)) {
      if (TYPE(tmp) == T_FIXNUM) {
        ledstring->dmanum = NUM2INT(tmp);
        if (ledstring->dmanum < 0) {
          rb_raise( rb_eArgError, "DMA channel cannot be negative: %d", ledstring->dmanum );
        }
      } else {
        rb_raise( rb_eTypeError, "DMA channel must be a number: %s", rb_obj_classname(tmp) );
      }
    }

    /* get the frequency */
    tmp = rb_hash_lookup( opts, sym_frequency );
    if (!NIL_P(tmp)) {
      if (TYPE(tmp) == T_FIXNUM) {
        ledstring->freq = NUM2UINT(tmp);
      } else {
        rb_raise( rb_eTypeError, "frequency must be a number: %s", rb_obj_classname(tmp) );
      }
    }

    /* get the brightness */
    tmp = rb_hash_lookup( opts, sym_brightness );
    if (!NIL_P(tmp)) {
      if (TYPE(tmp) == T_FIXNUM) {
        ledstring->channel[0].brightness = (NUM2UINT(tmp) & 0xff);
        if (ledstring->channel[0].brightness < 0) {
          rb_raise( rb_eArgError, "brightness cannot be negative: %d", ledstring->channel[0].brightness );
        }
      } else {
        rb_raise( rb_eTypeError, "brightness must be a number: %s", rb_obj_classname(tmp) );
      }
    }

    /* get the invert flag */
    tmp = rb_hash_lookup( opts, sym_invert );
    if (!NIL_P(tmp)) {
      if (RTEST(tmp)) ledstring->channel[0].invert = 1;
      else            ledstring->channel[0].invert = 0;
    }
  }

  /* initialize the DMA and PWM cycle */
  resp = ws2811_init( ledstring );
  if (resp < 0) {
    rb_raise( ePixelPiError, "Leds could not be initialized: %d", resp );
  }

  return self;
}

/* call-seq:
 *    length
 *
 * Returns the number of pixels in the LED string.
 */
static VALUE
pp_leds_length_get( VALUE self )
{
  ws2811_t *ledstring = pp_leds_struct( self );
  return INT2FIX(ledstring->channel[0].count);
}

/* call-seq:
 *    gpio
 *
 * Returns the GPIO number used to control the pixels.
 */
static VALUE
pp_leds_gpio_get( VALUE self )
{
  ws2811_t *ledstring = pp_leds_struct( self );
  return INT2FIX(ledstring->channel[0].gpionum);
}

/* call-seq:
 *    dma
 *
 * Returns the DMA channel used to control the pixels.
 */
static VALUE
pp_leds_dma_get( VALUE self )
{
  ws2811_t *ledstring = pp_leds_struct( self );
  return INT2FIX(ledstring->dmanum);
}

/* call-seq:
 *    frequency
 *
 * Returns the output frequency.
 */
static VALUE
pp_leds_frequency_get( VALUE self )
{
  ws2811_t *ledstring = pp_leds_struct( self );
  return INT2FIX(ledstring->freq);
}

/* call-seq:
 *    invert
 *
 * Returns `true` if the invert flag is set and `false` if it is not set.
 */
static VALUE
pp_leds_invert_get( VALUE self )
{
  ws2811_t *ledstring = pp_leds_struct( self );
  if (ledstring->channel[0].invert) {
    return Qtrue;
  } else {
    return Qfalse;
  }
}

/* call-seq:
 *    brightness
 *
 * Returns the brightness.
 */
static VALUE
pp_leds_brightness_get( VALUE self )
{
  ws2811_t *ledstring = pp_leds_struct( self );
  return INT2FIX(ledstring->channel[0].brightness);
}

/* call-seq:
 *    brightness = 128   # value between 0 and 255
 *
 * Set the pixel brightness. This is a value between 0 and 255. All pixels will
 * be scaled by this value. The hue is not affected; only the luminosity is
 * affected.
 *
 * Returns the new brightness.
 */
static VALUE
pp_leds_brightness_set( VALUE self, VALUE brightness )
{
  ws2811_t *ledstring = pp_leds_struct( self );
  ledstring->channel[0].brightness = (NUM2UINT(brightness) & 0xff);
  return brightness;
}

/* call-seq:
 *    show
 *
 * Update the display with the data from the LED buffer.
 *
 * Returns this PixelPi::Leds instance.
 */
static VALUE
pp_leds_show( VALUE self )
{
  ws2811_t *ledstring = pp_leds_struct( self );
  int resp = ws2811_render( ledstring );
  if (resp < 0) {
    rb_raise( ePixelPiError, "PixelPi::Leds failed to render: %d", resp );
  }
  return self;
}

/* call-seq:
 *    clear
 *
 * Clear the display. This will set all values in the LED buffer to zero, and
 * then update the display. All pixels will be turned off by this method.
 *
 * Returns this PixelPi::Leds instance.
 */
static VALUE
pp_leds_clear( VALUE self )
{
  ws2811_t *ledstring = pp_leds_struct( self );
  ws2811_channel_t channel = ledstring->channel[0];
  int ii, resp;

  for (ii=0; ii<channel.count; ii++) {
    channel.leds[ii] = 0;
  }

  resp = ws2811_render( ledstring );
  if (resp < 0) {
    rb_raise( rb_eRuntimeError, "PixelPi::Leds failed to render: %d", resp );
  }

  return self;
}

/* call-seq:
 *    close
 *
 * FIXME
 *
 * Returns `nil`.
 */
static VALUE
pp_leds_close( VALUE self )
{
  ws2811_t *ledstring = pp_leds_struct( self );
  if (ledstring->device) ws2811_fini( ledstring );
  return Qnil;
}

/* call-seq:
 *    leds[num]
 *
 * Get the 24-bit RGB color value for the LED at position `num`.
 *
 * Returns a 24-bit RGB color value.
 */
static VALUE
pp_leds_get_pixel_color( VALUE self, VALUE num )
{
  ws2811_t *ledstring = pp_leds_struct( self );
  ws2811_channel_t channel = ledstring->channel[0];

  int n = NUM2INT(num);
  if (n < 0 || n >= channel.count) {
    rb_raise( rb_eIndexError, "index %d is outside of LED range: 0...%d", n, channel.count-1 );
  }

  return INT2FIX(channel.leds[n]);
}

/* call-seq:
 *    leds[num] = color
 *    leds[num] = PixelPi::Color( red, green, blue )
 *
 * Set the LED at position `num` to the provided 24-bit RGB color value.
 *
 * Returns the 24-bit RGB color value.
 */
static VALUE
pp_leds_set_pixel_color( VALUE self, VALUE num, VALUE color )
{
  ws2811_t *ledstring = pp_leds_struct( self );
  ws2811_channel_t channel = ledstring->channel[0];

  int n = NUM2INT(num);
  if (n >= 0 && n < channel.count) {
    channel.leds[n] = NUM2INT(color);
  }
  return self;
}

/* call-seq:
 *    set_pixel( num, color )
 *    set_pixel( num, red, green, blue )
 *
 * Set the LED at position `num` to the given color. The color can be a single
 * 24-bit RGB `color` value or three separate color values - one for `red`, one
 * for `green`, and one for `blue`.
 *
 * Returns this PixelPi::Leds instance.
 */
static VALUE
pp_leds_set_pixel_color2( int argc, VALUE* argv, VALUE self )
{
  VALUE num, color, red, green, blue;
  rb_scan_args( argc, argv, "22", &num, &red, &green, &blue );

  switch (argc) {
    case 2: {
      color = red;
      break;
    }
    case 4: {
      int c = pp_rgb_to_color( red, green, blue );
      color = INT2FIX(c);
      break;
    }
    default: {
      rb_raise( rb_eArgError, "expecting either 2 or 4 arguments: %d", argc );
      break;
    }
  }

  pp_leds_set_pixel_color( self, num, color );
  return self;
}

/* call-seq:
 *    PixelPi::Color(red, green, blue)  #=> 24-bit color
 *
 * Given a set of RGB values return a single 24-bit color value. The RGB values
 * are nubmers in the range 0..255.
 *
 * Returns a 24-bit RGB color value.
 */
static VALUE
pp_color( VALUE klass, VALUE red, VALUE green, VALUE blue )
{
  int color = pp_rgb_to_color( red, green, blue );
  return INT2FIX(color);
}

void Init_leds( )
{
  sym_dma        = ID2SYM(rb_intern( "dma" ));
  sym_frequency  = ID2SYM(rb_intern( "frequency" ));
  sym_invert     = ID2SYM(rb_intern( "invert" ));
  sym_brightness = ID2SYM(rb_intern( "brightness" ));

  mPixelPi = rb_define_module( "PixelPi" );

  cLeds = rb_define_class_under( mPixelPi, "Leds", rb_cObject );
  rb_define_alloc_func( cLeds, pp_leds_allocate );
  rb_define_method( cLeds, "initialize", pp_leds_initialize, -1 );

  rb_define_method( cLeds, "length",      pp_leds_length_get,        0 );
  rb_define_method( cLeds, "gpio",        pp_leds_gpio_get,          0 );
  rb_define_method( cLeds, "dma",         pp_leds_dma_get,           0 );
  rb_define_method( cLeds, "frequency",   pp_leds_frequency_get,     0 );
  rb_define_method( cLeds, "invert",      pp_leds_invert_get,        0 );
  rb_define_method( cLeds, "brightness",  pp_leds_brightness_get,    0 );
  rb_define_method( cLeds, "brightness=", pp_leds_brightness_set,    1 );
  rb_define_method( cLeds, "show",        pp_leds_show,              0 );
  rb_define_method( cLeds, "clear",       pp_leds_clear,             0 );
  rb_define_method( cLeds, "close",       pp_leds_close,             0 );
  rb_define_method( cLeds, "[]",          pp_leds_get_pixel_color,   1 );
  rb_define_method( cLeds, "[]=",         pp_leds_set_pixel_color,   2 );
  rb_define_method( cLeds, "set_pixel",   pp_leds_set_pixel_color2, -1 );

  rb_define_module_function( mPixelPi, "Color", pp_color, 3 );

  /* Define the PixelPi::Error class */
  ePixelPiError = rb_define_class_under( mPixelPi, "Error", rb_eStandardError );
}
