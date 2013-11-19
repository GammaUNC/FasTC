/*
 * targa.c
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
 * format.
 *
 * Written by Joshua S. English.
 */

// preprocessor directives

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "targa.h"


// define targa private constants

#define TGA_COLOR_MAP_NONE							0

#define TGA_IMAGE_TYPE_NONE							0
#define TGA_IMAGE_TYPE_CM							1
#define TGA_IMAGE_TYPE_BGR							2
#define TGA_IMAGE_TYPE_BW							3
#define TGA_IMAGE_TYPE_RLE_CM						9
#define TGA_IMAGE_TYPE_RLE_BGR						10
#define TGA_IMAGE_TYPE_RLE_BW						11

#define TGA_R										0
#define TGA_G										1
#define TGA_B										2
#define TGA_A										3


// declare targa private functions

static int handleTargaError(FILE *fh, int errorCode, const char *function,
		size_t line);

static int ctoi(char value);

static char *localStrndup(char *string, int length);


// define targa private macros

#define targaErrorf() \
	handleTargaError(fh, rc, __FUNCTION__, __LINE__)


// define targa private functions

static int handleTargaError(FILE *fh, int errorCode, const char *function,
		size_t line)
{
	char *errorMessage = NULL;

	if((errorMessage = strerror(errno)) == NULL) {
		errorMessage = "uknown error";
	}

	fprintf(stderr, "[%s():%i] error(%i) - #%i, '%s'.\n",
			(char *)function, (int)line, errorCode, (int)errno, errorMessage);

	if(fh != NULL) {
		fclose(fh);
	}

	return -1;
}

static int ctoi(char value)
{
	return (int)((unsigned char)value);
}

static char *localStrndup(char *string, int length)
{
	char *result = NULL;

	if((string == NULL) || (length < 1)) {
		fprintf(stderr, "[%s():%i] error - invalid or missing argument(s).\n",
				__FUNCTION__, __LINE__);
		return NULL;
	}

#if defined(_GNU_SOURCE)
	result = strndup(string, length);
#else // !_GNU_SOURCE
	result = (char *)malloc(sizeof(char) * (length + 1));
	memset(result, 0, (sizeof(char) * (length + 1)));
	memcpy(result, string, length);
#endif // _GNU_SOURCE

	return result;
}


// define targa public functions

int targa_init(Targa *targa)
{
	if(targa == NULL) {
		fprintf(stderr, "[%s():%i] error - invalid or missing argument(s).\n",
				__FUNCTION__, __LINE__);
		return -1;
	}

	memset((void *)targa, 0, sizeof(Targa));

	targa->width = 0;
	targa->height = 0;
	targa->imageLength = 0;
	targa->image = NULL;

	return 0;
}

int targa_free(Targa *targa)
{
	if(targa == NULL) {
		fprintf(stderr, "[%s():%i] error - invalid or missing argument(s).\n",
				__FUNCTION__, __LINE__);
		return -1;
	}

	if(targa->image != NULL) {
		free(targa->image);
	}

	memset((void *)targa, 0, sizeof(Targa));

	return 0;
}

int targa_getDimensions(Targa *targa, int *width, int *height)
{
	if((targa == NULL) || (width == NULL) || (height == NULL)) {
		fprintf(stderr, "[%s():%i] error - invalid or missing argument(s).\n",
				__FUNCTION__, __LINE__);
		return -1;
	}

	*width = targa->width;
	*height = targa->height;

	return 0;
}

int targa_getImageLength(Targa *targa, int *imageLength)
{
	if((targa == NULL) || (imageLength == NULL)) {
		fprintf(stderr, "[%s():%i] error - invalid or missing argument(s).\n",
				__FUNCTION__, __LINE__);
		return -1;
	}

	*imageLength = targa->imageLength;

	return 0;
}

int targa_getRgbaTexture(Targa *targa, char **texture, int *textureLength)
{
	if((targa == NULL) || (texture == NULL) || (textureLength == NULL)) {
		fprintf(stderr, "[%s():%i] error - invalid or missing argument(s).\n",
				__FUNCTION__, __LINE__);
		return -1;
	}

	*texture = (char *)targa->image;
	*textureLength = targa->imageLength;

	return 0;
}

