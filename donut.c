#include <stdio.h>
#include <unistd.h>
#define UNUSED_X_PIXELS 192 
// each row in the buffer is 1024 addrs, or 512 pixels
// subtract 320 for the pixels that are actually used to get 192 unused pixels

short* pixel_base = (short*) 0xc8000000;
int diff = (0x400 - (0x140 * 0x2));
	
void clearBuffer() {
	short* pixel_addr = pixel_base;
	
	for(int j=0;j<240;j++) {
		for(int i=0; i<320; i++) {
			*pixel_addr = 0x00;
			pixel_addr += 1;
		}
		pixel_addr += UNUSED_X_PIXELS;
	}
}

void writeBuffer(short x, short y, char brightness) {
	short* pixel_addr = (short*) pixel_base;
	pixel_addr += (x) + (512 * y);

	short value = (short) brightness;
	short pixel = ((value >> 3) << 11); // red uses the first 5 bits
	pixel |= (value >> 2) << 5; // green uses 6 bits
	pixel |= value >> 3; // blue uses last 5 bits
	*pixel_addr = pixel;
}
	
int main() {
	clearBuffer();
	for(int i=0; i < 30; i++) {
		for(int j=0; j<30; j++) {
			writeBuffer(i, j, 0xFF);
		}
	}
}

