/******************************************/
/*
   Final Project
   SLICE SAMPLER
   Lucas Hanson & Carrie Sutherland

*/
/******************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <portaudio.h>
#include <unistd.h>
#include <stdbool.h>
#include <SOIL/SOIL.h>
#include <sndfile.h>//libsndfile library
#include "fft.h"

// OpenGL
//#ifdef __MACOSX_CORE__
    #include <GLUT/glut.h>
/*  #else
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glut.h>
#endif
*/

// Platform-dependent sleep routines.
#if defined( __WINDOWS_ASIO__ ) || defined( __WINDOWS_DS__ )
#include <windows.h>
#define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds ) 
#else // Unix variants
#include <unistd.h>
#define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif


//-----------------------------------------------------------------------------
// global variables and #defines
//-----------------------------------------------------------------------------
#define FORMAT                  paFloat32
#define BUFFER_SIZE             2048
#define SAMPLE                  float
#define SAMPLING_RATE           44100
#define MONO                    1
#define STEREO                  2
#define cmp_abs(x)              ( sqrt( (x).re * (x).re + (x).im * (x).im ) )
#define INIT_WIDTH              800
#define INIT_HEIGHT             600
#define INIT_FREQUENCY          440
#define INC_FREQUENCY           5
#define INIT_START              0
#define INIT_VOLUME             0.8
#define INC_VAL_MOUSE           1.0f
#define INC_VAL_KB              .75f
#define DEFAULT_LOOP_LENGTH     100000
#define INC_LOOP_LENGTH         25000
#define START_NUDGE_AMOUNT      10000
#define FAST_NUDGE              100000
#define LOWPASS_INCR            5
#define HIGHPASS_INCR           2
#define WINDOW_SIZE             (BUFFER_SIZE/4)
#define HOP_SIZE                (WINDOW_SIZE/2)
#define VOLUME_INCR             0.1

typedef double  MY_TYPE;
typedef char BYTE;   // 8-bit unsigned entity.

typedef struct {
    int x;
    int y;
} Pos;

//individual slice data
typedef struct {
    SNDFILE *infile;
    SF_INFO sfinfoInput;
    bool playing;
    int start;
    int loopCounter;
    int loopLength;
    float volume;
    float muter;
    float lowpass;
    float highpass;
    float buffer[BUFFER_SIZE * STEREO];
    float filterLeft[BUFFER_SIZE];
    float filterRight[BUFFER_SIZE];
    float prev_left[WINDOW_SIZE];
    float prev_right[WINDOW_SIZE];

} slice;

typedef struct {
    float overlap; // Percentage of overlap
    int overlap_samples; // Overlap in samples
    int osamp; // Oversampling factor (WINDOW_SIZE / HOP_SIZE)
    bool change;
    slice sliceA;
    slice sliceB;
    slice sliceC;
    slice sliceD;
    int sliceSelector;
    //members for filtering
    float file_buff[STEREO * (BUFFER_SIZE + HOP_SIZE)];
    float window[WINDOW_SIZE];
    float curr_win[WINDOW_SIZE];
    int lowpass;
    int highpass;

} sndFile;

//Filter GLobals
float curr_magnitude[WINDOW_SIZE/2];
float curr_phase[WINDOW_SIZE/2];

float prev_magnitude[WINDOW_SIZE/2];
float prev_phase[WINDOW_SIZE/2];





//struct for GUI objects 
typedef struct {
    GLuint texture_id;
    Pos center;
    int width;
    int height;
} Texture;

// width and height of the window
GLsizei g_width = INIT_WIDTH;
GLsizei g_height = INIT_HEIGHT;
GLsizei g_last_width = INIT_WIDTH;
GLsizei g_last_height = INIT_HEIGHT;

// global audio vars
GLint g_buffer_size = BUFFER_SIZE;
unsigned int g_channels = STEREO;
SAMPLE g_buffer[BUFFER_SIZE];
SAMPLE g_window[BUFFER_SIZE];
float *songBuffer;


//Initialize sound file struct and slices
sndFile data;


// Threads Management
GLboolean g_ready = false;

// fill mode
GLenum g_fillmode = GL_FILL;

// light 0 position
GLfloat g_light0_pos[4] = { 1.0f, 1.2f, 1.0f, 5.0f };

// light 1 parameters
GLfloat g_light1_ambient[] = { .2f, .2f, .2f, 1.0f };
GLfloat g_light1_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat g_light1_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat g_light1_pos[4] = { -2.0f, 0.0f, -4.0f, 1.0f };

// fullscreen
GLboolean g_fullscreen = false;

// modelview stuff
GLfloat g_linewidth = 2.0f;
GLfloat g_angle_y = 0.0f;
GLfloat g_angle_x = 0.0f;
GLfloat g_inc = 0.0f;
GLfloat g_inc_y = 0.0;
GLfloat g_inc_x = 0.0;
bool g_key_rotate_y = false;
bool g_key_rotate_x =  false;

// rotation increments, set to defaults
GLfloat g_inc_val_mouse = INC_VAL_MOUSE;
GLfloat g_inc_val_kb = INC_VAL_KB;

// Texture
Texture g_texture;

// Translation
Pos g_tex_init_pos = {0,0};
Pos g_tex_incr = {0,0};

bool g_translate = false;
Pos g_init_pos = {0,0};
Pos g_incr = {0,0};

// Port Audio struct
PaStream *g_stream;


//-----------------------------------------------------------------------------
// function prototypes
//-----------------------------------------------------------------------------
void idleFunc( );
void displayFunc( );
float computeRMS(SAMPLE *buffer);
void drawFace();
void drawTimeDomain(SAMPLE *buffer, int side);
void drawCube(int x, int y, int side, SAMPLE *buffer);
void drawWindowedTimeDomain(SAMPLE * buffer);
void reshapeFunc( int width, int height );
void keyboardFunc( unsigned char, int, int );
void specialKey( int key, int x, int y );
void specialUpKey( int key, int x, int y);
void initialize_graphics( );
void initialize_glut(int argc, char *argv[]);
void initialize_gui();
void initialize_audio(char * audioFilename);
void stop_portAudio();
void startstop();
void setLocation(int location);
void increaseLoopLength();
void decreaseLoopLength();
void nudgeLocation(int nudgeAmount);
void incrementLoops();
void resetLoops();
void muteSlice();
void drawPad();
void filter(SAMPLE *buffer, SAMPLE *prev_win, int selector);
void volumeIncrease();
void volumeDecrease();
void decreaseLowpass();
void increaseLowpass();
void decreaseHighpass();
void increaseHighpass();

//Mouse callback functions
void mouseFunc(int button, int state, int x, int y);
void mouseMotionFunc(int x, int y);

