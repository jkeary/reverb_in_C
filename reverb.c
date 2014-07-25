/* 
** Problem Set 6 Part II
**
** reverb.c
**
** Takes three audio file names as arguments: 
**     reverb <audio input filename> <impulse response filename> <ouput filename>
**
** The audio input and impulse response must be the same sample rate; but either can
** be mono or stero.
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sndfile.h>
#include "convolve.h"

#define MONO 1
#define STEREO 2

typedef struct {
	SNDFILE* audioFile;
	SNDFILE* impulseResponse;
	SF_INFO audioFileInfo;
	SF_INFO impulseFileInfo;
} reverbData;

/* function prototype */
void processAudio(const char *audioFilename, const char *impulseFilename, const char *outfilename);

/**************************************************************************/
							/* Main Function */
/**************************************************************************/

int main (int argc, char **argv)
{
	
  if (argc != 4) {
    printf ("\nThree file names required as arguments: \n");
    printf ("    Usage : reverb <audio input filename> <impulse response filename> <ouput filename>\n");
    exit (1);
  }
  
  /* Add reverb to audio file specified by convolving it with impulse response specified */
  processAudio(argv[1], argv[2], argv[3]);
  
  return 0 ;
} 

/**************************************************************************/
							/* ProcessAudio Function */
/**************************************************************************/

