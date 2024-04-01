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
#define R1					1 // radius of the inside
#define R2					2 // radius of outside
// camera constants
#define K1					320*5*3/(8*(R1+R2))  // distance from camera to projection screen
#define K2					8 // distance from camera to donut
// MATHHHHHHHHHHHH
#define PI					3.141592653
#define HPI					1.570796327
#define TAU					6.283185306


// each row in the buffer is 1024 addrs, or 512 pixels
// subtract 320 for the pixels that are actually used to get 192 unused pixels
double rotation[] = {0,0,0};
int* timer = (int*) TIMER_BASE;

// little mod function for doubles: just used in the trig function replacements
double dmod(double x, double y) {
    return x - (int)(x/y) * y;
}
/**
 * use taylor series to approximate sine and cosine
 * math.h is more accurate but the intel fpga monitor program wont let me add the -lm flag >:(
 *
 * in overall, it is roughly off by no more than 0.005, the worse section being when the true value is near 1 or -1
 * (i love desmos)
 */
double sine(double x) {
	x = dmod(x, TAU);
	if(x < 0) x = TAU + x; // restrict to positive range 

	// our accurate range is -pi/2 -> pi/2, and our input domain is 0 -> tau [2pi]
	// to throw everything in this range, we "bounce" off of pi/2 when x is greater
	// (think of an arrow rotating from 0 -> pi/2, then bouncing counterclockwise down to -pi/2)
	// if x is greater than or equal to 
	if(x > 1.5*PI) {
		x = x - TAU; // if x is in the last quadrant just subtract 2pi so we stay in range 
	} else if(x > HPI) {
		// set x to (distance from x to hpi) less than hpi, sort of "bouncing" it back
		// (pi/2) - (x - (pi/2))
		// simplifies down to 
		// pi - x
		// intuitive visualization in desmos if you chart (sin x), (sin(pi-x)), and (p-x)
		x = PI - x; 
	}

	// janky taylor series
	x = x - (x*x*x / 6) + ((x*x*x*x*x) / 120);
	return x;
}

double cosine(double x) {
	return sine(x - HPI);
}

/*
 * empty the hardware frame buffer
 *
 * for startup and debug only, use the engine frame buffer for actual proj pls
 *
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
 * debug only
 */
void writeBufferPixel(unsigned short x,unsigned short y, char brightness) {
	short* pixel_addr = (short*) FRAME_BUFFER_BASE;
	pixel_addr += (x) + ((BUFFER_WIDTH>>1) * y);

	short value = (short) brightness;
	short pixel = ((value >> 3) << 11); // red uses the first 5 bits
	pixel |= (value >> 2) << 5; // green uses 6 bits
	pixel |= value >> 3; // blue uses last 5 bits
	*pixel_addr = pixel;
}

/**
 * write characters to buffer
 * grayscale cuz i suck
 */
void writeBuffer(char buffer[][320]) {
	short* pixel_addr = (short*) FRAME_BUFFER_BASE;
	for(int i=0; i < 240; i++) {
		for(int j=0; j < 320; j++) {

			short value = (short) buffer[i][j];
			
			short pixel = ((value >> 3) << 11); // red uses the first 5 bits
			pixel |= (value >> 2) << 5; // green uses 6 bits
			pixel |= value >> 3; // blue uses last 5 bits

			*pixel_addr = pixel;
			pixel_addr += 1;
		}
		pixel_addr += UNUSED_X_PIXELS;
	}
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
	double z = -((*button_addr & 4) * (4 - 2*(*switch_addr & 4))) >> 2;

	// rotations in radians
	rotation[0] += time * x / ROTATION_UNITS;
	if(rotation[0] > TAU)	rotation[0] -= TAU;
	else if(rotation[0] < 0) rotation[0] += TAU;

	rotation[1] += time * y / ROTATION_UNITS;
	if(rotation[1] > TAU)	rotation[1] -= TAU;
	else if(rotation[1] < 0) rotation[1] += TAU;

	rotation[2] += time * z / ROTATION_UNITS;
	if(rotation[2] > TAU)	rotation[2] -= TAU;

	
	// reset timer 
	*(timer+1) = *(timer);
}

void render() {
	char donutFrame[240][320];
	float donut_z[320][240];
	for(int i=0;i<240;i++) {
		for(int j=0;j<320;j++){
			donutFrame[i][j] = 0;
			donut_z[i][j] = 0;
		}
	}

	double x,y,z;
	double resolution = 0.0001;
	// the simulator runs slow, so we render a super low resolution if running in sim
	if(SIM) resolution = 0.5;
	//clearBuffer();

	//double m,s,r,i,j,k,minv, psi;
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

	for(double phi=-PI; phi<PI; phi += resolution) {
		for(double theta=-PI; theta<PI; theta += resolution) {
			// generate donut
			//x = cosine(phi)*(R2+(R1*cosine(theta)));
			//y = -sine(phi)*(R2+(R1*cosine(theta)));
			//z = R1 * sine(theta);

			//if(m != 0) {
				// calculate the rotation using the quaternion
				//x = (x*(1-(2*((j*j)+(k*k))*s))) + (y*(2*((i*j)+(k*r))*s)) + (z*(2*((i*k)-(j*r))*s));
				//y = (x*(2*((i*j)-(k*r))*s)) + (y*(1-(2*((i*i)+(k*k))*s))) + (z*(2*((j*k)+(i*r))*s));
				//z = (x*(2*((i*k)+(j*r))*s)) + (y*(2*((j*k)-(i*r))*s)) + (z*(1-(2*((i*i)+(j*j))*s)));
			//}
			// rotations with euler
			double costheta = cosine(theta);
			double sintheta = sine(theta);
			double cosphi = cosine(phi);
			double sinphi = sine(phi);
			double cosA = cosine(rotation[0]);
			double sinA = sine(rotation[0]);
			double cosB = cosine(rotation[1]);
			double sinB = sine(rotation[1]);

			x = (R2+(R1*costheta)) * ((cosB * cosphi) + (sinA*sinB*sinphi)) - (R1 * cosA * sinB * sintheta);
			y = (R2+(R1*costheta)) * ((sinB * cosphi) - (cosB*sinA*sinphi)) + (R1 * cosA * cosB * sintheta);
			z = (cosA * (R2 + (R1 * costheta) * sinphi)) + (R1 * sinA * sintheta);
			z += 2;


			// projection
			x *= K1 / (K2 + z);
			y *= K1 / (K2 + z);

			// center de donut
			x += 160;
			y += 120;

			// pixels are integers!
			int xPixel = (int)x;
			int yPixel = (int)y;

			double oz = 1 / z;
			if(x < 320 && y < 240 && x >= 0 && y >= 0) {
				if(oz > donut_z[yPixel][xPixel]) {
					donut_z[yPixel][xPixel] = oz;
					donutFrame[yPixel][xPixel] = 255;
				}
			}

		}
	}
	writeBuffer(donutFrame);
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



