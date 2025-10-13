#include <stdio.h>
#include <stdlib.h>
#include <ogc/machine/processor.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <string.h>
#include <di/di.h>
#include "ios.h"

#define AHBPROT_DISABLED (*(vu32*)0xcd800064 == 0xFFFFFFFF)

DI_DriveID DI_id;

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

extern int __CONF_GetTxt(const char *name, char *buf, int length);

#define VER "1.0"

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

u16 get_tmd_version(u64 title) { // From the homebrew channel
    STACK_ALIGN(u8, tmdbuf, 1024, 32);
    u32 tmd_view_size = 0;
    s32 res;

    res = ES_GetTMDViewSize(title, &tmd_view_size);

    if (res < 0) return 0;

    if (tmd_view_size > 1024) return 0;

    ES_GetTMDView(title, (tmd_view*)tmdbuf, tmd_view_size);

    if (res < 0) return 0;

    return (tmdbuf[88] << 8) | tmdbuf[89];
}

float GetSysMenuNintendoVersion(u32 sysVersion) { // From SysCheck
	float ninVersion = 0.0;

	switch (sysVersion)
	{
		case 33:
			ninVersion = 1.0f;
		break;

		case 97:
		case 128:
		case 130:
			ninVersion = 2.0f;
		break;

		case 162:
			ninVersion = 2.1f;
		break;

		case 192:
		case 193:
		case 194:
			ninVersion = 2.2f;
		break;

		case 224:
		case 225:
		case 226:
			ninVersion = 3.0f;
		break;

		case 256:
		case 257:
		case 258:
			ninVersion = 3.1f;
		break;

		case 288:
		case 289:
		case 290:
			ninVersion = 3.2f;
		break;

		case 352:
		case 353:
		case 354:
		case 326:
			ninVersion = 3.3f;
		break;

		case 384:
		case 385:
		case 386:
			ninVersion = 3.4f;
		break;

		case 390:
			ninVersion = 3.5f;
		break;

		case 416:
		case 417:
		case 418:
			ninVersion = 4.0f;
		break;

		case 448:
		case 449:
		case 450:
		case 454:
		case 54448:
		case 54449:
		case 54450:
		case 54454:
			ninVersion = 4.1f;
		break;

		case 480:
		case 481:
		case 482:
		case 486:
			ninVersion = 4.2f;
		break;

		case 512:
		case 513:
		case 514:
		case 518:
		case 544:
		case 545:
		case 546:
		case 608:
		case 609:
		case 610:
			ninVersion = 4.3f;
		break;
	}

	return ninVersion;
}

char GetSysMenuRegion(u32 sysVersion) { // From SysCheck
	switch(sysVersion)
	{
		case 1:  //Pre-launch
		case 97: //2.0U
		case 193: //2.2U
		case 225: //3.0U
		case 257: //3.1U
		case 289: //3.2U
		case 353: //3.3U
		case 385: //3.4U
		case 417: //4.0U
		case 449: //4.1U
		case 54449: // mauifrog 4.1U
		case 481: //4.2U
		case 513: //4.3U
		case 545:
		case 609:
			return 'U';
		break;
		
		case 130: //2.0E
		case 162: //2.1E
		case 194: //2.2E
		case 226: //3.0E
		case 258: //3.1E
		case 290: //3.2E
		case 354: //3.3E
		case 386: //3.4E
		case 418: //4.0E
		case 450: //4.1E
		case 54450: // mauifrog 4.1E
		case 482: //4.2E
		case 514: //4.3E
		case 546:
		case 610:
			return 'E';
		break;

		case 128: //2.0J
		case 192: //2.2J
		case 224: //3.0J
		case 256: //3.1J
		case 288: //3.2J
		case 352: //3.3J
		case 384: //3.4J
		case 416: //4.0J
		case 448: //4.1J
		case 54448: // mauifrog 4.1J
		case 480: //4.2J
		case 512: //4.3J
		case 544:
		case 608:
			return 'J';
		break;
		
		case 326: //3.3K
		case 390: //3.5K
		case 454: //4.1K
		case 54454: // mauifrog 4.1K
		case 486: //4.2K
		case 518: //4.3K
			return 'K';
		break;
	}
	return 'X';
}

const u32 RGB2YCBCR(u8 r1, u8 g1, u8 b1) {
	u8 r2 = r1; u8 g2 = g1; u8 b2 = b1;
	if (r1 < 16) r1 = 16;
	if (g1 < 16) g1 = 16;
	if (b1 < 16) b1 = 16;
	if (r2 < 16) r2 = 16;
	if (g2 < 16) g2 = 16;
	if (b2 < 16) b2 = 16;

	if (r1 > 240) r1 = 240;
	if (g1 > 240) g1 = 240;
	if (b1 > 240) b1 = 240;
	if (r2 > 240) r2 = 240;
	if (g2 > 240) g2 = 240;
	if (b2 > 240) b2 = 240;

	u8 Y1 = ( 77 * r1 + 150 * g1 + 29 * b1) / 256;
	u8 Y2 = ( 77 * r2 + 150 * g2 + 29 * b2) / 256;
	u8 Cb = (112 * (b1 + b2) -  74 * (g1 + g2) - 38 * (r1 + r2)) / 512 + 128;
	u8 Cr = (112 * (r1 + r2) - 94 * (g1 + g2) - 18 * (b1 + b2)) / 512 + 128;

	return Y1 << 24 | Cb << 16 | Y2 << 8 | Cr;
}

