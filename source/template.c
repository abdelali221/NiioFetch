#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <string.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

extern int __CONF_GetTxt(const char *name, char *buf, int length);

#define VER "0.1"

const char *languages[] = {
	"Japanese",
	"English",
	"German",
	"French",
	"Italian",
	"Dutch",
	"Chinese (Simplified)",
	"Chinese (Traditonal)",
	"Korean"
};

const char *regions[] = {
	"Japan",
	"USA",
	"Europe",
	"NULL",
	"Korea",
	"China"	
};

void get_wii_model(char* model) {
    s32 ret = __CONF_GetTxt("MODEL", model, 13);  // Always 12 characters
    if (ret < 0) {
        // Failed to get MODEL
        strcpy(model, "????????????");
    }
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	// Initialise the video system
	VIDEO_Init();

	// This function initialises the attached controllers
	WPAD_Init();

	CONF_Init();

	u8 nickname[11];
	char sernumber[11];
	char sernumberprefix[4];
	char model[14];

	CONF_GetNickName(nickname);
	__CONF_GetTxt("CODE", sernumberprefix, 4);
	__CONF_GetTxt("SERNO", sernumber, 10);
	__CONF_GetTxt("MODEL", model, 13);

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth-20,rmode->xfbHeight-20,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	//SYS_STDIO_Report(true);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Clear the framebuffer
	VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);

	// Make the display visible
	VIDEO_SetBlack(false);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	printf("\x1b[0;%dH",30);

	printf("NiioFetch %s", VER);

	printf ("\x1b[5;0H");
	printf("\n@@@@       @@       @@@@  @@@@   @@@@");
	printf("\n @@@      @@@@      @@@   @@@@   @@@@");
	printf("\n @@@@    @@@@@@    @@@@");
	printf("\n  @@@@   @@@ @@@   @@@    @@@@   @@@@");
	printf("\n  @@@@  @@@  @@@  @@@@    @@@@   @@@@");
	printf("\n   @@@  @@@   @@@ @@@     @@@@   @@@@");
	printf("\n    @@@@@@    @@@@@@@     @@@@   @@@@");
	printf("\n    @@@@@@     @@@@@      @@@@   @@@@");
	printf("\n     @@@@      @@@@       @@@@   @@@@");

	printf ("\x1b[4;44H Running on IOS : %d", IOS_GetVersion());
	printf ("\x1b[6;44H CPU : IBM PowerPC 750CL");
	printf ("\x1b[8;44H Resolution : %dx%d", rmode->viWidth, rmode->viHeight);

	printf ("\x1b[10;44H Nickname : %s", nickname);
	printf ("\x1b[12;44H Wii Model : %s", model);
	printf ("\x1b[14;44H S/N : %s%s", sernumberprefix, sernumber);
	
	printf ("\x1b[16;44H Region : %s", regions[CONF_GetRegion()]);
	printf ("\x1b[18;44H Language : %s", languages[CONF_GetLanguage() ]);
	

	while(1) {

		// Call WPAD_ScanPads each loop, this reads the latest controller states

		WPAD_ScanPads();

		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0);

		// We return to the launcher application via exit
		if ( pressed & WPAD_BUTTON_HOME ) {
			exit(0);
		}

		// Wait for the next frame
		VIDEO_WaitVSync();
	}

	return 0;
}
