/* 
 * Really primitive text mode VGA driver. Could be implemented as a real
 * driver at some point I guess...
*/
#include <scaraOS/kernel.h>
#include <arch/vga.h>
#include <arch/io.h>

#define VIDMEM		__va(0xB8000)
#define MONITOR_NONE    0
#define MONITOR_COLOUR  1
#define MONITOR_MONO    2
#define MONITOR_UNKNOWN 3

#define CRTC_INDEX_REG	0x3d4
#define CRTC_DATA_REG	0x3d5

static volatile uint8_t *vidmem = (uint8_t *)VIDMEM;
static uint16_t attrib = 0x0017;
static uint8_t xpos, ypos;
static unsigned int monitor = MONITOR_NONE;

void vga_preinit(void)
{
	int i;

	/* Detect monitor type */
	switch (*(((uint8_t *)0x410)) & 0x30) {
	case 0x00:
		monitor = MONITOR_NONE;
		break;
	case 0x20:
		monitor = MONITOR_COLOUR;
		break;
	case 0x30:
		monitor = MONITOR_MONO;
		attrib = 0x00 - 7;
		break;
	default:
		monitor = MONITOR_UNKNOWN;
	}

	/* Clear the screeen */
	for (i = 0; i < COLS * ROWS * 2; i += 2) {
		*(vidmem + i) = 0;
		*(vidmem + i + 1) = attrib;
	}

	/* Set cursor to 0,0 */
	vga_curs(0, 0);
}

void vga_put(uint8_t c)
{
	switch(c) {
	case 8: /* Delete */
		/* Move our position back one */
		if ( xpos == 0 ) {
			if ( ypos )
				ypos--;
			xpos = COLS - 1;
		}else{
			xpos--;
		}
		
		/* Blank current position */
		*(vidmem + (xpos + ypos * COLS) * 2) = ' ';
		*(vidmem + (xpos + ypos * COLS) * 2 + 1) = attrib;
		
		break;
		
	case '\n':
	case '\r':
newline:
		xpos = 0;
		ypos++;
		if (ypos >= ROWS) {
			int i;
		
			/* Move all the lines up one */
			for (i = COLS; i < (ROWS * COLS); i++ ) {
				*(vidmem + ((i - COLS)) * 2) =
					*(vidmem + i * 2);
			}
		
			/* Blank the last line */
			for (i = 0; i < COLS; i++) {
				*(vidmem + ((ROWS * COLS - COLS + i) * 2)) =' ';
				*(vidmem + ((ROWS * COLS - COLS + i) * 2)+1) = attrib;
			}
			ypos--;
		}
		break;

	default:
		*(vidmem + (xpos + ypos * COLS) * 2) = c;
		*(vidmem + (xpos + ypos * COLS) * 2 + 1) = attrib;
		xpos++;
		if (xpos >= COLS)
			goto newline; /* Wrap the line */
		break;
	}
}

/* Set cursor position */
void vga_curs(int x, int y)
{
	uint32_t vid_off;

	if ( x < 0 )
		x = xpos;
	if ( y < 0 )
		y = ypos;

	vid_off = (x + y * COLS) * 2;
	vid_off >>= 1;

	outb(CRTC_INDEX_REG, 0x0f);
	outb(CRTC_DATA_REG, vid_off & 0xfff);

	outw(CRTC_INDEX_REG, 0x0e);
	outb(CRTC_DATA_REG, vid_off >> 8);
}

void __init vga_init(void)
{
	static const char * const m[] = {"none", "colour", "monochrome"};

	if ( monitor < sizeof(m)/sizeof(*m) )
		dprintk("vga: monitor detected: %s\n", m[monitor]);
	else
		printk("vga: monitor not recognised\n");
}

driver_init(vga_init);