// Function copied from the FFT library
/*void apply_window( float * data, float * window, unsigned long length )
{
   unsigned long i;
   for( i = 0; i < length; i++ )
       data[i] *= window[i];
}*///Already in fft code

//-----------------------------------------------------------------------------
// name: help()
// desc: ...
//-----------------------------------------------------------------------------
void help()
{
    printf( "----------------------------------------------------\n" );
    printf( "||SLICE SAMPLER||\n" );
    printf( "----------------------------------------------------\n" );
    printf( "'h' - print this help message\n" );
    printf( "'SPACEBAR' - Start and stop audio\n");
    printf( "----------------------------------------------------\n" );
    printf( "'a' - select slice A\n");
    printf( "'b' - select slice B\n");
    printf( "'c' - select slice C\n");
    printf( "'d' - select slice D\n");
    printf( "----------------------------------------------------\n" );
    printf( "'[' - decrease loop length\n" );
    printf( "']' - increase loop length\n" );
    printf( "'-' - nudge start location left\n" );
    printf( "'=' - nudge start location right\n" );
    printf( "'_' - quick nudge start location left\n" );
    printf( "'+' - quick nudge start location right\n" );
    printf( "[e/r] decreases/increases lowpass cutoff freq\n" \
            "[u/i] decreases/increases highpass cutoff freq \n");
    printf( "'t' increases volume of a slice\n" \
            "'y' decreases volume of a slice \n");
    printf( "'f' - toggle fullscreen\n" );
    printf( "'m' - mute on/off\n"); 
    printf( "'q' - quit\n" );
    printf( "----------------------------------------------------\n" );
    printf( "\n" );
}


//-----------------------------------------------------------------------------
// Name: paCallback( )
// Desc: callback from portAudio
//-----------------------------------------------------------------------------
static int paCallback(const void *inputBuffer,
        void *outputBuffer, unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData ) 
{
    //initialize output audio buffer
    SAMPLE * out = (SAMPLE *)outputBuffer;    

    int i, j; 
    int readcountA, readcountB, readcountC, readcountD;

    /* read data from file*/
    readcountA = sf_readf_float (data.sliceA.infile, data.sliceA.buffer, framesPerBuffer); 
    readcountB = sf_readf_float (data.sliceB.infile, data.sliceB.buffer, framesPerBuffer + data.overlap_samples);
    readcountC = sf_readf_float (data.sliceC.infile, data.sliceC.buffer, framesPerBuffer); 
    readcountD = sf_readf_float (data.sliceD.infile, data.sliceD.buffer, framesPerBuffer);  
    
    //Slice A, B, C, D, Off on
    if (!data.sliceA.playing) {
        sf_seek(data.sliceA.infile, data.sliceA.start, SEEK_SET);

        readcountA = sf_readf_float (data.sliceA.infile, data.sliceA.buffer+(readcountA*STEREO), framesPerBuffer-readcountA);

        data.sliceA.muter = 0.0;  
    }
    //output audio
    else {
        data.sliceA.muter = 1.0;
    }
    if (!data.sliceB.playing) {
        sf_seek(data.sliceB.infile, data.sliceB.start, SEEK_SET);

        readcountB = sf_readf_float (data.sliceB.infile, data.sliceB.buffer+(readcountB*STEREO), framesPerBuffer-readcountB + data.overlap_samples);

        data.sliceB.muter = 0.0;  
    }

    //output audio
    else {
        data.sliceB.muter = 1.0;
    }
    if (!data.sliceC.playing) {
        sf_seek(data.sliceC.infile, data.sliceC.start, SEEK_SET);

        readcountC = sf_readf_float (data.sliceC.infile, data.sliceC.buffer+(readcountC*STEREO), framesPerBuffer-readcountC);

        data.sliceC.muter = 0.0;  
    }

    //output audio
    else {
        data.sliceC.muter = 1.0;
    }
    if (!data.sliceD.playing) {
        sf_seek(data.sliceD.infile, data.sliceD.start, SEEK_SET);

        readcountD = sf_readf_float (data.sliceD.infile, data.sliceD.buffer+(readcountD*STEREO), framesPerBuffer-readcountD);

        data.sliceD.muter = 0.0;  
    }

    //output audio
    else {
        data.sliceD.muter = 1.0;
    }

    //Rewind overlap
    sf_seek( data.sliceB.infile, -data.overlap_samples, SEEK_CUR );
    
    resetLoops();

    //de-interleave

    for (i = 0; i < framesPerBuffer+data.overlap_samples; i++){
        data.sliceB.filterLeft[i] = data.sliceB.buffer[2 * i];
        data.sliceB.filterRight[i] = data.sliceB.buffer[2 * i +1];
    }    
    //filter left
    filter(data.sliceB.filterLeft, data.sliceB.prev_left, 1);
    //filter right
    filter(data.sliceB.filterRight,data.sliceB.prev_right, 1);

    //interleave
    for (i = 0; i < framesPerBuffer; i++){
        data.sliceB.buffer[2 * i] = data.sliceB.filterLeft[i]; 
        data.sliceB.buffer[2 * i + 1] = data.sliceB.filterRight[i];
    }

    /* combine samples adjusted for volume from each file for each channel */
    for (i = 0; i < framesPerBuffer * STEREO; i+=2) {
        out[i] = ((data.sliceA.volume * data.sliceA.buffer[i] * data.sliceA.muter) + (data.sliceB.volume * data.sliceB.buffer[i] * data.sliceB.muter) + (data.sliceC.volume * data.sliceC.buffer[i] * data.sliceC.muter) + (data.sliceD.volume * data.sliceD.buffer[i] * data.sliceD.muter));
        out[i+1] = ((data.sliceA.volume * data.sliceA.buffer[i+1] * data.sliceA.muter) + (data.sliceB.volume * data.sliceB.buffer[i+1] * data.sliceB.muter) + (data.sliceC.volume * data.sliceC.buffer[i+1] * data.sliceC.muter) + (data.sliceD.volume * data.sliceD.buffer[i+1] * data.sliceD.muter));
        incrementLoops();
    }


    //set flag
    g_ready = true;

    return paContinue;
    return 0;
}
 //-----------------------------------------------------------------------------
