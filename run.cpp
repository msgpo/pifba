// Run module
#include "burner.h"
#include "gamewidget.h"
#include "snd.h"
#include "font.h"
#include "gp2xsdk.h"
#include "gp2xmemfuncs.h"
#include "config.h"

extern int fps;
extern unsigned int FBA_KEYPAD[4];
extern CFG_OPTIONS config_options;
extern void do_keypad();

static int VideoBufferWidth = 0;
static int VideoBufferHeight = 0;
static int PhysicalBufferWidth = 0;


//static unsigned short BurnVideoBuffer[384 * 224];	// think max enough
static unsigned short * BurnVideoBuffer = NULL;	// think max enough
static bool BurnVideoBufferAlloced = false;

static unsigned short * BurnVideoBufferSave = NULL;	// think max enough

bool bShowFPS = true;

int InpMake(unsigned int[]);
void uploadfb(void);
void VideoBufferUpdate(void);
void VideoTrans();

int RunReset()
{
//	if (VideoBufferWidth == 384 && (config_options.option_rescale == 2 || VideoBufferHeight == 240))
//    {
//        gp2x_deinit();
//		gp2x_setvideo_mode(384,240);
//    }
//	else
//	if (VideoBufferWidth == 448)
//    {
//        gp2x_deinit();
//		gp2x_setvideo_mode(448,240);
//    }
    
    //SQ Rom loading screen uses different video size to the actual game
    //so close the screen and GLES and reinitialise.
    gp2x_deinit();
    
    //Burn rotates vertical games internally and as we rotate the bitmap
    //after Burn has drawn it but before we draw to GLES we need to
    //swap the dimensions for our GLES initialisation.
    if(BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL)
        gp2x_setvideo_mode(VideoBufferHeight, VideoBufferWidth);
    else
        gp2x_setvideo_mode(VideoBufferWidth, VideoBufferHeight);
    
	nFramesEmulated = 0;
	nCurrentFrame = 0;
	nFramesRendered = 0;	
	
	return 0;
}

int RunOneFrame(bool bDraw, int fps) 
{
	do_keypad();
	InpMake(FBA_KEYPAD);

	nFramesEmulated++;
	nCurrentFrame++;
	
	pBurnDraw = NULL;
	if ( bDraw )
	{
		nFramesRendered++;
		VideoBufferUpdate();
		pBurnDraw = (unsigned char *)&BurnVideoBuffer[0];
	}
	
	BurnDrvFrame();
	
	pBurnDraw = NULL;
	if ( bDraw )
	{
		VideoTrans();
		if (bShowFPS)
		{
			char buf[10];
			int x;
			sprintf(buf, "FPS: %2d", fps);
			//draw_text(buf, x, 0, 0xBDF7, 0x2020);
			DrawRect((uint16 *) (unsigned short *) &VideoBuffer[0],0, 0, 60, 9, 0,PhysicalBufferWidth);
			DrawString (buf, (unsigned short *) &VideoBuffer[0], 0, 0,PhysicalBufferWidth);
		}
		gp2x_video_flip();
	}
/*	if (config_options.option_sound_enable)
		SndPlay();
*/	return 0;
}

static unsigned int HighCol16(int r, int g, int b, int  /* i */)
{
	unsigned int t;

	t  = (r << 8) & 0xF800;
	t |= (g << 3) & 0x07E0;
	t |= (b >> 3) & 0x001F;

	return t;
}

static void BurnerVideoTransDemo(){}

static void (*BurnerVideoTrans) () = BurnerVideoTransDemo;

static void BurnerVideoCopy()
{
	unsigned short * p = VideoBuffer;
	unsigned short * q = BurnVideoBuffer;
    gp2x_memcpy (p, q, nBurnPitch*VideoBufferHeight);
}

static void BurnerVideoDoNothing()
{
    return;
}

//Rotate the bitmap 270 degrees
static void BurnerVideoRotate()
{
    register unsigned short *pD = VideoBuffer;
    register unsigned short *pS = BurnVideoBuffer;
    
    unsigned int r, c, dr, dc;
    int row = VideoBufferHeight;
    int col = VideoBufferWidth;
    for(r = 0; r < row; r++)
    {
        dr = VideoBufferHeight-r;

        for(c = 0; c < col; c++)
        {
            dc = VideoBufferWidth-c;
            
            ((unsigned short*)pD)[dc * row + (row - dr )] = ((unsigned short*)pS)[(r * col + c)];
        }
    }
    
}

int VideoInit()
{
	BurnDrvGetFullSize(&VideoBufferWidth, &VideoBufferHeight);
	
//sq
	printf("Screen Size: %d x %d\n", VideoBufferWidth, VideoBufferHeight);
	
	nBurnBpp = 2;
	BurnHighCol = HighCol16;
	
	BurnRecalcPal();
	
    BurnVideoBufferAlloced = false;
    nBurnPitch  = VideoBufferWidth * 2;
    
    if(BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL)
    {
        BurnerVideoTrans = BurnerVideoRotate;
        BurnVideoBuffer = (unsigned short *)malloc( VideoBufferWidth * VideoBufferHeight * 2 );
        BurnVideoBufferSave = BurnVideoBuffer;
        PhysicalBufferWidth = VideoBufferHeight;
    }
    else {
        BurnerVideoTrans = BurnerVideoDoNothing;
    	BurnVideoBuffer = VideoBuffer;
        PhysicalBufferWidth = VideoBufferWidth;
    }
		
	return 0;
}

// 'VideoBuffer' is updated each frame due to double buffering, so we sometimes need to ensure BurnVideoBuffer is updated too.
void VideoBufferUpdate (void)
{   
    if(BurnVideoBufferSave)
        BurnVideoBuffer = BurnVideoBufferSave;
    else
        BurnVideoBuffer = VideoBuffer;
}

void VideoTrans()
{
	BurnerVideoTrans();
}

void VideoExit()
{
	if ( BurnVideoBufferAlloced )
		free(BurnVideoBuffer);
	BurnVideoBuffer = NULL;
	BurnVideoBufferAlloced = false;
	BurnerVideoTrans = BurnerVideoTransDemo;
}

void ChangeFrameskip()
{
	bShowFPS = !bShowFPS;
//	DrawRect((uint16 *) (unsigned short *) &VideoBuffer[0],0, 0, 60, 9, 0,VideoBufferWidth);
	gp2x_clear_framebuffers();
	nFramesRendered = 0;
}