int targa_loadFromFile(Targa *targa, char *filename)
{
	int rc = 0;
	int fileLength = 0;
	unsigned char *buffer = NULL;

	FILE *fh = NULL;

	if((targa == NULL) || (filename == NULL)) {
		fprintf(stderr, "[%s():%i] error - invalid or missing argument(s).\n",
				__FUNCTION__, __LINE__);
		return -1;
	}

	if((fh = fopen(filename, "r+b")) == NULL) {
		return targaErrorf();
	}

	if((rc = fseek(fh, 0, SEEK_END)) != 0) {
		return targaErrorf();
	}

	if((fileLength = ftell(fh)) < 0) {
		return targaErrorf();
	}

	if((rc = fseek(fh, 0, SEEK_SET)) != 0) {
		return targaErrorf();
	}

	if(fileLength < 18) {
		fprintf(stderr, "error - TGA file '%s' length %i invalid.\n",
				filename, fileLength);
		fclose(fh);
		return -1;
	}

	buffer = (unsigned char *)malloc(sizeof(unsigned char) * fileLength);
	memset(buffer, 0, (sizeof(unsigned char) * fileLength));

	rc = (int)fread((char *)buffer, sizeof(char), fileLength, fh);
	if(rc != fileLength) {
		return targaErrorf();
	}

	fclose(fh);

	rc = targa_loadFromData(targa, buffer, fileLength);

	free(buffer);

	return rc;
}

