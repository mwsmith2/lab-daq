# A wrapper Class for the BK Precision 9124 Power Supply
import serial
import u3
<<<<<<< HEAD
import urllib2 as url
from urllib import quote
=======
>>>>>>> mws-dev

class BKPrecision:

    def __init__(self, dev_path, baud=4800, timeout=1):
        self.s = serial.Serial(dev_path, baud, timeout=timeout)
<<<<<<< HEAD
        print self.get_version()
=======
>>>>>>> mws-dev
   	self.id = int(self.get_version().split(',')[-2][-4:])
 
    def get_version(self):
        self.s.write('*IDN?\n')
        return self.s.read(64).strip()
    
    def meas_volt(self):
        self.s.write('MEAS:VOLT?\n')
        return self.s.read(64).strip()

    def get_volt(self):
	self.s.write('LIST:VOLT?\n')

    def set_volt(self, new_volt):
	self.s.write('SOUR:VOLT ' + str(new_volt) + '\n')

    def input_cmd(self, cmd):
	self.s.write(cmd + '\n')
	return self.s.read(64).strip()

    def power_on(self):
	self.s.write('OUTP:ON\n')

    def power_off(self):
	self.s.write('OUTP:OFF\n')

    def meas_curr(self):
	self.s.write('MEAS:CURR?\n')
	return self.s.read(64).strip()

    def get_curr(self):
	self.s.write('LIST:CURR?\n')
	return self.s.read(64).strip()

    def set_curr(self, new_curr):
	self.s.write('SOUR:CURR ' + str(new_curr) + '\n')

class Mover:

    def __init__(self, dev_path, baud_rate=9600, timeout=1):
	self.s = serial.Serial(dev_path)
	self.xnet = 0.0
	self.ynet = 0.0
	self.xmax = 20.0
	self.ymax = 20.0

    def move_x(self, x):
	if (x + self.xnet > self.xmax):
	   print "Request exceeds allowed movement range."
	   return
    	self.s.write('AX\n')
	self.s.write('MR ' + str(x) + '\n')
	self.s.write('GO\n')
	self.xnet += x

    def move_y(self, y):
	if (y + self.ynet > self.ymax):
	    print "Request exceeds allowed movement range."
	    return
	self.s.write('AY\n')
	self.s.write('MR ' + str(y) + '\n')
	self.s.write('GO\n')
	self.ynet += y

    def go_home(self):
	self.move_x(-self.xnet)
	self.move_y(-self.ynet)

class TempProbe:
  
    def __init__(self, default_probe=1):
	self.d = u3.U3() 
	self.address = 0x48
	self.channel_map = {
	    1:[12, 13],
	    2:[18, 19],
	    3:[10, 11],
	    4:[16, 17],
	    5:[ 8, 9 ],
	    6:[14, 15]
	}
	self.ch = self.channel_map[default_probe]
	self.i2c_conf = {
	    "EnableClockStretching": False,
            "NoStopWhenRestarting": True, #important
            "ResetAtStart": True,
            "SpeedAdjust": 0,
            "SDAPinNum": self.ch[0], #EIO6
            "SCLPinNum": self.ch[1], #EIO7
            "AddressByte": None
	}
    
    def reset_all(self):
	for i in range(1, 7):
	    self.reset(i)
	    
    def reset(self, ch):
	self.set_channel(ch)
	self.set_mode16()

    def set_mode16(self):
	self.d.i2c(self.address, [3, 0x80], NumI2CBytesToReceive=0, **self.i2c_conf)

    def set_channel(self, channel):
	self.ch = self.channel_map[channel]
	self.i2c_conf['SDAPinNum'] = self.ch[0]
	self.i2c_conf['SCLPinNum'] = self.ch[1]

    def meas_temp(self):
	data = self.d.i2c(self.address, [], NumI2CBytesToReceive=2, **self.i2c_conf)
	res = data['I2CBytes']
	temp = (res[0] << 8 | res[1] ) / 128.0
	return temp

    def meas_all_temp(self):
	temp = []
	for i in range(1, 7):
	    self.set_channel(i)
	    temp.append(self.meas_temp())
<<<<<<< HEAD
	return temp  


