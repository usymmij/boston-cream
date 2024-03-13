#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
SIM = 1
## frame buffer constants
BUFFER_WIDTH = 1024 ## width is 1024 addresses
UNUSED_X_PIXELS		 =192 
## base addrs for I/O
FRAME_BUFFER_BASE	=0xc8000000
BUTTON_BASE			=0xff200050
SWITCH_BASE			=0xff200040
TIMER_BASE			=0xfffec600
## geometry constants for the displayed torus
ROTATION_UNITS		=2048
R1					=40 ## radius of the inside
R2					=50 ## radius of outside
## camera constants
K1					=30  ## distance from camera to projection screen
K2					=100 ## distance from camera to donut
## MATHHHHHHHHHHHH
PI					=3.141592653

import math
import numpy as np
import cv2


## each row in the buffer is 1024 addrs, or 512 pixels
## subtract 320 for the pixels that are actually used to get 192 unused pixels
rotation = [0,0,0];
frame = np.zeros((240,320), 'uint8')

def clearBuffer():
    global frame
    frame = np.zeros(frame.shape, 'uint8')

def writeBuffer(x,y,brightness):
    frame[x][y] = brightness;
    cv2.imshow("fr", frame)

def readButtons():
    key = cv2.waitKey(10)
    if(key == ord('w')):
        rotation[1] += 0.1
    if(key == ord('s')):
        rotation[1] -= 0.1

    if(key == ord('d')):
        rotation[0] += 0.1
    if(key == ord('a')):
        rotation[0] -= 0.1

    if(key == ord('e')):
        rotation[2] += 0.1
    if(key == ord('q')):
        rotation[2] -= 0.1

    for r in rotation:
        r %= 2 * math.pi 

    if(key == ord('x')):
        return 1
    return 0

def render():
    x=0
    y=0
    z=0
    resolution = 0.1
    clearBuffer()
    
    for phi in [x for x in range(math.ceil(2*math.pi/resolution))]:
        phi += rotation[2] 
        phi %= 2* math.pi
        for theta in [x for x in range(math.ceil(2*math.pi/resolution))]:
            ctheta = math.cos(theta)
            stheta = math.sin(theta)
            sphi = math.sin(phi)
            cphi = math.cos(phi)
            cA = math.cos(rotation[0])
            sA = math.sin(rotation[0])
            cB = math.cos(rotation[1])
            sB = math.sin(rotation[1])
            x = (R2+(R1*ctheta))*((cB*cphi)+(sA*sB*sphi)) - (R1*cA*sB*stheta)
            y = (R2+(R1*ctheta))*((cphi*sB)-(cB*sA*sphi)) + (R1*cA*cB*stheta)
            z = (cA*(R2+(R1*ctheta))*sphi)+(R1*sA*stheta)

            # perspective
            x *= K1
            x /= (K2 + z)
            y *= K1
            y /= (K2 + z)
            
            # center de donut
            x += 120
            y += 160

            brightness = ((z + 150)/300) % 1
            brightness = abs(255 * brightness)
            if(x < 240 and y < 320 and x > 0 and y > 0 and z < K2):
                writeBuffer(math.floor(x), math.floor(y), brightness)
			
if __name__ == "__main__":
    writeBuffer(0,0,0)
    while(readButtons() == 0):
        render()



'''

void render() {	
    double x,y,z;

	double resolution = 0.0001;
	# the simulator runs slow, so we render a super low resolution if running in sim
	if(SIM) resolution = 0.5;
	clearBuffer();

	double m,s,r,i,j,k,minv, psi;
	# rotation quaternion magnitude
	m = rotation[0] * rotation[0];
	m += rotation[1] * rotation[1];
	m += rotation[2] * rotation[2];
	if(m != 0) {
		s = 1/m;
		m = sqrt(m);
		minv = 1/m;
		psi = PI * m/2;
		r = cosine(psi);
		i = sine(psi) * rotation[0] * minv;
		j = sine(psi) * rotation[1] * minv;
		k = sine(psi) * rotation[2] * minv;
	}

	for(double phi=0; phi<2*PI; phi += resolution) {
		for(double theta=0; theta<2*PI; theta += resolution) {
			# generate donut
			x = cosine(phi)*(R2+(R1*cosine(theta)));
			y = -sine(phi)*(R2+R1*cosine(theta));
			z = R1 * sine(theta);

			if(m != 0) {
				# calculate the rotation using the quaternion
				x = (x*(1-(2*((j*j)+(k*k))*s))) + (y*(2*((i*j)+(k*r))*s)) + (z*(2*((i*k)-(j*r))*s));
				y = (x*(2*((i*j)-(k*r))*s)) + (y*(1-(2*((i*i)+(k*k))*s))) + (z*(2*((j*k)+(i*r))*s));
				z = (x*(2*((i*k)+(j*r))*s)) + (y*(2*((j*k)-(i*r))*s)) + (z*(1-(2*((i*i)+(j*j))*s)));
			}

			# projection
			x *= K1;
			x /= (K2 + z);
			y *= K1;
			y /= (K2 + z);

			# center de donut
			x += 160;
			y += 120;

			if(x < 320 && y < 240 && x > 0 && y > 0 && z > 0) writeBuffer((short)x, (short)y, 255);
		}
	}
}
int main() {
	# set background
	clearBuffer();
	*timer = 0xffffffff; # set the timer to max 
	*(timer + 2) = 0xff01; # 256 prescaler (781.25 kHz), no interrupt, no restart, enable 

	for(;;){
		readButtons();
		render();
	}
}


'''