int targa_loadFromData(Targa *targa, unsigned char *data, int dataLength)
{
	short sNumber = 0;
	int ii = 0;
	int nn = 0;
	int imageIdLength = 0;
	int colorMap = 0;
	int imageType = 0;
	int bitLength = 0;
	int colorMode = 0;
	int length = 0;
	int rleId = 0;
	int pixel[4];
	unsigned char *imageId = NULL;
	unsigned char *ptr = NULL;

	if((targa == NULL) || (data == NULL) || (dataLength < 18)) {
		fprintf(stderr, "[%s():%i] error - invalid or missing argument(s).\n",
				__FUNCTION__, __LINE__);
		return -1;
	}

	ptr = data;

	// determine image ID length

	imageIdLength = (int)ptr[0];
	ptr++;
	if((int)(ptr - data) > dataLength) {
		fprintf(stderr, "[%s():%i] error - detected data overrun with %i vs "
				"%i.\n", __FUNCTION__, __LINE__, (int)(ptr - data),
				dataLength);
		return -1;
	}

	// check for color map

	colorMap = (int)ptr[0];
	ptr++;
	if((int)(ptr - data) > dataLength) {
		fprintf(stderr, "[%s():%i] error - detected data overrun with %i vs "
				"%i.\n", __FUNCTION__, __LINE__, (int)(ptr - data),
				dataLength);
		return -1;
	}
	if(colorMap != TGA_COLOR_MAP_NONE) {
		fprintf(stderr, "[%s():%i] error - unable to read TARGA color map "
				"%i.\n", __FUNCTION__, __LINE__, colorMap);
		return -1;
	}

	// obtain image type

	imageType = (int)ptr[0];
	ptr++;
	if((int)(ptr - data) > dataLength) {
		fprintf(stderr, "[%s():%i] error - detected data overrun with %i vs "
				"%i.\n", __FUNCTION__, __LINE__, (int)(ptr - data),
				dataLength);
		return -1;
	}
	if((imageType == TGA_IMAGE_TYPE_NONE) ||
			(imageType == TGA_IMAGE_TYPE_CM) ||
			(imageType == TGA_IMAGE_TYPE_RLE_CM)) {
		fprintf(stderr, "[%s():%i] error - unsupported image type %i.\n",
				__FUNCTION__, __LINE__, imageType);
		return -1;
	}

	// skip 9 bytes (color-map information and x & y origins)

	ptr += 9;
	if((int)(ptr - data) > dataLength) {
		fprintf(stderr, "[%s():%i] error - detected data overrun with %i vs "
				"%i.\n", __FUNCTION__, __LINE__, (int)(ptr - data),
				dataLength);
		return -1;
	}

	// obtain image width

	memcpy((char *)&sNumber, ptr, sizeof(short));
	ptr += sizeof(short);
	if((int)(ptr - data) > dataLength) {
		fprintf(stderr, "[%s():%i] error - detected data overrun with %i vs "
				"%i.\n", __FUNCTION__, __LINE__, (int)(ptr - data),
				dataLength);
		return -1;
	}
	if(sNumber < 1) {
		fprintf(stderr, "[%s():%i] error - invalid image width %i.\n",
				__FUNCTION__, __LINE__, (int)sNumber);
		return -1;
	}

	targa->width = (int)sNumber;

	// obtain image height

	memcpy((char *)&sNumber, ptr, sizeof(short));
	ptr += sizeof(short);
	if((int)(ptr - data) > dataLength) {
		fprintf(stderr, "[%s():%i] error - detected data overrun with %i vs "
				"%i.\n", __FUNCTION__, __LINE__, (int)(ptr - data),
				dataLength);
		return -1;
	}
	if(sNumber < 1) {
		fprintf(stderr, "[%s():%i] error - invalid image height %i.\n",
				__FUNCTION__, __LINE__, (int)sNumber);
		return -1;
	}

	targa->height = (int)sNumber;

	// determine pixel depth

	bitLength = (int)ptr[0];
	ptr++;
	if((int)(ptr - data) > dataLength) {
		fprintf(stderr, "[%s():%i] error - detected data overrun with %i vs "
				"%i.\n", __FUNCTION__, __LINE__, (int)(ptr - data),
				dataLength);
		return -1;
	}
	if((bitLength != 16) && (bitLength != 24) && (bitLength != 32)) {
		fprintf(stderr, "[%s():%i] error - unknown pixel depth of %i-bits.\n",
				__FUNCTION__, __LINE__, bitLength);
		return -1;
	}
	if((bitLength == 16) &&
			((imageType != TGA_IMAGE_TYPE_BGR) &&
			 (imageType != TGA_IMAGE_TYPE_BW))) {
		fprintf(stderr, "[%s():%i] error - unable to RLE-decode pixel depth "
				"of %i-bits.\n", __FUNCTION__, __LINE__, bitLength);
		return -1;
	}

	// skip 1 byte (image descriptor)

	ptr++;
	if((int)(ptr - data) > dataLength) {
		fprintf(stderr, "[%s():%i] error - detected data overrun with %i vs "
				"%i.\n", __FUNCTION__, __LINE__, (int)(ptr - data),
				dataLength);
		return -1;
	}

	// obtain the image ID

	if(imageIdLength > 0) {
		if(((int)(ptr - data) + imageIdLength) > dataLength) {
			fprintf(stderr, "[%s():%i] error - detected data overrun at %i "
					"(image ID) by %i bytes.\n", __FUNCTION__, __LINE__,
					(int)(ptr - data),
					(((int)(ptr - data) + imageIdLength) - dataLength));
			return -1;
		}
		imageId = (unsigned char *)localStrndup((char *)ptr, imageIdLength);
		ptr += imageIdLength;
		if((int)(ptr - data) > dataLength) {
			fprintf(stderr, "[%s():%i] error - detected data overrun with %i "
					"vs %i.\n", __FUNCTION__, __LINE__, (int)(ptr - data),
					dataLength);
			return -1;
		}
	}

	// process the image

	targa->imageLength = (long int)(targa->width * targa->height * 4);
	targa->image = (unsigned char *)malloc(sizeof(unsigned char) *
			targa->imageLength);

	if((imageType == TGA_IMAGE_TYPE_BGR) || (imageType == TGA_IMAGE_TYPE_BW)) {
		if(bitLength == 16) {
			colorMode = 2;
		}
		else {
			colorMode = (bitLength / 8);
		}
		length = (targa->width * targa->height * colorMode);
		if(((int)(ptr - data) + length) > dataLength) {
			fprintf(stderr, "[%s():%i] error - detected data overrun at %i "
					"(image pixels) by %i bytes.\n", __FUNCTION__, __LINE__,
					(int)(ptr - data),
					(((int)(ptr - data) + length) - dataLength));
			return -1;
		}
		for(ii = 0, nn = 0; ((ii < length) && (nn < targa->imageLength));
				ii += colorMode, nn += 4) {
			if(colorMode == 2) {
				memcpy((char *)&sNumber, ptr, sizeof(short));
				pixel[TGA_R] = ctoi((sNumber & 0x1f) << 3);
				pixel[TGA_G] = ctoi(((sNumber >> 5) & 0x1f) << 3);
				pixel[TGA_B] = ctoi(((sNumber >> 10) & 0x1f) << 3);
				pixel[TGA_A] = 255;
			}
			else {
				pixel[TGA_R] = ctoi(ptr[2]);
				pixel[TGA_G] = ctoi(ptr[1]);
				pixel[TGA_B] = ctoi(ptr[0]);
				if(colorMode == 3) {
					pixel[TGA_A] = 255;
				}
				else {
					pixel[TGA_A] = ctoi(ptr[3]);
				}
			}

			targa->image[(nn + 0)] = (unsigned char)pixel[TGA_R];
			targa->image[(nn + 1)] = (unsigned char)pixel[TGA_G];
			targa->image[(nn + 2)] = (unsigned char)pixel[TGA_B];
			targa->image[(nn + 3)] = (unsigned char)pixel[TGA_A];

			ptr += colorMode;
		}
	}
	else { // RLE image
		ii = 0;
		nn = 0;
		rleId = 0;
		colorMode = (bitLength / 8);
		length = (targa->width * targa->height);
		while(ii < length) {
			rleId = (int)ptr[0];
			ptr++;
			if((int)(ptr - data) > dataLength) {
				fprintf(stderr, "[%s():%i] error - detected data overrun with "
						"%i vs %i.\n", __FUNCTION__, __LINE__,
						(int)(ptr - data), dataLength);
				return -1;
			}

			if(rleId < 128) {
				rleId++;
				while(rleId > 0) {
					pixel[TGA_R] = ctoi(ptr[2]);
					pixel[TGA_G] = ctoi(ptr[1]);
					pixel[TGA_B] = ctoi(ptr[0]);
					if(colorMode == 3) {
						pixel[TGA_A] = 255;
					}
					else {
						pixel[TGA_A] = ctoi(ptr[3]);
					}

					targa->image[(nn + 0)] = (unsigned char)pixel[TGA_R];
					targa->image[(nn + 1)] = (unsigned char)pixel[TGA_G];
					targa->image[(nn + 2)] = (unsigned char)pixel[TGA_B];
					targa->image[(nn + 3)] = (unsigned char)pixel[TGA_A];

					rleId--;
					ii++;
					nn += 4;
					if(nn >= targa->imageLength) {
						break;
					}
					ptr += colorMode;
					if((int)(ptr - data) > dataLength) {
						fprintf(stderr, "[%s():%i] error - detected data "
								"overrun with %i vs %i.\n", __FUNCTION__,
								__LINE__, (int)(ptr - data), dataLength);
						return -1;
					}
				}
			}
			else {
				pixel[TGA_R] = ctoi(ptr[2]);
				pixel[TGA_G] = ctoi(ptr[1]);
				pixel[TGA_B] = ctoi(ptr[0]);
				if(colorMode == 3) {
					pixel[TGA_A] = 255;
				}
				else {
					pixel[TGA_A] = ctoi(ptr[3]);
				}
				ptr += colorMode;
				if((int)(ptr - data) > dataLength) {
					fprintf(stderr, "[%s():%i] error - detected data overrun "
							"with %i vs %i.\n", __FUNCTION__, __LINE__,
							(int)(ptr - data), dataLength);
					return -1;
				}

				rleId -= 127;
				while(rleId > 0) {
					targa->image[(nn + 0)] = (unsigned char)pixel[TGA_R];
					targa->image[(nn + 1)] = (unsigned char)pixel[TGA_G];
					targa->image[(nn + 2)] = (unsigned char)pixel[TGA_B];
					targa->image[(nn + 3)] = (unsigned char)pixel[TGA_A];

					rleId--;
					ii++;
					nn += 4;
					if(nn >= targa->imageLength) {
						break;
					}
				}
			}
			if(nn >= targa->imageLength) {
				break;
			}
		}
	}

	if(imageId != NULL) {
		free(imageId);
	}

	return 0;
}

