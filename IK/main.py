from FK import *
import serial

def usage():
	print "Usage : input 9 pose parameters."

<<<<<<< HEAD
'''
ser = serial.Serial(
    port='/dev/ttyUSB1',
=======

ser = serial.Serial(
    port='/dev/cu.usbmodem1411',
>>>>>>> 38d72168839cdd1dfd24f681fd9598f5ca72b867
    baudrate=9600,
    parity=serial.PARITY_ODD,
    stopbits=serial.STOPBITS_TWO,
    bytesize=serial.SEVENBITS
)

ser.isOpen()
<<<<<<< HEAD
'''
=======
>>>>>>> 38d72168839cdd1dfd24f681fd9598f5ca72b867

print 'Enter your commands below.\r\nInsert "exit" to leave the application.'

cur_th = [0,-20,0,30,0,90]

while 1 :
    # get keyboard input
    cmd = raw_input(">> ")
    if cmd == 'exit':
        #ser.close()
        exit()
    else:
    	#print "Wrote : %s" % (cmd)
        pose = map(float, cmd.split())
        if len(pose) != 9:
        	usage()
        	continue

        ik_result = IK(th, pose)
<<<<<<< HEAD
        print ik_result
        cur_th = ik_result
		
		#ser.write(cur_th + '\n')

        out = 'result'
        # while ser.inWaiting() > 0:
        #     out += ser.read(1)
=======
        cur_th = list(ik_result)
        for i in range(len(cur_th)):
            cur_th[i] = str(int(cur_th[i]))
        print " ".join(cur_th)+" "
        ser.write(" ".join(cur_th)+" ")
        #print cur_th
        
        #ser.write(' '.join(cur_th) + '\n')
        #print ' '.join(cur_th) + '\n'
        out = 'result'
        while ser.inWaiting() > 0:
             out += ser.read(1)
>>>>>>> 38d72168839cdd1dfd24f681fd9598f5ca72b867

        if out != '':
            print "<< " + out