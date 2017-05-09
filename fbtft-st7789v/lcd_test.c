#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>

#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 320

int Read_bmp2memory ( char const *file_name, unsigned short *picture_pointer  )
{
	FILE* my_file;
	unsigned char bmp_header_buffer[54];
	unsigned char bmp_line_buffer[ DISPLAY_WIDTH * 3 ];
	unsigned short bfType;
	unsigned int bfOffBits;
	unsigned int biSize;
	unsigned int biWidth;
	unsigned int biHeight;
	unsigned short biBitCount;
	unsigned int y,x;
	unsigned char red, green, blue;
	unsigned int color;

	if( ( my_file = fopen( file_name, "rb" ) ) == NULL ){
		printf( "ERROR: Could not open input file for reading.\n" );
		return( -1 );
	}
	fread( &bmp_header_buffer, 1, 54, my_file );
	bfType = bmp_header_buffer[1];
	bfType = (bfType << 8) | bmp_header_buffer[0];
	if( bfType != 0x4D42){
		printf( "ERROR: Not a bitmap file.\n" );
		fclose( my_file );
		return( -1 );
	}
	biSize = bmp_header_buffer[17];
	biSize = (biSize << 8) | bmp_header_buffer[16];
	biSize = (biSize << 8) | bmp_header_buffer[15];
	biSize = (biSize << 8) | bmp_header_buffer[14];  
	if( biSize != 40 ){
		printf( "ERROR: Not Windows V3\n" );
		fclose( my_file );
		return( -1 );
	}
	biWidth = bmp_header_buffer[21];
	biWidth = (biWidth << 8) | bmp_header_buffer[20];
	biWidth = (biWidth << 8) | bmp_header_buffer[19];
	biWidth = (biWidth << 8) | bmp_header_buffer[18];
	biHeight = bmp_header_buffer[25];
	biHeight = (biHeight << 8) | bmp_header_buffer[24];
	biHeight = (biHeight << 8) | bmp_header_buffer[23];
	biHeight = (biHeight << 8) | bmp_header_buffer[22];

	biBitCount = bmp_header_buffer[29];
	biBitCount = (biBitCount << 8) | bmp_header_buffer[28];
	
    	printf( "%dx%dx%dbbp. ", biWidth, biHeight, biBitCount );
    
	if( (biWidth != DISPLAY_WIDTH) || (biHeight != DISPLAY_HEIGHT) || (biBitCount != 24) ){
		printf( "ERROR. %dx%dx%d required.\n", DISPLAY_WIDTH, DISPLAY_HEIGHT, 24 );
		fclose( my_file );
		return( -1 );
	}
	bfOffBits = bmp_header_buffer[13];
	bfOffBits = (bfOffBits << 8) | bmp_header_buffer[12];
	bfOffBits = (bfOffBits << 8) | bmp_header_buffer[11];
	bfOffBits = (bfOffBits << 8) | bmp_header_buffer[10];
	fseek( my_file, bfOffBits, SEEK_SET );

	for (y=DISPLAY_HEIGHT; y>0; y--){
		fread(&bmp_line_buffer[0], sizeof(bmp_line_buffer), 1, my_file );
		for (x=DISPLAY_WIDTH; x>0; x--){
			blue =  bmp_line_buffer[(x-1)*3 +0];
			green = bmp_line_buffer[(x-1)*3 +1];
			red =   bmp_line_buffer[(x-1)*3 +2];
			color = (red >> 3);
			color = color << 6;
			color = color | (green >> 2);
			color = color << 5;
			color = color | (blue >> 3);
			*picture_pointer = ~color;
			picture_pointer++;
		}
	}
	fclose( my_file );
	return ( 0 );
}

int main (int argc, char* argv[]) {
	int fp=0;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	long screensize=0;
	char *fbp = NULL;
	int x, y, location, loop, w, h;
	unsigned short nrgb[5] = {0x0000, 0xf800, 0x07e0, 0x001f, 0xffff};
	
	fp = open ("/dev/fb0",O_RDWR);
	if (fp < 0){
		printf("Error : Can not open framebuffer device\n");
		return 0;
	}

	if (ioctl(fp,FBIOGET_FSCREENINFO,&finfo)){
		printf("Error reading fixed information\n");
		goto END;
	}

	if (ioctl(fp,FBIOGET_VSCREENINFO,&vinfo)){
		printf("Error reading variable information\n");
		goto END;
	}

	printf("The mem is :%d\n",finfo.smem_len);
	printf("The line_length is :%d\n",finfo.line_length);
	printf("The xres is :%d\n",vinfo.xres);
	printf("The yres is :%d\n",vinfo.yres);
	printf("bits_per_pixel is :%d\n",vinfo.bits_per_pixel);

	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

	fbp =(char *) mmap (0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fp,0);
	if ((int) fbp == -1){
		printf ("Error: failed to map framebuffer device to memory./n");
		goto END;
	}

	if (argc == 1){
		for (loop = 0; loop < 50; loop++){
			h = vinfo.yres;
			x = 0;
			y = 0;
			printf("draw %d times\n", loop);
			while(h > 0){
				unsigned short* pos;
				w = vinfo.xres;
				x = 0;
				location = x * (vinfo.bits_per_pixel / 8) + y *  finfo.line_length;
				pos = (unsigned short*)(fbp + location);
				do{
					*pos = ~nrgb[loop % 5];
					pos++;
					x--;
					w--;
				}while(w>0);
				y++;
				h--;
			}
			usleep(1000 * 500);
		}
	}
	else {
/*		loop = 1;
		x = 0;
		y = 0;
		nrgb[0] = (unsigned short)strtol(argv[1], NULL, 16);
		printf("draw %d times\n", loop);
		while(y < vinfo.yres){
			unsigned short* pos;
			x = 0;
			location = x * (vinfo.bits_per_pixel / 8) + y *  finfo.line_length;
			pos = (unsigned short*)(fbp + location);
			do{
				*pos = nrgb[0];
				pos++;
				x++;
			}while(x<vinfo.xres);
			y++;
		}*/
		Read_bmp2memory(argv[1], (unsigned short*)(fbp));
	}
	munmap (fbp, screensize);
END:
	close (fp);
	return 0;
}
