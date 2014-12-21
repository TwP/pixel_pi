# What is this?

Ruby code for controlling [NeoPixels](https://www.adafruit.com/category/168)
attached to a [Raspberry Pi](http://www.raspberrypi.org).

# Is it any good?

Yes.

# How do I run it?

Install the gem on your RaspberryPi, and copy the `strandtest.rb` file to your
home directory:

```sh
sudo gem install pixel_pi
cp `gem contents pixel_pi | grep strandtest.rb` .
```

Edit the `LED_COUNT` and `LED_PIN` at the top of the file to
match the NeoPixel circuit attached to your RaspberryPi (read the [RaspberryPi
NeoPixel guide](https://learn.adafruit.com/neopixels-on-raspberry-pi/overview)
from Adafruit for all the details). Now run the strandtest example:

```sh
sudo ruby strandtest.rb
```

Enjoy the blinken lights!

# It won't run!

Yes it will.

The [ws2811](https://github.com/jgarff/rpi_ws281x) driver is using direct memory
addressing (DMA) via `/dev/mem` to control the NeoPixels. Only the root user has
permission to read and write to this hardware device. So any time your work with
NeoPixels, your code will need to be run as the super user.

# I want to help out

If you want to contribute to this gem then you will need a development
environment setup on your RaspberryPi. You will need the ruby headers installed
locally.

```
sudo apt-get install ruby ruby-dev
```

Install some ruby gems.

```
sudo gem install rake
sudo gem install rake-compiler
```

The `rake-compiler` gem might not install properly. The problem is a very strict
requirement on the rubygems version. To work around this we need to download the
source code, create the gem by hand, and install. You can skip this step if
rake-compiler installed successfully.

```
sudo su
git clone https://github.com/luislavena/rake-compiler.git
cd rake-compiler
rake gem
gem install pkg/rake-compiler-0.9.3.gem --no-rdoc --no-ri
cd ../
rm -fr rake-compiler
exit
```

Now we can compile the NeoPixels ruby library.

```
rake compile
```

Take a look at the `examples/strandtest.rb` file and adjust the `LED_COUNT` and
`LED_PIN` to match your circuit. Run the strandtest and bask in the glow of
blinking rainbow lights.

```
sudo ruby examples/strandtest.rb
```
