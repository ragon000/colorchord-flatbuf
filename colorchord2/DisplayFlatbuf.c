//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include "outdrivers.h"
#include "notefinder.h"
#include "parameters.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "color.h"
#include "DrawFunctions.h"
#include "hyperion_request_builder.h"
#include <stdio.h> 
#include <sys/socket.h> 
#include <netinet/tcp.h>
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
//Uses: note_amplitudes2[note] for how many lights to use.
//Uses: note_amplitudes_out[note] for how bright it should be.
// Convenient namespace macro to manage long namespace prefix.
#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(hyperionnet, x) // Specified in the schema.
// A helper to simplify creating vectors from C-arrays.
#define c_vec_len(V) (sizeof(V)/sizeof((V)[0]))
#define MAX_LEDS_PER_NOTE 512
#define PORT 19400

extern char **gargv;
extern int gargc;
int fd;

void hexDump (const char *desc, const void *addr, const int len) {
    int i;
    unsigned char buff[17];
    const unsigned char *pc = (const unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        printf("  NEGATIVE LENGTH: %i\n",len);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}

struct DPODriver
{
	int rows;
	int cols;
	int chain;
	int width;
	int heigth;
};


static void DPOUpdate(void *id, struct NoteFinder *nf)
{
	printf( "Update Matrix\n" );
	int x, y;
	struct DPODriver *d = (struct DPODriver *)id;
	int i = 0;
        uint8_t *buf;
        size_t size;
        flatcc_builder_t builder,  *B;
        B = &builder;
        flatcc_builder_init(B);
        flatbuffers_uint8_vec_ref_t imgvec = flatbuffers_uint8_vec_create(B,OutLEDs,c_vec_len(OutLEDs));
        ns(RawImage_ref_t) rawimg =  ns(RawImage_create(B,imgvec,d->width,d->heigth));
        ns(ImageType_union_ref_t) img = ns(ImageType_as_RawImage(rawimg));
        
        ns(Image_ref_t) req = ns(Image_create(B,img, -1));
        ns(Command_union_ref_t) cmd = ns(Command_as_Image(req));
        hyperionnet_Request_create_as_root(B,cmd);
        buf = flatcc_builder_finalize_buffer(B, &size);
        printf("size: %ld, width: %d, height: %d\n", size, d->width, d->heigth);
         uint8_t header[] = {
		(size >> 24) & 0xFF,
		(size >> 16) & 0xFF,
		(size >>  8) & 0xFF,
		(size	  ) & 0xFF};
        send(fd, header, 4,0);
        send(fd, buf, size, 0);
       // hexDump("buf", &buf, size);
        free(buf);
        flatcc_builder_reset(B);

}

void initMatrix(int width, int heigth, int chain_length, struct DPODriver *d)
{
        
        struct sockaddr_in addr;
        fd = socket(AF_INET,SOCK_STREAM, 0);
        if(fd == -1){
          printf("Error opening Scoekt\n");
          exit(1);
        }
        addr.sin_family = AF_INET; 
        addr.sin_port = htons(PORT);
            if(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        exit(1); 
    } 
            if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
//        return -1; 
    }
              int flag = 1; 
setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

	printf("\nsocket opened\n");
        uint8_t *buf;
        size_t size;
        flatcc_builder_t builder,  *B;
        B = &builder;
        flatcc_builder_init(B);

        flatbuffers_string_ref_t name = flatbuffers_string_create_str(B, "Colorchord");
        
        ns(Register_ref_t) registerCommand = ns(Register_create(B, name, 150));
        ns(Command_union_ref_t) cmd = ns(Command_as_Register(registerCommand));
        hyperionnet_Request_create_as_root(B,cmd);
        //ns(Request_ref_t) realreq = hyperionnet_Request_create(B,cmd);
        buf = flatcc_builder_finalize_aligned_buffer(B, &size);
        hexDump("buf", &buf, size);
         uint8_t header[] = {
		(size >> 24) & 0xFF,
		(size >> 16) & 0xFF,
		(size >>  8) & 0xFF,
		(size	  ) & 0xFF};
        int a = send(fd, header, 4,0);
        int b = send(fd, buf, size, 0);
	printf("\nRegister message send\nMessage: %hhn, size: %ld, a: %d, b: %d\n", buf, size,a,b);
        free(buf);
        flatcc_builder_reset(B);
        char buffer[1024] = {0};
//        read( fd , buffer, 1024); 
 //   printf("Server: %s\n",buffer );


//	offscreen_canvas = led_matrix_create_offscreen_canvas(matrix);
//	led_canvas_get_size(offscreen_canvas, &width, &heigth);

	d->width = width;
	d->heigth = heigth;
}

static void DPOParams(void *id)
{
	printf("Init Display Matirx Pi driver\n");
	struct DPODriver *d = (struct DPODriver *)id;

	d->cols = 32;
	RegisterValue("panel_cols", PAINT, &d->cols, sizeof(d->cols));
	d->rows = 32;
	RegisterValue("panel_rows", PAINT, &d->rows, sizeof(d->rows));
	d->chain = 1;
	RegisterValue("chain", PAINT, &d->chain, sizeof(d->chain));

	initMatrix(d->cols, d->rows, d->chain, d);
}	

static struct DriverInstances *DisplayFlatbuf(const char *parameters)
{
	struct DriverInstances *ret = malloc(sizeof(struct DriverInstances));
	struct DPODriver *d = ret->id = malloc(sizeof(struct DPODriver));
	memset(d, 0, sizeof(struct DPODriver));
	ret->Func = DPOUpdate;
	ret->Params = DPOParams;
	DPOParams(d);
	return ret;
}

REGISTER_OUT_DRIVER(DisplayFlatbuf);