// Name: filter
// Desc: Applies low pass and high pass filters 
//-----------------------------------------------------------------------------
 void filter(SAMPLE *buffer, SAMPLE *prev_win, int selector) {
 /* FILTERSSS */
    /* STFT */
    int i, j;
    float hipass, lowpass;
    for (i = 0; i < BUFFER_SIZE; i+=HOP_SIZE)
    {
        /* Apply window to current frame */
        for (j = 0; j < WINDOW_SIZE; j++) {
            data.curr_win[j] = buffer[i+j];
        }
        apply_window(data.curr_win, data.window, WINDOW_SIZE);

        /* FFT */
        rfft(data.curr_win, WINDOW_SIZE/2, FFT_FORWARD );
        complex * curr_cbuf = (complex *)data.curr_win;
        rfft( prev_win, WINDOW_SIZE/2, FFT_FORWARD );
        complex * prev_cbuf = (complex *)prev_win;

        /* Get Magnitude and Phase (polar coordinates) */
        for (j = 0; j < WINDOW_SIZE/2; ++j)
        {
            curr_magnitude[j] = cmp_abs(curr_cbuf[j]);
            curr_phase[j] = atan2f(curr_cbuf[j].im, curr_cbuf[j].re);

            prev_magnitude[j] = cmp_abs(prev_cbuf[j]);
            prev_phase[j] = atan2f(prev_cbuf[j].im, prev_cbuf[j].re);
        }
         switch (selector){
            case 0:
                lowpass = data.sliceA.lowpass;
                hipass = WINDOW_SIZE/4 - data.sliceA.highpass;
                
                break;
    
            case 1:
                lowpass = data.sliceB.lowpass;
                hipass = WINDOW_SIZE/4 - data.sliceB.highpass;
                break;

            case 2:
                lowpass = data.sliceC.lowpass;
                hipass = WINDOW_SIZE/4 - data.sliceC.highpass;
                break;
        
            case 3:
                lowpass = data.sliceD.lowpass;
                hipass = WINDOW_SIZE/4 - data.sliceD.highpass;
                break;   
        }
         /* Filter windows */
          for (j = 0; j < WINDOW_SIZE/2; ++j) {

              if ((j > WINDOW_SIZE/4 - lowpass && j < WINDOW_SIZE/4 + lowpass) || 
                  j < WINDOW_SIZE/4 - hipass || j > WINDOW_SIZE/4 + hipass)
              {
                  curr_magnitude[j] = 0;
                  prev_magnitude[j] = 0;
              }
          }

        /* Back to Cartesian coordinates */
        for (j = 0; j < WINDOW_SIZE/2; j++) {
            curr_cbuf[j].re = curr_magnitude[j] * cosf(curr_phase[j]);
            curr_cbuf[j].im = curr_magnitude[j] * sinf(curr_phase[j]);
            prev_cbuf[j].re = prev_magnitude[j] * cosf(prev_phase[j]);
            prev_cbuf[j].im = prev_magnitude[j] * sinf(prev_phase[j]);
        }

        // /* Back to Time Domain */
        rfft( (float*)curr_cbuf, WINDOW_SIZE/2, FFT_INVERSE );
        rfft( (float*)prev_cbuf, WINDOW_SIZE/2, FFT_INVERSE );

        /* Assign to the output */
        for (j = 0; j < HOP_SIZE; j++) {
            buffer[i+j] = prev_win[j+HOP_SIZE] + data.curr_win[j];
        }

        /* Update previous window */
        for (j = 0; j < WINDOW_SIZE; j++) {
            prev_win[j] = data.curr_win[j];
        }
    }
}
//-----------------------------------------------------------------------------
// Name: initialize_gui( )
// Desc: Initializes Gui object
//-----------------------------------------------------------------------------
void initialize_gui()
{
    // Zero out global texture
    memset(&g_texture, 0, sizeof(Texture));
}
//-----------------------------------------------------------------------------
// Name: initialize_glut( )
// Desc: Initializes Glut with the global vars
//-----------------------------------------------------------------------------
void initialize_glut(int argc, char *argv[]) {
    // initialize GLUT
    glutInit( &argc, argv );
    // double buffer, use rgb color, enable depth buffer
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
    // initialize the window size
    glutInitWindowSize( g_width, g_height );
    // set the window postion
    glutInitWindowPosition( 400, 100 );
    // create the window
    glutCreateWindow( "||SLICE SAMPLER||");
    // full screen
    if( g_fullscreen )
        glutFullScreen();

    // set the idle function - called when idle
    glutIdleFunc( idleFunc );
    // set the display function - called when redrawing
    glutDisplayFunc( displayFunc );
    // set the reshape function - called when client area changes
    glutReshapeFunc( reshapeFunc );
    // set the keyboard function - called on keyboard events
    glutKeyboardFunc( keyboardFunc );
    // set window's to specialKey callback
    glutSpecialFunc( specialKey );
    // set window's to specialUpKey callback (when the key is up is called)
    glutSpecialUpFunc( specialUpKey );
    // set the mouse function for new clicks
    glutMouseFunc( mouseFunc );
    // set the mouse function for motion when a button is pressed
    glutMotionFunc( mouseMotionFunc ); 
    
     // Load Texture
    /*g_texture.texture_id = SOIL_load_OGL_texture
        (
         "Speaker.jpg",
         SOIL_LOAD_AUTO,
         SOIL_CREATE_NEW_ID,
         SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y
        );
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glEnable( GL_TEXTURE_2D );

    // Get Width and Height
    unsigned char *image = SOIL_load_image("Speaker.jpg", &g_texture.width, &g_texture.height, NULL, 0);
    SOIL_free_image_data(image);*/

    // do our own initialization
    initialize_graphics( );  
}

/*void hanning( float * window, unsigned long length )
{
   unsigned long i;
   double pi, phase = 0, delta;

   pi = 4.*atan(1.0);
   delta = 2 * pi / (double) length;

   for( i = 0; i < length; i++ )
   {
       window[i] = (float)(0.5 * (1.0 - cos(phase)));
       phase += delta;
   }
}*/ //Already in fft code


