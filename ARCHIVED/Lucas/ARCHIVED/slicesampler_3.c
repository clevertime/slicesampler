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
#include <stdbool.h>
#include <sndfile.h>//libsndfile library

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
#define BUFFER_SIZE             1024
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

typedef double  MY_TYPE;
typedef char BYTE;   // 8-bit unsigned entity.

typedef struct {
    int x;
    int y;
} Pos;

typedef struct {
    SNDFILE *infile;
    SF_INFO sfinfoInput;
    float buffer[BUFFER_SIZE * STEREO];
    float frequency;
    float volume;
    bool playing;
    int start;
} sndFile;

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


//Initialize sound file struct 
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


// Translation
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
void drawWindowedTimeDomain(SAMPLE * buffer);
void reshapeFunc( int width, int height );
void keyboardFunc( unsigned char, int, int );
void specialKey( int key, int x, int y );
void specialUpKey( int key, int x, int y);
void initialize_graphics( );
void initialize_glut(int argc, char *argv[]);
void initialize_audio(char * audioFilename);
void stop_portAudio();
void startstop();
//Mouse callback functions
void mouseFunc(int button, int state, int x, int y);
void mouseMotionFunc(int x, int y);

// Function copied from the FFT library
void apply_window( float * data, float * window, unsigned long length )
{
   unsigned long i;
   for( i = 0; i < length; i++ )
       data[i] *= window[i];
}

//-----------------------------------------------------------------------------
// name: help()
// desc: ...
//-----------------------------------------------------------------------------
void help()
{
    printf( "----------------------------------------------------\n" );
    printf( "SLICE SAMPLER\n" );
    printf( "----------------------------------------------------\n" );
    printf( "'h' - print this help message\n" );
    printf( "'SPACEBAR' - Start and stop audio\n");
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

    int i, readcount;


    /* read data from file 1 */
    readcount = sf_readf_float (data.infile, data.buffer, framesPerBuffer);  

    /* if end of file 1 reached, rewind */
    if (readcount < framesPerBuffer) {
        sf_seek(data.infile, 0, SEEK_SET);
        readcount = sf_readf_float (data.infile, data.buffer+(readcount*STEREO), framesPerBuffer-readcount);  
    }

    for (i = 0; i < framesPerBuffer * STEREO; i+=2) {
        out[2 * i] = (data.volume * data.buffer[i]);
        out[2 * i +1] = (data.volume * data.buffer[i+1]);
    }

    //apply_window(g_buffer, g_window, framesPerBuffer);

    //set flag
    g_ready = true;

    return paContinue;
    return 0;
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
    glutCreateWindow( "SLICE SAMPLER");
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
    // do our own initialization
    initialize_graphics( );  
}

void hanning( float * window, unsigned long length )
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
}


//-----------------------------------------------------------------------------
// Name: initialize_audio( RtAudio *dac )
// Desc: Initializes PortAudio with the global vars and the stream
//-----------------------------------------------------------------------------
void initialize_audio(char * audioFilename) {

    //Clear structs
    memset(&data.sfinfoInput, 0, sizeof(data.sfinfoInput));

    //Open Input Audio File
    data.infile = sf_open(audioFilename, SFM_READ, &data.sfinfoInput);
    if (data.infile == NULL) {
        printf ("Error: could not open file: %s\n", audioFilename) ;
        puts(sf_strerror (NULL)) ;
        exit(1);
    }

    printf("Audio file 1: Frames: %d Channels: %d Samplerate: %d\n", 
            (int)data.sfinfoInput.frames, data.sfinfoInput.channels, data.sfinfoInput.samplerate);

    //check if audio file is between 10 seconds and 5 minutes
    if (data.sfinfoInput.frames / SAMPLING_RATE >= 300 || data.sfinfoInput.frames / SAMPLING_RATE < 10){
        printf("Error: Audio file must be between 10 seconds and 5 minutes in length.\n");
        exit(1);
    }

    //hanning(g_window, g_buffer_size);

    PaStreamParameters outputParameters;
    PaStreamParameters inputParameters;
    PaError err;

    //Initilialize struct data
    data.frequency = INIT_FREQUENCY;
    data.volume = INIT_VOLUME;
    data.start = INIT_START;
    data.playing = false;

    /* Initialize PortAudio */
    Pa_Initialize();

    /* Set input stream parameters */
    inputParameters.device = Pa_GetDefaultInputDevice();
    inputParameters.channelCount = MONO;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = 
        Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    /* Set output stream parameters */
    outputParameters.device = Pa_GetDefaultOutputDevice();
    outputParameters.channelCount = g_channels;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = 
        Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    /* Open audio stream */
    err = Pa_OpenStream( &g_stream,
            &inputParameters,
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
            if (data.volume == 0){
                data.volume = INIT_VOLUME;
            }
            else {
                data.volume = 0;
            }
            
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
            sf_close(data.infile);
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
      printf("[synthGL]: LEFT\n");
      g_key_rotate_y = false;
      break;
    case GLUT_KEY_RIGHT :    // Arrow key right is unpressed
      printf("[synthGL]: RIGHT\n");
      g_key_rotate_y = false;
      break;
    case GLUT_KEY_UP :        // Arrow key up is unpressed
      printf("[synthGL]: UP\n");
      g_key_rotate_x = false;
      break;
    case GLUT_KEY_DOWN :    // Arrow key down is unpressed
      printf("[synthGL]: DOWN\n");
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

        //Draw waveform
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.0f);
        glColor3f(1.0f, 0.0f, 0.8f);
        drawWindowedTimeDomain(data.buffer);
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

//-----------------------------------------------------------------------------
// Name: displayFunc( )
// Desc: callback function invoked to draw the client area
//-----------------------------------------------------------------------------
void displayFunc( )
{
    // local variables
    SAMPLE buffer[BUFFER_SIZE];

    // wait for data
    while( !g_ready ) usleep( 1000 );

    // copy currently playing oscillators into buffer
    //memcpy( buffer, out, g_buffer_size * sizeof(SAMPLE) );

    // Hand off to audio callback thread
    g_ready = false;

    // clear the color and depth buffers
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    //draw waveforms
    drawWaveform();
   
    // flush gl commands
    glFlush( );

    // swap the buffers
    glutSwapBuffers( );
}

//-----------------------------------------------------------------------------
// Name: startstop
// Desc: plays audio and stops it
//-----------------------------------------------------------------------------
void startstop( )
{
    if (data.playing){
        data.playing = false;      
    }
    else {
        data.playing = true;
        data.start = 0;
    }
}

