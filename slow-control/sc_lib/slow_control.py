# A wrapper Class for the BK Precision 9124 Power Supply
import serial
import u3
import urllib2 as url
from urllib import quote

class BKPrecision:

    def __init__(self, dev_path, baud=4800, timeout=1):
        self.s = serial.Serial(dev_path, baud, timeout=timeout)
        print self.get_version()
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
	return temp  


class UvaBias:
	"""A class to interface with the UVa bias supply.  It talks serially over ethernet."""
	
	def __init__(self, address="http://192.168.65.50"):
		self.addr = address
		self.lo_volt = 63.0
		self.hi_volt = 75.0
		self.bit_range = 4096
		self.num_channels = 32
		self.ch_vals = [self.bit_range] * self.num_channels # initial set
		self._set_all_channels(self.bit_range)

	def bias_check(self):
		"""Check whether the supply is powered on."""
		val = int(self._read_url(self.addr, '/rpc/BVenable/read'))

		if val == 1:
			return "The bias supply is powered on."

		elif val == 0:
			return "The bias supply is powered off."

	def commands(self):
		"""A command to make sure we are talking to the device."""
		print self._read_url(self.addr, '/rpc')

	def bias_on(self):
		"""Turn on the the bias voltages."""
		self._set_all_channels(self.bit_range)
		self._read_url(self.addr, '/rpc/BVenable/write 1')

	def bias_off(self):
		"""Turn off the bias voltages."""
		self._set_all_channels(self.bit_range)
		self._read_url(self.addr, '/rpc/BVenable/write 0')

	def set_channel(self, ch, volt):
		"""Change the bias on a specific channel, 1-32."""
		self.ch_vals[ch] = self._convert_volt(volt)
		self._set_channel_val(ch, self.ch_vals[ch])

	def read_channel(self, ch):
		"""Check the bias that was set on a specific channel."""
		val = int(self._read_url(self.addr, '/rpc/DAC/OutputRD %i' % ch))
		print _convert_val(val)

	def _set_channel_val(self, ch, val):
		"""Set the value of the channel in integer value."""
		command = '/rpc/DAC/OutputWR %i %i' % (ch, val)
		self._read_url(self.addr, command)

	def read_all_channels(self):
		"""Check the bias on each and every channel."""
		n = 1
		while (n < self.num_channels):

			line = ["Set Volt Channels %02i-%02i: " % (n, n + 7)]
			for i in range(n, n + 8):
				line.append("%.03f " % self._convert_bits(self.ch_vals[i - 1]))

			print ''.join(line)
			n += 8

	def read_temperature(self):
		"""Read out the temperature on both boards."""
		temp_string = self._read_url(self.addr, '/rpc/ADC/TempRDfloat')
		for (i, val) in enumerate(temp_string.split()):
			print "Board %i is at temperature %.02f C" % (i, float(val))

	def _set_all_channels(self, bias=-1):
		"""A private class function that sets all the channels."""
		if (bias == -1):

			for i in range(self.num_channels):
				self._set_channel_val(i + 1, self.ch_vals[i])

		else:

			for i in range(self.num_channels):
				self._set_channel_val(i + 1, bias)

	def _convert_volt(self, volt):
		"""A private function to convert voltages to integer values."""
		if (volt <= self.lo_volt):
			print "Value is below the miniumum voltage range."
			print "Using the minimum voltage %f." % self.lo_volt
			return self.bit_range

		elif (volt >= self.hi_volt):
			print "Value is above the maximum voltage range."
			print "Using the maximum voltage %f." % self.hi_volt
			return 0

		else:
			volt_range = self.hi_volt - self.lo_volt
			return int(self.bit_range * (self.hi_volt - volt) / volt_range)

	def _convert_bits(self, val):
		"""Convert the integer value back to a voltage."""
		volt_range = self.hi_volt - self.lo_volt
		return self.hi_volt - val * volt_range / self.bit_range

	def _read_url(self, url_path, args):
		"""A wrapper function for the url calls."""
		return url.urlopen(url_path + quote(args)).read()