void writetoxfb(void* videoBuffer, u32 offset, u32 length, u32 color)
{
	u32 *p = ((u32*)videoBuffer) + offset;
	for(u32 i = 0; i < length; i++) {
		*p++ = color;
	}
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	// Initialise the video system
	VIDEO_Init();

	// This function initialises the attached controllers
	WPAD_Init();

	bool ahbprot = disable_ahbprot();

	if (!AHBPROT_DISABLED) exit(0);

	CONF_Init();

	u16 SMVER = get_tmd_version(0x0000000100000002);

	u8 nickname[11];
	char drivedate[15] = {0};
	char sernumber[11];
	char sernumberprefix[4];
	char model[14];
	u32 boot2ver = 0;
	u32 numoftitles = 0;
	if (ahbprot) { // A wise man once told me that AHBPROT should be absent for homebrew to prosper
		DI_Init();
		if(!DI_Identify(&DI_id)) {
			sprintf(drivedate, "%8X", DI_id.rel_date); // Prints it as "YYYYMMDD"
			for(int i = 0; i < 4; i++) {
				drivedate[8-i] = drivedate[7-i]; // Makes it "YYYY MMDD" to add the first '/'
			}
			for(int i = 0; i < 2; i++) {
				drivedate[9-i] = drivedate[8-i]; // Makes it "YYYY MM DD" to add the second '/'
			}
			drivedate[4] = '/'; // Puts the first slash
			drivedate[7] = '/'; // Puts the second slash
			drivedate[10] = 0; // Remember kids, Null termination is very important when messing with strings
		}
		DI_Close();
	}

	ES_GetNumTitles(&numoftitles);
	ES_GetBoot2Version(&boot2ver);

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
	console_init(xfb,16,16,rmode->fbWidth-16,rmode->xfbHeight-16,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
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

	printf("\x1b[3;%dH",31);

	printf("NiioFetch %s", VER);

	printf ("\x1b[7;0H");
	printf("\n .==         .==.        ==.   .===.    .===.");
	printf("\n.====-      -====.      ====+ .=====   .=====");
	printf("\n .====.    .======     .====.  .===     .===");
	printf("\n  ====-    =======-   .===== ");
	printf("\n  .====.  .===-====.  .====.   =====    =====");
	printf("\n   ====-  ====..===-  -===-    =====    =====");
	printf("\n   .==== .===-..====..====.    =====    =====");
	printf("\n    ====-====.  .===-====:     =====    =====");
	printf("\n    .=======-    +=======      =====    =====");
	printf("\n     =======     .======:      =====    =====");
	printf("\n      =====.      .====-       =====    =====");

	printf ("\x1b[6;47H Running on IOS : %d", IOS_GetVersion());
	printf ("\x1b[7;47H CPU : IBM PowerPC 750CL");
	printf ("\x1b[8;47H System Menu : %.1f%c", GetSysMenuNintendoVersion(SMVER), GetSysMenuRegion(SMVER));
	printf ("\x1b[9;47H Boot2 : v%d", boot2ver);
	printf ("\x1b[10;47H Drive Date : %s", drivedate);
	printf ("\x1b[11;47H Resolution : %dx%d", rmode->viWidth, rmode->viHeight);

	printf ("\x1b[12;47H Nickname : %s", nickname);
	printf ("\x1b[13;47H Wii Model : %s", model);
	printf ("\x1b[14;47H S/N : %s%s", sernumberprefix, sernumber);
	
	printf ("\x1b[15;47H Region : %s", regions[CONF_GetRegion()]);
	printf ("\x1b[16;47H Language : %s", languages[CONF_GetLanguage()]);

	printf ("\x1b[17;47H Titles installed  : %d", numoftitles);	


	for(int i = 400; i < 416; i++) {
		writetoxfb(xfb, 160 + i*320, 12, COLOR_BLACK);
		writetoxfb(xfb, 160 + 12 + i*320, 12,  RGB2YCBCR(192, 0, 0));
		writetoxfb(xfb, 160 + 24 + i*320, 12,  RGB2YCBCR(0, 200, 0));
		writetoxfb(xfb, 160 + 36 + i*320, 12,  RGB2YCBCR(200, 200, 16));
		writetoxfb(xfb, 160 + 48 + i*320, 12,  RGB2YCBCR(16, 32, 192));
		writetoxfb(xfb, 160 + 60 + i*320, 12,  RGB2YCBCR(160, 16, 240));
		writetoxfb(xfb, 160 + 72 + i*320, 12,  RGB2YCBCR(16, 200, 200));
		writetoxfb(xfb, 160 + 84 + i*320, 12,  RGB2YCBCR(190, 190, 190));
	}
	for(int i = 416; i < 432; i++) {
		writetoxfb(xfb, 160 + i*320, 12, RGB2YCBCR(64, 64, 64));
		writetoxfb(xfb, 160 + 12 + i*320, 12,  RGB2YCBCR(255, 0, 0));
		writetoxfb(xfb, 160 + 24 + i*320, 12,  RGB2YCBCR(0, 255, 0));
		writetoxfb(xfb, 160 + 36 + i*320, 12,  RGB2YCBCR(255, 255, 64));
		writetoxfb(xfb, 160 + 48 + i*320, 12,  RGB2YCBCR(16, 64, 255));
		writetoxfb(xfb, 160 + 60 + i*320, 12,  RGB2YCBCR(180, 52, 240));
		writetoxfb(xfb, 160 + 72 + i*320, 12,  RGB2YCBCR(64, 240, 240));
		writetoxfb(xfb, 160 + 84 + i*320, 12,  COLOR_WHITE);
	}

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