// Given the name of an input audio file, impulse response file, and output file, read in the first two files, add reverberation to the input signal by convolving it with the impluse response, and output the results to the specified output file. 
void processAudio(const char *audioFilename, const char *impulseFilename, const char *outfilename)
{	
	
	reverbData data;
	
/**************************************************************************/
							/* Open audio files */
/**************************************************************************/
	
	// clear everything in SF_INFO structs to 0
	memset(&data.audioFileInfo, 0, sizeof(data.audioFileInfo));
	memset(&data.impulseFileInfo, 0, sizeof(data.impulseFileInfo));
							
	// open and read SNDFILEs, putting info from the file into the corresponding SF_INFO structs.  Error check files
	data.audioFile =  sf_open(audioFilename, SFM_READ, &data.audioFileInfo);
	if (data.audioFile == NULL) {
		printf("Error: couldn't open file: %s\n", audioFilename);
		puts(sf_strerror (NULL)) ; 
		return;
	}
	data.impulseResponse = sf_open(impulseFilename, SFM_READ, &data.impulseFileInfo);
	if (&data.impulseFileInfo == NULL) {
		printf("Error: couldn't open file: %s\n", impulseFilename);
		puts(sf_strerror (NULL)) ;
		return;
	}
	
	// if sampling rates differ, error message and quit
	printf("Audio file %s: Frames: %d Channels: %d Samplerate: %d\n", audioFilename, (int)data.audioFileInfo.frames, data.audioFileInfo.channels, data.audioFileInfo.samplerate);
	
	printf("Impulse response %s: Frames: %d Channels: %d Samplerate: %d\n", impulseFilename, (int)data.impulseFileInfo.frames, data.impulseFileInfo.channels, data.impulseFileInfo.samplerate);
	
	/* If sample rates don't match, exit */
	if (data.audioFileInfo.samplerate != data.impulseFileInfo.samplerate) {
		printf("Error: audio files must have same sample rates.\nExiting.\n");
		return;
	} 

/**************************************************************************/
							/* CONVOLVE */
/**************************************************************************/

// declare an int to iterate through everything.  Will return to 0 every time its done being used b/c its not a static variabe, so it can be used multiple times for iterating.
	int i = 0;
	
// IF BOTH ARE STEREO : then convolve Channel 1 of the input audio with Channel 1 of the impulse response, and Channel 2 of the input audio with Channel 2 of the impulse response.  
	if (data.audioFileInfo.channels == STEREO && data.impulseFileInfo.channels == STEREO) {
		printf("Both files are stereo.\n");
	// allocate memory for MONO Audio File left and right buffers
		float *audioBufferLeft = malloc((data.audioFileInfo.frames) * sizeof(float));
		if (audioBufferLeft == NULL) {
			fprintf(stderr, "Could not allocate memory for audio file\n");
			sf_close(data.audioFile);
			return;
		}
		float *audioBufferRight = malloc((data.audioFileInfo.frames) * sizeof(float));
		if (audioBufferRight == NULL) {
			fprintf(stderr, "Could not allocate memory for audio file\n");
			sf_close(data.audioFile);
			return;
		}
	// allocate memory for MONO Impulse Response left and right buffers
		float *impulseBufferLeft = malloc((data.impulseFileInfo.frames) * sizeof(float));
		if (impulseBufferLeft == NULL) {
			fprintf(stderr, "Could not allocate memory for impulse response\n");
			sf_close(data.impulseResponse);
			return;
		}
		float *impulseBufferRight = malloc((data.impulseFileInfo.frames) * sizeof(float));
		if (impulseBufferRight == NULL) {
			fprintf(stderr, "Could not allocate memory for impulse response\n");
			sf_close(data.impulseResponse);
			return;
		}
	// put samples in buffers to make mono left and right channel buffers
		for (i = 0; i < data.audioFileInfo.frames; i++) {
			sf_read_float(data.audioFile, &audioBufferLeft[i], 1);
			sf_read_float(data.audioFile, &audioBufferRight[i], 1);
		}
		for (i = 0; i < data.impulseFileInfo.frames; i++) {
			sf_read_float(data.impulseResponse, &impulseBufferLeft[i], 1);
			sf_read_float(data.impulseResponse, &impulseBufferRight[i], 1);
		}
	// get lengths
		int audioBufferLeftLen = sizeof(audioBufferLeft);
		int audioBufferRightLen = sizeof(audioBufferRight);
		int impulseBufferLeftLen = sizeof(impulseBufferLeft);
		int impulseBufferRightLen = sizeof(impulseBufferRight);		
	// init ptr to output buffer
		float *outputBufferLeft = NULL;
		float *outputBufferRight = NULL;		
	// convolve left, return size of left output
		int leftOutLen = convolve(audioBufferLeft, impulseBufferLeft, audioBufferLeftLen, impulseBufferLeftLen, &outputBufferLeft);
	// convolve right, return size of right output
		int rightOutLen = convolve(audioBufferRight, impulseBufferRight, audioBufferRightLen, impulseBufferRightLen, &outputBufferRight);
	// allocate memory for STEREO output		
		float *output = malloc((leftOutLen + rightOutLen) * sizeof(float));
		if (output == NULL) {
			fprintf(stderr, "Could not allocate memory for output audio file, something went terribly wrong!\n");
			free(output);
			return;
		}
	// put the samples into output to make STEREO array and count length
		int outLength = 1;
		for (i = 0; i < (leftOutLen + rightOutLen); i++) {
			sf_read_float(&outputBufferLeft, &output[i], 1);
			outLength++;
			sf_read_float(&outputBufferRight, &output[i], 1);
			outLength++;
		}
	// free allocated not in use memory	
		free(audioBufferLeft);
		free(audioBufferRight);
		free(impulseBufferLeft);
		free(impulseBufferRight);
	}

// IF AUDIO IS STEREO AND IR IS MONO : convolve each channel of the input audio with the mono signal of the impulse response. 
	if (data.audioFileInfo.channels == STEREO && data.impulseFileInfo.channels == MONO) {
		printf("%s is in stereo, and %s is in mono.\n", data.audioFile, data.impulseResponse);
	// allocate memory for MONO Audio File left and right buffers but not for already MONO IR
		float *audioBufferLeft = malloc((data.audioFileInfo.frames) * sizeof(float));
		if (audioBufferLeft == NULL) {
			fprintf(stderr, "Could not allocate memory for audio file\n");
			sf_close(data.audioFile);
			return;
		}
		float *audioBufferRight = malloc((data.audioFileInfo.frames) * sizeof(float));
		if (audioBufferRight == NULL) {
			fprintf(stderr, "Could not allocate memory for audio file\n");
			sf_close(data.audioFile);
			return;
		}
	// put samples in buffers to make mono left and right channel buffers
		for (i = 0; i < data.audioFile.frames; i++) {
			sf_read_float(data.audioFile, &audioBufferLeft[i], 1);
			sf_read_float(data.audioFile, &audioBufferRight[i], 1);
		}
	// get lengths
		int audioBufferLeftLen = sizeof(audioBufferLeft);
		int audioBufferRightLen = sizeof(audioBufferRight);
		int IRLen = sizeof(data.impulseResponse);
	// init ptr to a ptr to output buffer
		float **outputBufferLeft;
		float **outputBufferRight;
	// convolve left, return size of left output
		int leftOutLen = convolve(audioBufferLeft, impulseResponse, audioBufferLeftLen, IRLen, *outputBufferLeft);
	// convolve right, return size of right output
		int rightOutLen = convolve(audioBufferRight, impulseResponse, audioBufferRightLen, IRLen, *outputBufferRight);
	// allocate memory for STEREO output		
		float *output = malloc((leftOutLen + rightOutLen) * sizeof(float));
		if (output == NULL) {
			fprintf(stderr, "Could not allocate memory for output audio file, something went terribly wrong!\n");
			free(output);
			return;
		}
	// put the samples into output to make STEREO array, count length
		int outLength = 1;
		for (i = 0; i < (leftOutLen + rightOutLen); i++) {
			sf_read_float(outputBufferLeft, &output[i], 1);
			outLength++;
			sf_read_float(outputBufferRight, &output[i], 1);
			outLength++;
		}
	// free allocated not in use memory	
		free(audioBufferLeft);
		free(audioBufferRight);
	}

// IF AUDIO IS MONO AND IR IS STEREO, average the two channels of the impulse response and then convolve it with the input audio.
	if (data.audioFileInfo.channels == MONO && data.impulseFileInfo.channels == STEREO) {
		printf("%s is in mono, and %s is in stereo.\n", data.audioFile, data.impulseResponse);
	// allocate memory for Impulse Response left and right buffers but not for already MONO audio file
		float *impulseBufferLeft= malloc((data.impulseFileInfo.frames) * sizeof(float));
		if (impulseBufferLeft == NULL) {
			fprintf(stderr, "Could not allocate memory for impulse response\n");
			sf_close(data.impulseResponse);
			return;
		}
		float *impulseBufferRight = malloc((data.impulseFileInfo.frames) * sizeof(float));
		if (impulseBufferRight == NULL) {
			fprintf(stderr, "Could not allocate memory for impulse response\n");
			sf_close(data.impulseResponse);
			return;
		}
	// put samples in buffers to make mono left and right channel buffers.  Divide all points by 2 to average the points
		for (i = 0; i < data.impulseFileInfo.frames; i++) {
			sf_read_float((data.impulseResponse)/2, &impulseBufferLeft[i], 1);
			sf_read_float((data.impulseResponse)/2, &impulseBufferRight[i], 1);
		}
	// allocate memory for Impulse Responsed MONO
		float *impulseBufferMONO = malloc((data.impulseFileInfo.frames) * sizeof(float));
		if (impulseBufferMONO == NULL) {
			fprintf(stderr, "Could not allocate memory for impulse response\n");
			sf_close(data.impulseResponse);
			return;
		}	
	// iterate through IRs putting samples into MONO buffer, by adding averaged points of the left and right channels.	
		int impulseBufferMONOLen = 1;
		for (i = 0; i < data.impulseFileInfo.frames; i++) {
			impulseBufferMONO[i] = (impulseBufferLeft[i] + impulseBufferRight[i]);			
			impulseBufferMONOLen++;
		}
	// convolve and get output length
		int outLength = convolve(data.audioFile, impulseBufferMONO, data.audioFileInfo.frames, impulseBufferMONOLen, *output);
	// free allocated not in use memory	
		free(impulseBufferLeft);
		free(impulseBufferRight);
		free(impulseBufferMONO);
	}
	
// IF BOTH ARE MONO, convolve normally.
	if (data.audioFile.channels == MONO && data.impulseFileInfo.channels == MONO) {
		printf("Both files are mono.\n");
		// init ptr to an output buffer
		
		// convolve and get output length
		int outLength = convolve(data.audioFile, data.impulseResponse, data.audioFileInfo.frames, data.impulseFileInfo.frames, float **output);
		// free allocated not in use memory	

		
	}
/**************************************************************************/
						/* MAKE FILE AND CLEAN UP! */
/**************************************************************************/
	
// Close audio files not in use
	sf_close(data.audioFile);
	sf_close(data.impulseResponse);
// write output array to playable audio file STEREO MONO?
	sf_write_float	(outfilename, output, outLength);
// free allocated not in use memory		
	free(output);
// end!
	return;
}

