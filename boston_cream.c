#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
// frame buffer constants
#define BUFFER_WIDTH		1024// width is 1024 addresses
#define UNUSED_X_PIXELS		192 
// base addrs for I/O
#define FRAME_BUFFER_BASE	0xc8000000
#define BUTTON_BASE			0xff200050
#define SWITCH_BASE			0xff200040
#define TIMER_BASE			0xfffec600
// geometry constants for the displayed torus
#define R1					40 // radius of the inside
#define R2					40 // radius of outside
// camera constants
#define K1					5  // distance from camera to projection screen
#define K2					10 // distance from camera to donut
// MATHHHHHHHHHHHH
#define PI					3.141592653


// each row in the buffer is 1024 addrs, or 512 pixels
// subtract 320 for the pixels that are actually used to get 192 unused pixels
double rotation[] = {0,0,0};
int* timer = (int*) TIMER_BASE;

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
	if(x >= 320 || y >= 240) return;
	short* pixel_addr = (short*) FRAME_BUFFER_BASE;
	pixel_addr += (x) + (BUFFER_WIDTH/2 * y);

	short value = (short) brightness;
	short pixel = ((value >> 3) << 11); // red uses the first 5 bits
	pixel |= (value >> 2) << 5; // green uses 6 bits
	pixel |= value >> 3; // blue uses last 5 bits
	*pixel_addr = pixel;
}

/*
 * read the buttons
 * we multiply the time since buttons were last read, by the respective button for each axis
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

	// rougly 1 1.5s of button holding is pi rotation
	rotation[0] += time * x / 1000.0;
	if(rotation[0] > 2)	rotation[0] -= 2.0;

	rotation[1] += time * y / 1000.0;
	if(rotation[1] > 2)	rotation[1] -= 2.0;

	rotation[2] += time * z / 1000.0;
	if(rotation[2] > 2)	rotation[2] -= 2.0;
	
	// reset timer 
	*(timer+1) = *(timer);
}

void render() {
	char donutFrame[320][240];
	float donut_z[320][240];
	for(int i=0;i<320;i++) {
		for(int j=0;j<240;j++){
			donutFrame[i][j] = 0;
			donut_z[i][j] = 0;
		}
	}

	double x,y,z;
	for(double theta=0; theta<2*PI; theta+= 0.05) {
		for(double phi=0; phi<2*PI; phi += 0.05) {
			x = cos(phi)*(R2+(R1*cos(theta)));
			y = R1 * sin(theta);
			z = sin(phi)*(R2+(R1*cos(theta)));
			x += 160;
			y += 120;
			writeBuffer((short)x, (short)y, 255);
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



