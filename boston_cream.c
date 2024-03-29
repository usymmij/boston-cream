#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define SIM					1
// frame buffer constants
#define BUFFER_WIDTH		1024// width is 1024 addresses
#define UNUSED_X_PIXELS		192 
// base addrs for I/O
#define FRAME_BUFFER_BASE	0xc8000000
#define BUTTON_BASE			0xff200050
#define SWITCH_BASE			0xff200040
#define TIMER_BASE			0xfffec600
// geometry constants for the displayed torus
#define ROTATION_UNITS		2048
#define R1					40 // radius of the inside
#define R2					40 // radius of outside
// camera constants
#define K1					30  // distance from camera to projection screen
#define K2					50 // distance from camera to donut
// MATHHHHHHHHHHHH
#define PI					3.141592653


// each row in the buffer is 1024 addrs, or 512 pixels
// subtract 320 for the pixels that are actually used to get 192 unused pixels
double rotation[] = {0,0,0};
int* timer = (int*) TIMER_BASE;

double cosine(double x) {
	return cos(x);
}
double sine(double x) {
	return sin(x);
}

/*
 * empty the frame buffer
 */
void clearBuffer() {
	short* pixel_addr = (short*) FRAME_BUFFER_BASE;
	
	for(int j=0;j<240;j++) {
		for(int i=0; i<320; i++) {
			*pixel_addr = 0x00;
			pixel_addr += 1;
		}
		pixel_addr += UNUSED_X_PIXELS;
	}
}

/*
 * write a pixel to the buffer
 */
void writeBuffer(unsigned short x,unsigned short y, char brightness) {
	short* pixel_addr = (short*) FRAME_BUFFER_BASE;
	pixel_addr += (x) + ((BUFFER_WIDTH>>1) * y);

	short value = (short) brightness;
	short pixel = ((value >> 3) << 11); // red uses the first 5 bits
	pixel |= (value >> 2) << 5; // green uses 6 bits
	pixel |= value >> 3; // blue uses last 5 bits
	*pixel_addr = pixel;
}

/*
 * read the buttons
 * we multiply the time sinece buttons were last read, by the respective button for each axis
 * we also multiply by (1 - (2* switch)) for the following logic: 0 -> positive, 1 -> negative
 */
void readButtons() {
	char* button_addr = (char*) BUTTON_BASE;
	char* switch_addr = (char*) SWITCH_BASE;
	double time = *(timer) - *(timer + 1); // the timer init value - curr val
	time /= 512; // convert to 1.53 kHz ~ 1 kHz
	
	// signed +1 or -1 of the button / switch input
	// i cooould case to double later for memory efficiency but it doesn't reaally matter here
	double x = (*button_addr & 1) * (1 - 2*(*switch_addr & 1));
	double y = ((*button_addr & 2) * (2 - 2*(*switch_addr & 2))) >> 1;
	double z = ((*button_addr & 4) * (4 - 2*(*switch_addr & 4))) >> 2;

	// rotations in radians
	rotation[0] += time * x / ROTATION_UNITS;
	if(rotation[0] > PI)	rotation[0] -= 2.0 * PI;

	rotation[1] += time * y / ROTATION_UNITS;
	if(rotation[1] > PI)	rotation[1] -= 2.0 * PI;

	rotation[2] += time * z / ROTATION_UNITS;
	if(rotation[2] > PI)	rotation[2] -= 2.0 * PI;
	
	// reset timer 
	*(timer+1) = *(timer);
}

void render() {
	/**
	char donutFrame[320][240];
	float donut_z[320][240];
	for(int i=0;i<320;i++) {
		for(int j=0;j<240;j++){
			donutFrame[i][j] = 0;
			donut_z[i][j] = 0;
		}
	}
	**/

	double x,y,z;
	double resolution = 0.0001;
	// the simulator runs slow, so we render a super low resolution if running in sim
	if(SIM) resolution = 0.5;
	clearBuffer();

	double m,s,r,i,j,k,minv, psi;
	// rotation quaternion magnitude
	//m = rotation[0] * rotation[0];
	//m += rotation[1] * rotation[1];
	//m += rotation[2] * rotation[2];
	//if(m != 0) {
	//	s = 1/m;
	//	m = sqrt(m);
	//	minv = 1/m;
	//	psi = PI * m/2;
	//	r = cosine(psi);
	//	i = sine(psi) * rotation[0] * minv;
	//	j = sine(psi) * rotation[1] * minv;
	//	k = sine(psi) * rotation[2] * minv;
	//}

	for(double phi=0; phi<2*PI; phi += resolution) {
		for(double theta=0; theta<2*PI; theta += resolution) {
			// generate donut
			x = cosine(phi)*(R2+(R1*cosine(theta)));
			y = -sine(phi)*(R2+(R1*cosine(theta)));
			z = R1 * sine(theta);

			if(m != 0) {
				// calculate the rotation using the quaternion
				//x = (x*(1-(2*((j*j)+(k*k))*s))) + (y*(2*((i*j)+(k*r))*s)) + (z*(2*((i*k)-(j*r))*s));
				//y = (x*(2*((i*j)-(k*r))*s)) + (y*(1-(2*((i*i)+(k*k))*s))) + (z*(2*((j*k)+(i*r))*s));
				//z = (x*(2*((i*k)+(j*r))*s)) + (y*(2*((j*k)-(i*r))*s)) + (z*(1-(2*((i*i)+(j*j))*s)));
				x = (R2+(R1*cosine(theta))) * ((cos(rotation[1]) * cos(phi)) + (sine(rotation[0])* sine(rotation[1])*sine(phi))) - (R1 * cos(rotation[0]) * cos(rotation[1]) * sine(theta))
				y = (R2+(R1*cosine(theta))) * ((sine(rotation[1]) * cos(phi))-(cosine(rotation[1])*sine(rotation[0])*sine(phi))) + (R1 * cos(rotation[0]) * cos(rotation[1]) * sine(theta))
				z = (cosine(rotation[0]) * (R2 + (R1 * cosine(theta)) * sine(phi) ) + (R1 * sine(A) * sine(phi))

			}

			// projection
			x *= K1;
			x /= (K2 + z);
			y *= K1;
			y /= (K2 + z);

			// center de donut
			x += 160;
			y += 120;

			if(x < 320 && y < 240 && x > 0 && y > 0 && z > 0) writeBuffer((short)x, (short)y, 255);
		}
	}
}
	
/*
 * entry point
 */
int main() {
	// set background
	clearBuffer();
	*timer = 0xffffffff; // set the timer to max 
	*(timer + 2) = 0xff01; // 256 prescaler (781.25 kHz), no interrupt, no restart, enable 

	for(;;){
		readButtons();
		render();
	}
}