//-----------------------------------------------------------------------------
// Name: initialize_audio( RtAudio *dac )
// Desc: Initializes PortAudio with the global vars and the stream
//-----------------------------------------------------------------------------
void initialize_audio(char * audioFilename) {

    //Clear structs
    memset(&data.sliceA.sfinfoInput, 0, sizeof(data.sliceA.sfinfoInput));
    memset(&data.sliceB.sfinfoInput, 0, sizeof(data.sliceB.sfinfoInput));
    memset(&data.sliceC.sfinfoInput, 0, sizeof(data.sliceC.sfinfoInput));
    memset(&data.sliceD.sfinfoInput, 0, sizeof(data.sliceD.sfinfoInput));

    //Open Input Audio File
    data.sliceA.infile = sf_open(audioFilename, SFM_READ, &data.sliceA.sfinfoInput);
    if (data.sliceA.infile == NULL) {
        printf ("Error: could not open file: %s\n", audioFilename) ;
        puts(sf_strerror (NULL)) ;
        exit(1);
    }

    printf("Audio file: Frames: %d Channels: %d Samplerate: %d\n\n", 
            (int)data.sliceA.sfinfoInput.frames, data.sliceA.sfinfoInput.channels, data.sliceA.sfinfoInput.samplerate);

    //check if audio file is between 10 seconds and 5 minutes
    if (data.sliceA.sfinfoInput.frames / SAMPLING_RATE >= 300 || data.sliceA.sfinfoInput.frames / SAMPLING_RATE < 10){
        printf("Error: Audio file must be between 10 seconds and 5 minutes in length.\n");
        exit(1);
    }

    //Open Input Audio File
    data.sliceB.infile = sf_open(audioFilename, SFM_READ, &data.sliceB.sfinfoInput);
    if (data.sliceB.infile == NULL) {
        printf ("Error: could not open file: %s\n", audioFilename) ;
        puts(sf_strerror (NULL)) ;
        exit(1);
    }
    //Open Input Audio File
    data.sliceC.infile = sf_open(audioFilename, SFM_READ, &data.sliceC.sfinfoInput);
    if (data.sliceC.infile == NULL) {
        printf ("Error: could not open file: %s\n", audioFilename) ;
        puts(sf_strerror (NULL)) ;
        exit(1);
    }
    //Open Input Audio File
    data.sliceD.infile = sf_open(audioFilename, SFM_READ, &data.sliceD.sfinfoInput);
    if (data.sliceD.infile == NULL) {
        printf ("Error: could not open file: %s\n", audioFilename) ;
        puts(sf_strerror (NULL)) ;
        exit(1);
    }
 
    hanning(data.window, WINDOW_SIZE);
    memset(&data.sliceA.prev_left, 0, WINDOW_SIZE*sizeof(float));
    memset(&data.sliceA.prev_right, 0, WINDOW_SIZE*sizeof(float));
    memset(&data.sliceB.prev_left, 0, WINDOW_SIZE*sizeof(float));
    memset(&data.sliceB.prev_right, 0, WINDOW_SIZE*sizeof(float));
    memset(&data.sliceC.prev_left, 0, WINDOW_SIZE*sizeof(float));
    memset(&data.sliceC.prev_right, 0, WINDOW_SIZE*sizeof(float));
    memset(&data.sliceD.prev_left, 0, WINDOW_SIZE*sizeof(float));
    memset(&data.sliceD.prev_right, 0, WINDOW_SIZE*sizeof(float));

    PaStreamParameters outputParameters;
    PaStreamParameters inputParameters;
    PaError err;

    //Initilialize struct data
    data.sliceSelector = 0;
    // Set overlap factor
    data.overlap = (WINDOW_SIZE - HOP_SIZE) / (float)WINDOW_SIZE;
    data.overlap_samples = data.overlap * WINDOW_SIZE;
    data.osamp = WINDOW_SIZE / HOP_SIZE;
    //Slice initialization
    data.sliceA.playing = false;
    data.sliceA.start = INIT_START;
    data.sliceA.loopCounter = 0;
    data.sliceA.loopLength = DEFAULT_LOOP_LENGTH;
    data.sliceA.volume = INIT_VOLUME;
    data.sliceA.muter = 1.0;
    data.sliceA.highpass = 0;
    data.sliceA.lowpass = 0;

    
    data.sliceB.playing = false;
    data.sliceB.start = data.sliceB.sfinfoInput.frames/4;//start at 1/4
    data.sliceB.loopCounter = 0;
    data.sliceB.loopLength = DEFAULT_LOOP_LENGTH;
    data.sliceB.volume = INIT_VOLUME;
    data.sliceB.muter = 1.0;
    data.sliceB.highpass = 0;
    data.sliceB.lowpass = 0;

    data.sliceC.playing = false;
    data.sliceC.start = data.sliceC.sfinfoInput.frames/2;//start 1/2
    data.sliceC.loopCounter = 0;
    data.sliceC.loopLength = DEFAULT_LOOP_LENGTH;
    data.sliceC.volume = INIT_VOLUME;
    data.sliceC.muter = 1.0;
    data.sliceC.highpass = 0;
    data.sliceC.lowpass = 0;
    
    data.sliceD.playing = false;
    data.sliceD.start = 3 * (data.sliceD.sfinfoInput.frames/4);//start at 3/4 through file
    data.sliceD.loopCounter = 0;
    data.sliceD.loopLength = DEFAULT_LOOP_LENGTH;
    data.sliceD.volume = INIT_VOLUME;
    data.sliceD.muter = 1.0;
    data.sliceD.highpass = 0;
    data.sliceD.lowpass = 0;



    /* Initialize PortAudio */
    Pa_Initialize();

    /* Set output stream parameters */
    outputParameters.device = Pa_GetDefaultOutputDevice();
    outputParameters.channelCount = g_channels;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = 
        Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    /* Open audio stream */
    err = Pa_OpenStream( &g_stream,
            NULL,
            &outputParameters,
            SAMPLING_RATE, BUFFER_SIZE, paNoFlag, 
            paCallback, &data);


    if (err != paNoError) {
        printf("PortAudio error: open stream: %s\n", Pa_GetErrorText(err));
    }

    /* Start audio stream */
    err = Pa_StartStream( g_stream );
    if (err != paNoError) {
        printf(  "PortAudio error: start stream: %s\n", Pa_GetErrorText(err));
    }
}

void stop_portAudio(PaStream **stream) {
    PaError err;

    /* Stop audio stream */
    err = Pa_StopStream( *stream );
    if (err != paNoError) {
        printf(  "PortAudio error: stop stream: %s\n", Pa_GetErrorText(err));
    }
    /* Close audio stream */
    err = Pa_CloseStream(*stream);
    if (err != paNoError) {
        printf("PortAudio error: close stream: %s\n", Pa_GetErrorText(err));
    }
    /* Terminate audio stream */
    err = Pa_Terminate();
    if (err != paNoError) {
        printf("PortAudio error: terminate: %s\n", Pa_GetErrorText(err));
    }
}


//-----------------------------------------------------------------------------
// Name: main
// Desc: ...
//-----------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
    if (argc != 2) {
        printf ("\nAn input file is required: \n");
        printf ("    Usage : slicesampler <audio input filename> \n");
        exit (1);
    }
    // Print help
    help();
    
    //Initialize Gui objects
    initialize_gui();

    // Initialize Glut
    initialize_glut(argc, argv);

    // Initialize PortAudio
    initialize_audio(argv[1]);

    // Wait until 'q' is pressed to stop the process
    glutMainLoop();

    // This will never get executed

    return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
// Name: idleFunc( )
// Desc: callback from GLUT
//-----------------------------------------------------------------------------
void idleFunc( )
{
    // render the scene
    glutPostRedisplay( );
}

