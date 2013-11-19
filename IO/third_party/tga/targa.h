/*
 * targa.h
 *
 * Copyright (C) 2006 - 2009 by Joshua S. English.
 *
 * This software is the intellectual property of Joshua S. English. This
 * software is provided 'as-is', without any express or implied warranty. In no
 * event will the author be held liable for any damages arising from the use of
 * this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software in
 *    a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source version must be plainly marked as such, and must not be
 *    misrepresented as being original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *
 * Notes:
 *
 * A plugin to read targa (TGA) image files into an OpenGL-compatible RGBA
 * format, header file.
 *
 * Written by Joshua S. English.
 *
 *
 * The TARGA Specification:
 *
 *	Position:	Length:		Description:
 *	--------	------		-----------
 *	0			1			length of the image ID
 *	1			1			type of color map included (if any)
 *								0	=>	no color map
 *								1	=>	has color map
 *	2			1			image type
 *								0	=>	no image data
 *								1	=>	color-mapped image
 *								2	=>	true-color image
 *								3	=>	black & white image
 *								9	=>	RLE color-mapped image
 *								10	=>	RLE true-color image
 *								11	=>	RLE black & white image
 *	3			2			index of the first color-map entry as an offest
 *							into the color-map table
 *	5			2			color-map length (number of entries)
 *	7			1			color-map entry length - (number of bits per entry)
 *	8			2			x-origin of image
 *	10			2			y-origin of image
 *	12			2			image width in pixels
 *	14			2			image height in pixels
 *	16			1			pixel depth - the number of bits per pixel
 *	17			1			image descriptor
 *	n			var			image id - only exists if non-zero
 *	n			var			color-map data - only exists if non-zero
 *	n			var			image data
 */

#if !defined(_TARGA_H)

#define _TARGA_H


// define targa public constants

#define TARGA_COLOR_RED										1
#define TARGA_COLOR_GREEN									2
#define TARGA_COLOR_BLUE									3
#define TARGA_COLOR_ALPHA									4


// define targa public data types

typedef struct _Targa {
	int width;
	int height;
	int imageLength;
	unsigned char *image;
} Targa;


// declare targa public functions

/**
 * targa_init()
 *
 * Initilize the Targa structure for utilization.
 *
 * @param	targa(in)	The Targa struct to initialize.
 *
 * @return	An integer where zero is pass, less than zero is failure.
 */
int targa_init(Targa *targa);


/**
 * targa_free()
 *
 * Free the Targa structure.
 *
 * @param	targa(in)	The Targa struct to free.
 *
 * @return	An integer where zero is pass, less than zero is failure.
 */
int targa_free(Targa *targa);


/**
 * targa_getDimensions()
 *
 * Obtain the width and height in pixels of a loaded Targa image.
 *
 * @param	targa(in)	The Targa struct of a loaded image.
 *
 * @param	width(out)	The width in pixels of a loaded Targa image.
 *
 * @param	height(out)	The height in pixels of a loaded Targa image.
 *
 * @return	An integer where zero is pass, less than zero is failure.
 */
int targa_getDimensions(Targa *targa, int *width, int *height);


/**
 * targa_getImageLength()
 *
 * Obtain the length in bytes of the serialized RGBA image.
 *
 * @param	targa(in)			The Targa struct of a loaded image.
 *
 * @param	imageLength(out)	The length in bytes of the RGBA image.
 *
 * @return	An integer where zero is pass, less than zero is failure.
 */
int targa_getImageLength(Targa *targa, int *imageLength);


/**
 * targa_getRgbaTexture()
 *
 * Obtain the serialized RGBA texture and its' length from a Targa image.
 *
 * @param	targa(in)			The Targa struct of a loaded image.
 *
 * @param	texture(out)		The serialized RGBA image pointer.
 *
 * @param	textureLength(out)	The serialized RGBA image length in bytes.
 *
 * @return	An integer where zero is pass, less than zero is failure.
 */
int targa_getRgbaTexture(Targa *targa, char **texture, int *textureLength);


/**
 * targa_loadFromFile()
 *
 * Load a targa file and decode it into a 32-bit RGBA serialized image.
 *
 * @param	targa(in)		The Targa struct of an image to load.
 *
 * @param	filename(in)	The filename of the image to load.
 *
 * @return	An integer where zero is pass, less than zero is failure.
 */
int targa_loadFromFile(Targa *targa, char *filename);


/**
 * targa_loadFromData()
 *
 * Load the targa from an in-memory location and decode it into a 32-bit RGBA
 * serialize image.
 *
 * @param	targa(in)		The Targa struct an image to load.
 *
 * @param	data(in)		A pointer to an in-memory location containing a
 *							Targa image.
 *
 * @param	dataLength(in)	The length of the Targa in-memory image.
 *
 * @return	An integer where zero is pass, less than zero is failure.
 */
int targa_loadFromData(Targa *targa, unsigned char *data, int dataLength);


/**
 * targa_applyRgbaMask()
 *
 * Apply a red, green, blue or alpha-channel additive color-mask to a loaded
 * Targa image.
 *
 * @param	targa(in)		The Targa struct of a loaded image.
 *
 * @param	colorType(in)	The color channel to mask.
 *
 * @param	value(in)		The color code (0 - 255) to mask.
 *
 * @return	An integer where zero is pass, less than zero is failure.
 */
int targa_applyRgbaMask(Targa *targa, int colorType, unsigned char value);


/**
 * targa_setRgbaChannel()
 *
 * Apply a red, green, blue or alpha-channel additive color-channel
 * replacement to a loaded Targa image.
 *
 * @param	targa(in)		The Targa struct of a loaded image.
 *
 * @param	colorType(in)	The color channel to replace.
 *
 * @param	value(in)		The color code (0 - 255) to replace.
 *
 * @return	An integer where zero is pass, less than zero is failure.
 */
int targa_setRgbaChannel(Targa *targa, int colorType, unsigned char value);


#endif // _TARGA_H

