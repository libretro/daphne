/*  SDL_Console.c
 *  Written By: Garrett Banuk <mongoose@wpi.edu>
 *  This is free, just be sure to give me credit when using it
 *  in any of your programs.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include "SDL_DrawText.h"


#ifdef WIN32
#pragma warning (disable:4244)	// disable the warning about possible loss of data
#endif

static int		TotalFonts = 0;
/* Linked list of fonts */
static BitFont *BitFonts = NULL;

int LoadFontFromMemory(const char *src, int size,
      int flags, SDL_PixelFormat * pnSurfaceFormat)
{
	int		FontNumber = 0;
	BitFont	**CurrentFont = &BitFonts;
	SDL_Surface	*Temp;
   SDL_RWops *rw = SDL_RWFromConstMem(src, size);
	
	while(*CurrentFont)
	{
		CurrentFont = &((*CurrentFont)->NextFont);
		FontNumber++;
	}
	
	/* load the font bitmap */
	if(NULL ==  (Temp = SDL_LoadBMP_RW(rw, 1)))
	{
		printf("Error Cannot load binary file\n");
		return -1;
	}

	/* Add a font to the list */
	*CurrentFont = (BitFont*)malloc(sizeof(BitFont));

	// RJS CHANGE START
	// (*CurrentFont)->FontSurface = SDL_DisplayFormat(Temp);
	(*CurrentFont)->FontSurface = SDL_ConvertSurface(Temp, pnSurfaceFormat, 0);
	// RJS CHANGE END
	SDL_FreeSurface(Temp);

	(*CurrentFont)->CharWidth = (*CurrentFont)->FontSurface->w/256;
	(*CurrentFont)->CharHeight = (*CurrentFont)->FontSurface->h;
	(*CurrentFont)->FontNumber = FontNumber;
	(*CurrentFont)->NextFont = NULL;

	TotalFonts++;

	/* Set font as transparent if the flag is set */
	if(flags & TRANS_FONT)
	{
		/* This line was left in in case of problems getting the pixel format */
		/* SDL_SetColorKey((*CurrentFont)->FontSurface, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0xFF00FF); */
		// RJS CHANGE
		// SDL_SetColorKey((*CurrentFont)->FontSurface, SDL_SRCCOLORKEY | SDL_RLEACCEL,
		// 	(*CurrentFont)->FontSurface->format->Rmask | (*CurrentFont)->FontSurface->format->Bmask);
		SDL_SetColorKey((*CurrentFont)->FontSurface, SDL_TRUE | SDL_RLEACCEL,
			(*CurrentFont)->FontSurface->format->Rmask | (*CurrentFont)->FontSurface->format->Bmask);
	}
	
	return FontNumber;
}

/* Returns a pointer to the font struct of the number
 * returns NULL if theres an error
 */
static BitFont* FontPointer(int FontNumber)
{
	int		fontamount = 0;
	BitFont	*CurrentFont = BitFonts;


	while(fontamount<TotalFonts)
	{
		if(CurrentFont->FontNumber == FontNumber)
			return CurrentFont;
		else
		{
			CurrentFont = CurrentFont->NextFont;
			fontamount++;
		}
	}
	
	return NULL;

}

/* Takes the font type, coords, and text to draw to the surface*/
void SDLDrawText(const char *string, SDL_Surface *surface, int FontType, int x, int y )
{
	int			loop;
	int			characters;
	SDL_Rect	SourceRect, DestRect;
	BitFont		*CurrentFont;
	
	CurrentFont = FontPointer(FontType);
	// RJS ADD - doc'd out initially
	// if (CurrentFont == NULL) return;

	/* see how many characters can fit on the screen */
	if(x>surface->w || y>surface->h) return;

	if(strlen(string) < (unsigned int) (surface->w-x)/CurrentFont->CharWidth)
		characters = strlen(string);
	else
	{
		characters = (surface->w - x) / CurrentFont->CharWidth;
	}

	DestRect.x = x;
	DestRect.y = y;
	DestRect.w = CurrentFont->CharWidth;
	DestRect.h = CurrentFont->CharHeight;

	SourceRect.y = 0;
	SourceRect.w = CurrentFont->CharWidth;
	SourceRect.h = CurrentFont->CharHeight;

	/* Now draw it */
	for(loop=0; loop<characters; loop++)
	{
		SourceRect.x = string[loop] * CurrentFont->CharWidth;
		SDL_BlitSurface(CurrentFont->FontSurface, &SourceRect, surface, &DestRect);
		DestRect.x += CurrentFont->CharWidth;
	}

}