//-----------------------------------------------------------------------------
// Name: keyboardFunc( )
// Desc: key event
//-----------------------------------------------------------------------------
void keyboardFunc( unsigned char key, int x, int y )
{
    //printf("key: %c\n", key);
    switch( key )
    {
        // Print Help
        case 'h':
            help();
            break;

        //Start Stop
        case 32:
            startstop();
            break;

        //Mute on/off
        case 'm': 
            muteSlice();
            break;
        //Select Slice A
        case 'a':
            data.sliceSelector = 0;
            break;
        //Select Slice B
        case 'b':
            data.sliceSelector = 1;
            break;
            
        //Select Slice C
        case 'c':
            data.sliceSelector = 2;
            break;
        //Select Slice D
        case 'd':
            data.sliceSelector = 3;
            break;
        //Increase Loop Length
        case ']':
            increaseLoopLength();
            break;
        //Decrease Loop Length
        case '[':
            decreaseLoopLength();

            break;
        //Nudge right start location
        case '-':
            nudgeLocation(-START_NUDGE_AMOUNT);
            break;
        //Nudge left start location
        case '=':
            nudgeLocation(START_NUDGE_AMOUNT);
            break;

        //Fast Nudge right start location
        case '_':
            nudgeLocation(-FAST_NUDGE);
            break;
        //Fast Nudge left start location
        case '+':
            nudgeLocation(FAST_NUDGE);
            break;

        case 'e':
            decreaseLowpass();
            break;
        case 'r':
            increaseLowpass();
            break;

        case 'u':
            decreaseHighpass();
            break;
        case 'i':
            increaseHighpass(); 
            break;
        
        //Volume Increase
        case 't':
            volumeIncrease();
            break;
        //Volume Decrease
        case 'y':
            volumeDecrease();
            break;

        // Fullscreen
        case 'f':
            if( !g_fullscreen )
            {
                g_last_width = g_width;
                g_last_height = g_height;
                glutFullScreen();
            }
            else
                glutReshapeWindow( g_last_width, g_last_height );

            g_fullscreen = !g_fullscreen;
            printf("[SLICESAMPLER]: fullscreen: %s\n", g_fullscreen ? "ON" : "OFF" );
            break;

        case 'q':
            // Close Stream before exiting
            stop_portAudio(&g_stream);
            free(songBuffer);
            sf_close(data.sliceA.infile);
            sf_close(data.sliceB.infile);
            sf_close(data.sliceC.infile);
            sf_close(data.sliceD.infile);
            printf("-------------------------------");
            printf("\nGOODBYE :)\n");
            exit( 0 );
            break;
    }
}

//-----------------------------------------------------------------------------
// Name: specialUpKey( int key, int x, int y)
// Desc: Callback to know when a special key is pressed
//-----------------------------------------------------------------------------
void specialKey(int key, int x, int y) { 
  // Check which (arrow) key is pressed
  // Rotations are called when keys are pressed
  switch(key) {
    case GLUT_KEY_LEFT : // Arrow key left is pressed
      g_key_rotate_y = true;
      g_inc_y = -g_inc_val_kb;
      break;
    case GLUT_KEY_RIGHT :    // Arrow key right is pressed
      g_key_rotate_y = true;
      g_inc_y = g_inc_val_kb;
      break;
    case GLUT_KEY_UP :        // Arrow key up is pressed
      g_key_rotate_x = true;
      g_inc_x = g_inc_val_kb;
      break;
    case GLUT_KEY_DOWN :    // Arrow key down is pressed
      g_key_rotate_x = true;
      g_inc_x = -g_inc_val_kb;
      break;   
  }
}  

//-----------------------------------------------------------------------------
// Name: specialUpKey( int key, int x, int y)
// Desc: Callback to know when a special key is up
//-----------------------------------------------------------------------------
void specialUpKey( int key, int x, int y) {
  // Check which (arrow) key is unpressed
  // Rotations halted when keys become unpressed
  switch(key) {
    case GLUT_KEY_LEFT : // Arrow key left is unpressed
      printf("[SLICESAMPLER]: LEFT\n");
      g_key_rotate_y = false;
      break;
    case GLUT_KEY_RIGHT :    // Arrow key right is unpressed
      printf("[SLICESAMPLER]: RIGHT\n");
      g_key_rotate_y = false;
      break;
    case GLUT_KEY_UP :        // Arrow key up is unpressed
      printf("[SLICESAMPLER]: UP\n");
      g_key_rotate_x = false;
      break;
    case GLUT_KEY_DOWN :    // Arrow key down is unpressed
      printf("[SLICESAMPLER]: DOWN\n");
      g_key_rotate_x = false;
      break;   
  }
}

//-----------------------------------------------------------------------------
// Name: mouseFunc(int button, int state, int x, int y)
// Desc: Callback to manage the mouse input when click new button
//-----------------------------------------------------------------------------
void mouseFunc(int button, int state, int x, int y) {
    printf(": %d, %d, x:%d, y:%d\n", button, state, x, y);
    if (state == 0) {
        // start Translation
        g_translate = true;
    }
    else {
        // Stop Translation
        g_translate = false;
        g_incr.x = 0;
        g_incr.y = 0;
       
    }
}

//-----------------------------------------------------------------------------
// Name: mouseFunc(int button, int state, int x, int y)
// Desc: Callback to manage the mouse motion
//-----------------------------------------------------------------------------
void mouseMotionFunc(int x, int y) {
    printf("Mouse Moving: %d, %d\n", x, y);
    if (g_translate) { //if mouse is being clicked move scene
        g_incr.x = (x - (g_width / 2));
        g_incr.y = (y - (g_height / 2));
        g_init_pos.x += g_incr.x / 100;
        g_init_pos.y += g_incr.y / 100;
    }
}

//-----------------------------------------------------------------------------
// Name: reshapeFunc( )
// Desc: called when window size changes
//-----------------------------------------------------------------------------
void reshapeFunc( int w, int h )
{
    // save the new window size
    g_width = w; g_height = h;
    // map the view port to the client area
    glViewport( 0, 0, w, h );
    // set the matrix mode to project
    glMatrixMode( GL_PROJECTION );
    // load the identity matrix
    glLoadIdentity( );
    // create the viewing frustum
    //gluPerspective( 45.0, (GLfloat) w / (GLfloat) h, .05, 50.0 );
    gluPerspective( 45.0, (GLfloat) w / (GLfloat) h, 1.0, 1000.0 );
    // set the matrix mode to modelview
    glMatrixMode( GL_MODELVIEW );
    // load the identity matrix
    glLoadIdentity( );
    // position the view point
    gluLookAt( 0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f );
}


