# A wrapper Class for the BK Precision 9124 Power Supply
import serial
import u3

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