class UvaBias:
	"""A class to interface with the UVa/JMU bias supply.  It talks serially over ethernet."""
	
	def __init__(self, address="http://192.168.1.50"):
		self.addr = address

		self.lo_volt = [
			63.46, 62.86, 63.91, 63.67, 63.53, 63.64, 63.60, 63.84,
			63.65, 63.92, 64.35, 64.07, 64.08, 64.79, 64.17, 63.84,
			62.57, 63.36, 63.55, 64.19, 63.60, 63.83, 63.36, 64.61,
			63.43, 63.64, 63.84, 63.46, 64.04, 64.04, 64.91, 63.97]

		self.hi_volt = [
			72.3, 72.2, 73.0, 72.6, 72.6, 72.9, 72.6, 73.0,
			73.3, 73.8, 74.0, 74.0, 75.0, 74.5, 74.9, 74.3,
			71.4, 72.6, 72.5, 73.3, 72.3, 72.7, 72.3, 73.7,
			73.0, 73.4, 73.6, 73.6, 74.1, 74.1, 74.3, 74.0]

		self.bit_range = 2**14 - 1
		self.num_channels = 32
		self.ch_bits = [self.bit_range] * self.num_channels # initial set

	def check_bias(self):
		"""Check whether the supply is powered on."""
		val = int(self._open_url(self.addr, '/rpc/BVenable/read'))

		if val == 1:
			return "The bias supply is powered on."

		elif val == 0:
			return "The bias supply is powered off."

	def commands(self):
		"""A command to make sure we are talking to the device."""
		print self._open_url(self.addr, '/rpc')

	def bias_on(self):
		"""Turn on the the bias voltages."""
		self._set_all_channel_bits(self.bit_range)
		self._open_url(self.addr, '/rpc/BVenable/write 1')

	def bias_restore(self):
		"""Restore to the previous voltages before shutting down."""
		# Power on to lowest voltages.
		self._set_all_channel_bits(self.bit_range)
		self._open_url(self.addr, '/rpc/BVenable/write 1')

		# Move the the stored values
		self._set_all_channel_bits()

	def bias_off(self):
		"""Turn off the bias voltages."""
		self._set_all_channel_bits(self.bit_range)
		self._open_url(self.addr, '/rpc/BVenable/write 0')

	def set_channel(self, ch, volt):
		"""Change the bias on a specific channel, 1-32."""
		self.ch_bits[ch - 1] = self._convert_volt(ch, volt)
		self._set_channel_bits(ch, self.ch_bits[ch - 1])

	def read_channel(self, ch):
		"""Check the bias that was set on a specific channel."""
		val = int(self._open_url(self.addr, '/rpc/DAC/OutputRD %i' % ch))
		return self._convert_bits(ch, val)

	def set_all_channels(self, bias):
		"""Set all channels to the same bias value."""
		self._set_all_channel_bits(self._convert_volt(bias))

	def read_all_channels(self):
		"""Check the bias on each and every channel."""
		n = 1
		while (n < self.num_channels):

			line = ["Set Volt Channels %02i-%02i: " % (n, n + 7)]
			for i in range(n, n + 8):
				line.append("%.03f " % self.read_channel(i))

			print ''.join(line)
			n += 8

	def check_temperature(self):
		"""Read out the temperature on both boards."""
		temp_string = self._open_url(self.addr, '/rpc/ADC/TempRDfloat')
		for (i, val) in enumerate(temp_string.split()):
                    print "Board %i is at temperature %.02f C" % (i, float(val))


	def read_temperature(self):
		"""Read out the temperature on both boards."""
		temp_string = self._open_url(self.addr, '/rpc/ADC/TempRDfloat')
                temp = []
		for (i, val) in enumerate(temp_string.split()):
                    temp.append(float(val))
                return temp

	# "Private" functions
	def _set_channel_bits(self, ch, val):
		"""Set the value of the channel in integer value."""
		command = '/rpc/DAC/OutputWR %i %i' % (ch, val)
		self._open_url(self.addr, command)

	def _set_all_channel_bits(self, bias=-1):
		"""A private class function that sets all the channels."""
		if (bias == -1):

			for i in range(self.num_channels):
				self._set_channel_bits(i + 1, self.ch_bits[i])

		else:

			for i in range(self.num_channels):
				self._set_channel_bits(i + 1, bias)

	def _convert_volt(self, ch, volt):
		"""A private function to convert voltages to integer values."""
		if (volt <= self.lo_volt[ch - 1]):
			print "Value is below the channel's miniumum voltage."
			print "Using the voltage %f." % self.lo_volt[ch - 1]
			return self.bit_range

		elif (volt >= self.hi_volt[ch - 1]):
			print "Value is above the channel's maximum voltage."
			print "Using the voltage %f." % self.hi_volt[ch - 1]
			return 0

		else:
			volt_range = self.hi_volt[ch - 1] - self.lo_volt[ch - 1]
			return int(self.bit_range * (self.hi_volt[ch - 1] - volt) / volt_range)

	def _convert_bits(self, ch, val):
		"""Convert the integer value back to a voltage."""
		volt_range = self.hi_volt[ch - 1] - self.lo_volt[ch - 1]
		return self.hi_volt[ch - 1] - val * volt_range / self.bit_range

	def _open_url(self, url_path, args):
		"""A wrapper function for the url calls."""
		return url.urlopen(url_path + quote(args)).read()

=======
	return temp        
>>>>>>> mws-dev