//-----------------------------------------------------------------------------
// Name: initialize_graphics( )
// Desc: sets initial OpenGL states and initializes any application data
//-----------------------------------------------------------------------------
void initialize_graphics()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);                 // Black Background
    // set the shading model to 'smooth'
    glShadeModel( GL_SMOOTH );
    // enable depth
    glEnable( GL_DEPTH_TEST );
    // set the front faces of polygons
    glFrontFace( GL_CCW );
    // set fill mode
    glPolygonMode( GL_FRONT_AND_BACK, g_fillmode );
    // enable lighting
    glEnable( GL_LIGHTING );
    // enable lighting for front
    glLightModeli( GL_FRONT_AND_BACK, GL_TRUE );
    // material have diffuse and ambient lighting 
    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
    // enable color
    glEnable( GL_COLOR_MATERIAL );
    // normalize (for scaling)
    glEnable( GL_NORMALIZE );
    // line width
    glLineWidth( g_linewidth );

    // enable light 0
    glEnable( GL_LIGHT0 );

    // setup and enable light 1
    glLightfv( GL_LIGHT1, GL_AMBIENT, g_light1_ambient );
    glLightfv( GL_LIGHT1, GL_DIFFUSE, g_light1_diffuse );
    glLightfv( GL_LIGHT1, GL_SPECULAR, g_light1_specular );
    glEnable( GL_LIGHT1 );
}

//-----------------------------------------------------------------------------
// Name: void rotateView ()
// Desc: Rotates the current view by the angle specified in the globals
//-----------------------------------------------------------------------------
void rotateView () {
  if (g_key_rotate_y) {
    glRotatef ( g_angle_y += g_inc_y, 0.0f, 1.0f, 0.0f );
  }
  else {
    glRotatef (g_angle_y, 0.0f, 1.0f, 0.0f );
  }
  
  if (g_key_rotate_x) {
    glRotatef ( g_angle_x += g_inc_x, 1.0f, 0.0f, 0.0f );
  }
  else {
    glRotatef (g_angle_x, 1.0f, 0.0f, 0.0f );
  }
}

//-----------------------------------------------------------------------------
// Name: void drawWaveform ()
// Desc: draws each waveform from their respective buffers
//-----------------------------------------------------------------------------
void drawWaveform() {

    glPushMatrix();
    {
        //apply any rotations
        rotateView();

        //Draw 1st slice waveform
        glPushMatrix();
        glTranslatef(0.0f, 3.0f,  0.0f);
        if (data.sliceSelector == 0){
            glColor3f(0.0f, 0.0f, 1.0f);
        }
        else {
        glColor3f(1.0f, 0.0f, 0.0f);//Red
        }
        drawWindowedTimeDomain(data.sliceA.buffer);
        glPopMatrix();

        //Draw 2nd slice waveform
        glPushMatrix();
        glTranslatef(0.0f, 1.1f, 0.0f);
        if (data.sliceSelector == 1){
            glColor3f(0.0f, 0.0f, 1.0f);
        }
        else {
            glColor3f(1.0f, 0.20f, 0.0f);//Orange
        }
        drawWindowedTimeDomain(data.sliceB.buffer);
        glPopMatrix();
        
        //Draw 3rd slice waveform
        glPushMatrix();
        glTranslatef(0.0f, -1.1f, 0.0f);
        if (data.sliceSelector == 2){
            glColor3f(0.0f, 0.0f, 1.0f);
        }
        else {
            glColor3f(1.0f, 1.0f, 0.0f);//Yellow
        }
        drawWindowedTimeDomain(data.sliceC.buffer);
        glPopMatrix();

        //Draw 4th slice waveform
        glPushMatrix();
        glTranslatef(0.0f, -3.0f, 0.0f);
        if (data.sliceSelector == 3){
            glColor3f(0.0f, 0.0f, 1.0f);
        }
        else {
            glColor3f(0.0f, 1.0f, 0.0f);//Green
        }
        drawWindowedTimeDomain(data.sliceD.buffer);
        glPopMatrix();


       
    }
    glPopMatrix();
}
//-----------------------------------------------------------------------------
// Name: void drawWindowedTimeDomain(SAMPLE *buffer)
// Desc: Draws the Windowed Time Domain signal in the top of the screen
//-----------------------------------------------------------------------------
void drawWindowedTimeDomain(SAMPLE * buffer) {
    // Initialize initial x
    GLfloat x = -5;

    // Calculate increment x
    GLfloat xinc = fabs((2*x)/BUFFER_SIZE);

    glPushMatrix();
    {
        glBegin(GL_LINE_STRIP);

        //Draw Windowed Time Domain
        int i;
        for (i = 0; i<BUFFER_SIZE; i++)
        {
            glVertex3f(x, buffer[i], 0.0f);
            x += xinc;
        }

        glEnd();

    }
    glPopMatrix();
}
/*//-----------------------------------------------------------------------------
// Name: drawPad
// Desc: Draws GUI pads
//-----------------------------------------------------------------------------
void drawFace(int side) {
    GLfloat half = side / 2.0f;

    // Use the texture
    glBindTexture(GL_TEXTURE_2D, g_texture.texture_id);

    glBegin( GL_QUADS );
    glTexCoord2d(0.0,0.0); 
    glVertex3d(-half, -half, 0);

    glTexCoord2d(1.0,0.0); 
    glVertex3d(half, -half, 0);
    glTexCoord2d(1.0,1.0); 
    glVertex3d(half, half, 0);
    glTexCoord2d(0.0,1.0); 
    glVertex3d(-half, half, 0);
    glEnd();
}
void drawPad() {
    int side = 250;
    GLfloat half = side / 2.0f;
 
    glPushMatrix(); {
        // Translate
        glTranslatef(g_texture.center.x + g_tex_incr.x, g_texture.center.y + g_tex_incr.y, 0.0f);

        // Rotate
        rotateView();

        // DRAW
        // Front Face
        glPushMatrix();
        glTranslatef(0, 0, half);
        //drawTimeDomain(buffer, side);
        drawFace(side);
        glPopMatrix();


    }
    //drawWaveform();
    glPopMatrix();
}*/

//-----------------------------------------------------------------------------
// Name: displayFunc( )
// Desc: callback function invoked to draw the client area
//-----------------------------------------------------------------------------
void displayFunc( )
{ 
    SAMPLE buffer[g_buffer_size];

    // wait for data
    while( !g_ready ) usleep( 1000 );

    // copy currently playing audio into buffer
    memcpy( buffer, data.sliceA.buffer, g_buffer_size * sizeof(SAMPLE) );

    // Hand off to audio callback thread
    g_ready = false;

    // clear the color and depth buffers
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    //drawPad();
    drawWaveform();

    // flush gl commands
    glFlush( );

    // swap the buffers
    glutSwapBuffers( );
}