int targa_applyRgbaMask(Targa *targa, int colorType, unsigned char value)
{
	int ii = 0;
	int startPosition = 0;

	if((targa == NULL) ||
			((colorType != TARGA_COLOR_RED) &&
			 (colorType != TARGA_COLOR_GREEN) &&
			 (colorType != TARGA_COLOR_BLUE) &&
			 (colorType != TARGA_COLOR_ALPHA))) {
		fprintf(stderr, "[%s():%i] error - invalid or missing argument(s).\n",
				__FUNCTION__, __LINE__);
		return -1;
	}

	switch(colorType) {
		case TARGA_COLOR_RED:
			startPosition = 0;
			break;
		case TARGA_COLOR_GREEN:
			startPosition = 1;
			break;
		case TARGA_COLOR_BLUE:
			startPosition = 2;
			break;
		case TARGA_COLOR_ALPHA:
			startPosition = 3;
			break;
	}

	for(ii = startPosition; ii < targa->imageLength; ii += 4) {
		targa->image[ii] += value;
	}

	return 0;
}

int targa_setRgbaChannel(Targa *targa, int colorType, unsigned char value)
{
	int ii = 0;
	int startPosition = 0;

	if((targa == NULL) ||
			((colorType != TARGA_COLOR_RED) &&
			 (colorType != TARGA_COLOR_GREEN) &&
			 (colorType != TARGA_COLOR_BLUE) &&
			 (colorType != TARGA_COLOR_ALPHA))) {
		fprintf(stderr, "[%s():%i] error - invalid or missing argument(s).\n",
				__FUNCTION__, __LINE__);
		return -1;
	}

	switch(colorType) {
		case TARGA_COLOR_RED:
			startPosition = 0;
			break;
		case TARGA_COLOR_GREEN:
			startPosition = 1;
			break;
		case TARGA_COLOR_BLUE:
			startPosition = 2;
			break;
		case TARGA_COLOR_ALPHA:
			startPosition = 3;
			break;
	}

	for(ii = startPosition; ii < targa->imageLength; ii += 4) {
		targa->image[ii] = value;
	}

	return 0;
}

