#!/usr/bin/env python

### 
###	This node takes cmds from the joy node and publishes pwms
###		The servos are held in one position
###														

import rospy

from dynamixel_servo.msg import DynamixelFullConfig
from std_msgs.msg import Float64
from sensor_msgs.msg import Joy
from std_msgs.msg import Float64

pwm_port_pub = rospy.Publisher('stm32f3discovery_imu_driver/pwm1', Float64, queue_size = 10)
pwm_starboard_pub = rospy.Publisher('stm32f3discovery_imu_driver/pwm2', Float64, queue_size = 10)


def start():
	#Init node
	rospy.init_node('rc_tank_steer', anonymous=True)

	#Init subscribers and publishers
	joy_sub = rospy.Subscriber('joy', Joy, xbox_cb, queue_size = 10)

	#Spin
	rospy.spin()

def xbox_cb(joy_msg):
	pwm1 = Float64(0.005*(joy_msg.axes[1]) + 0.015)  #L_STICK
	pwm_port_pub.publish(pwm1)
	pwm2 = Float64(0.005*(joy_msg.axes[3]) + 0.015) #R_STICK
	pwm_starboard_pub.publish(pwm2) 

if __name__ == '__main__':
	try:
		start()
	except rospy.ROSInterruptException: pass 