//-----------------------------------------------------------------------------
// Name: startstop
// Desc: sets sf_seek to new location in audio file
//-----------------------------------------------------------------------------
void startstop(){
    switch (data.sliceSelector){
        case 0:
            if (data.sliceA.playing){
                data.sliceA.playing = false;
            }
            else {
                data.sliceA.playing = true;
            }
            break;
        
        case 1:
            if (data.sliceB.playing){
                data.sliceB.playing = false;
            }
            else {
                data.sliceB.playing = true;
            }
            break;

        case 2:
            if (data.sliceC.playing){
                data.sliceC.playing = false;
            }
            else {
                data.sliceC.playing = true;
            }
            break;
        
        case 3:
            if (data.sliceD.playing){
                data.sliceD.playing = false;
            }
            else {
                data.sliceD.playing = true;
            }
            break;   
    }  
}
//-----------------------------------------------------------------------------
// Name: increaseLoopLength
// Desc: increases slice loop length
//-----------------------------------------------------------------------------
void increaseLoopLength(int length)
{
    switch (data.sliceSelector){
        case 0:
            if (data.sliceA.loopLength < INC_LOOP_LENGTH){
                data.sliceA.loopLength = data.sliceA.loopLength * 2;
            }
            else if (data.sliceA.loopLength < data.sliceA.sfinfoInput.frames - INC_LOOP_LENGTH){//Only increase if loop length will be smaller than total audio
                data.sliceA.loopLength += INC_LOOP_LENGTH;
            }
            break;
        
        case 1:
            if (data.sliceB.loopLength < INC_LOOP_LENGTH){
                data.sliceB.loopLength = data.sliceB.loopLength * 2;
            }
            else if (data.sliceB.loopLength < data.sliceB.sfinfoInput.frames - INC_LOOP_LENGTH){//Only increase if loop length will be smaller than total audio
                data.sliceB.loopLength += INC_LOOP_LENGTH;
            }
            break;

        case 2:
            if (data.sliceC.loopLength < INC_LOOP_LENGTH){
                data.sliceC.loopLength = data.sliceC.loopLength * 2;
            }
            else if (data.sliceC.loopLength < data.sliceC.sfinfoInput.frames - INC_LOOP_LENGTH){//Only increase if loop length will be smaller than total audio
                data.sliceC.loopLength += INC_LOOP_LENGTH;
            }
            break;
        
        case 3:
            if (data.sliceD.loopLength < INC_LOOP_LENGTH){
                data.sliceD.loopLength = data.sliceD.loopLength * 2;
            }
            else if (data.sliceD.loopLength < data.sliceD.sfinfoInput.frames - INC_LOOP_LENGTH){//Only increase if loop length will be smaller than total audio
                data.sliceD.loopLength += INC_LOOP_LENGTH;
            }
            break;   
    }  
    
}//-----------------------------------------------------------------------------
// Name: decreaseLoopLength
// Desc: decreases slice loop length
//-----------------------------------------------------------------------------
void decreaseLoopLength()
{
    switch (data.sliceSelector){
        case 0:
            if (data.sliceA.loopLength - INC_LOOP_LENGTH > 0){//Only increase if loop length will be smaller than total audio
                data.sliceA.loopLength -= INC_LOOP_LENGTH;
            }
            else {
                if (data.sliceA.loopLength/2 > 0){
                    data.sliceA.loopLength/=2;
                }
            }
            break;
        
        case 1:
            if (data.sliceB.loopLength - INC_LOOP_LENGTH > 0){
                data.sliceB.loopLength -= INC_LOOP_LENGTH;
            }
            else {
                if (data.sliceB.loopLength/2 > 0){
                    data.sliceB.loopLength /= 2;
                }
            }
            break;

        case 2:
            if (data.sliceC.loopLength - INC_LOOP_LENGTH > 0){
                data.sliceC.loopLength -= INC_LOOP_LENGTH;
            }
            else {
                if (data.sliceC.loopLength/2 > 0){
                    data.sliceC.loopLength/=2;
                }
            }
            break;
        
        case 3:
            if (data.sliceD.loopLength - INC_LOOP_LENGTH > 0){
                data.sliceD.loopLength -= INC_LOOP_LENGTH;
            }
            else {
                if (data.sliceD.loopLength/2 > 0){
                    data.sliceD.loopLength/=2;
                }
            }
            break;   
    }  
    
}
//-----------------------------------------------------------------------------
// Name: nudgeLocation
// Desc: sets sf_seek to new location in audio file
//-----------------------------------------------------------------------------
void nudgeLocation(int nudgeAmount)
{
    switch (data.sliceSelector){
        case 0:
            if (data.sliceA.start + nudgeAmount > 0 && data.sliceA.start + nudgeAmount < data.sliceA.sfinfoInput.frames){
                data.sliceA.start += nudgeAmount;
            }
            break;
        
        case 1:
            if (data.sliceB.start + nudgeAmount > 0 && data.sliceB.start + nudgeAmount < data.sliceB.sfinfoInput.frames){
                data.sliceB.start += nudgeAmount;
            }
            break;

        case 2:
            if (data.sliceC.start + nudgeAmount > 0 && data.sliceC.start + nudgeAmount < data.sliceC.sfinfoInput.frames){
                data.sliceC.start += nudgeAmount;
            }
            break;
        
        case 3:
            if (data.sliceD.start + nudgeAmount > 0 && data.sliceD.start + nudgeAmount < data.sliceD.sfinfoInput.frames){
                data.sliceD.start += nudgeAmount;
            }
            break;    
    }
   
}
//-----------------------------------------------------------------------------
// Name: incrementLoops
// Desc: increments loopcounters for all slices
//-----------------------------------------------------------------------------
void incrementLoops()
{
    data.sliceA.loopCounter++;
    data.sliceB.loopCounter++;
    data.sliceC.loopCounter++;
    data.sliceD.loopCounter++;
   
}
//-----------------------------------------------------------------------------
// Name: resetLoops
// Desc: resetLoops if they have reached their endpoint
//-----------------------------------------------------------------------------
void resetLoops()
{
    //reset at end of any loops
    if (data.sliceA.loopCounter >= data.sliceA.loopLength){
        sf_seek(data.sliceA.infile, data.sliceA.start, SEEK_SET);
        data.sliceA.loopCounter = 0;
    }
    if (data.sliceB.loopCounter >= data.sliceB.loopLength){
        sf_seek(data.sliceB.infile, data.sliceB.start, SEEK_SET);
        data.sliceB.loopCounter = 0;
    }
    if (data.sliceC.loopCounter >= data.sliceC.loopLength){
        sf_seek(data.sliceC.infile, data.sliceC.start, SEEK_SET);
        data.sliceC.loopCounter = 0;
    }
    if (data.sliceD.loopCounter >= data.sliceD.loopLength){
        sf_seek(data.sliceD.infile, data.sliceD.start, SEEK_SET);
        data.sliceD.loopCounter = 0;
    }
   
}
//-----------------------------------------------------------------------------
// Name: muteSlice
// Desc: Mute on/off for slices
//-----------------------------------------------------------------------------
void muteSlice()
{
    switch (data.sliceSelector){
            case 0:
                if (data.sliceA.volume != 0){
                    data.sliceA.volume = 0;
                }
                else {
                    data.sliceA.volume = INIT_VOLUME;
                }
                
                break;
    
            case 1:
                if (data.sliceB.volume != 0){
                    data.sliceB.volume = 0;
                }
                else {
                    data.sliceB.volume = INIT_VOLUME;
                }
                break;

            case 2:
                if (data.sliceC.volume != 0){
                    data.sliceC.volume = 0;
                }
                else {
                    data.sliceC.volume = INIT_VOLUME;
                }
                break;
        
            case 3:
                if (data.sliceD.volume != 0){
                    data.sliceD.volume = 0;
                }
                else {
                    data.sliceD.volume = INIT_VOLUME;
                }
                break;   
        } 
}
//-----------------------------------------------------------------------------
// Name: volumeIncrease (outputBuffer)
// Desc: increase the volume
//-----------------------------------------------------------------------------

void volumeIncrease()

{
    switch (data.sliceSelector){
            case 0:
                if (data.sliceA.volume != 1){
                    data.sliceA.volume += VOLUME_INCR;
                }
                break;

            case 1:
                if (data.sliceB.volume != 1){
                
                    data.sliceB.volume += VOLUME_INCR;
                }
                break;

            case 2:
                if (data.sliceC.volume != 1){
                    data.sliceC.volume += VOLUME_INCR;
                }
                break;

            case 3:
                if (data.sliceD.volume != 1){
                    data.sliceD.volume += VOLUME_INCR;
                }
                break;
        }
}

//-----------------------------------------------------------------------------
// Name: volumeDecrease (outputBuffer)
// Desc: decrease the volume
//-----------------------------------------------------------------------------

void volumeDecrease()

{
    switch (data.sliceSelector){
            case 0:
                if (data.sliceA.volume != 0){
                    data.sliceA.volume -= VOLUME_INCR;
                }
                break;

            case 1:
                if (data.sliceB.volume != 0){
                    data.sliceB.volume -= VOLUME_INCR;
                }
                break;

            case 2:
                if (data.sliceC.volume != 0){
                    data.sliceC.volume -= VOLUME_INCR;
                }
                break;

            case 3:
                if (data.sliceD.volume != 0){
                    data.sliceD.volume -= VOLUME_INCR;
                }
                break;
        }
}

void decreaseLowpass(){
     switch (data.sliceSelector){
            case 0:
                data.sliceA.lowpass -= LOWPASS_INCR;
                if (data.sliceA.lowpass < 0) {
                    data.sliceA.lowpass = 0;
                }
                break;

            case 1:
                data.sliceB.lowpass -= LOWPASS_INCR;
                if (data.sliceB.lowpass < 0) {
                    data.sliceB.lowpass = 0;
                }
                break;

            case 2:
                data.sliceC.lowpass -= LOWPASS_INCR;
                if (data.sliceC.lowpass < 0) {
                    data.sliceC.lowpass = 0;
                }
                break;

            case 3:
                data.sliceD.lowpass -= LOWPASS_INCR;
                if (data.sliceD.lowpass < 0) {
                    data.sliceD.lowpass = 0;
                }
                break;
        }

}
void increaseLowpass(){
     switch (data.sliceSelector){
            case 0:
                data.sliceA.lowpass += LOWPASS_INCR;
                if (data.sliceA.lowpass > WINDOW_SIZE/2) {
                    data.sliceA.lowpass = WINDOW_SIZE/2;
                }
                break;

            case 1:
                data.sliceB.lowpass += LOWPASS_INCR;
                if (data.sliceB.lowpass > WINDOW_SIZE/2) {
                    data.sliceB.lowpass = WINDOW_SIZE/2;
                }
                break;

            case 2:
                data.sliceC.lowpass += LOWPASS_INCR;
                if (data.sliceC.lowpass > WINDOW_SIZE/2) {
                    data.sliceC.lowpass = WINDOW_SIZE/2;
                }
                break;

            case 3:
                data.sliceD.lowpass += LOWPASS_INCR;
                if (data.sliceD.lowpass > WINDOW_SIZE/2) {
                    data.sliceD.lowpass = WINDOW_SIZE/2;
                }
                break;
        }

}

void decreaseHighpass()
{
    switch (data.sliceSelector){
            case 0:
                data.sliceA.highpass -= HIGHPASS_INCR;
                if (data.sliceA.highpass < 0) {
                    data.sliceA.highpass = 0;
                }
                break;

            case 1:
                data.sliceB.highpass -= HIGHPASS_INCR;
                if (data.sliceB.highpass < 0) {
                    data.sliceB.highpass = 0;
                }
                break;

            case 2:
                data.sliceC.highpass -= HIGHPASS_INCR;
                if (data.sliceC.highpass < 0) {
                    data.sliceC.highpass = 0;
                }
                break;

            case 3:
                data.sliceD.highpass -= HIGHPASS_INCR;
                if (data.sliceD.highpass < 0) {
                    data.sliceD.highpass = 0;
                }
                break;
      }

}

void increaseHighpass()
{
    switch (data.sliceSelector){
            case 0:
                data.sliceA.highpass += HIGHPASS_INCR;
                if (data.sliceA.highpass > WINDOW_SIZE/2) {
                    data.sliceA.highpass = WINDOW_SIZE/2;
                }
                break;

            case 1:
                data.sliceB.highpass += HIGHPASS_INCR;
                if (data.sliceB.highpass > WINDOW_SIZE/2) {
                    data.sliceB.highpass = WINDOW_SIZE/2;
                }
                break;

            case 2:
                data.sliceC.highpass += HIGHPASS_INCR;
                if (data.sliceC.highpass > WINDOW_SIZE/2) {
                    data.sliceC.highpass = WINDOW_SIZE/2;
                }
                break;

            case 3:
                data.sliceD.highpass += HIGHPASS_INCR;
                if (data.sliceD.highpass > WINDOW_SIZE/2) {
                    data.sliceD.highpass = WINDOW_SIZE/2;
                }
                break;
    }

}
