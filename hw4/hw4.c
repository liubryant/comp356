/** View of several solid cubes (each face a solid color).
 *
 *  @author Evan Carmi
 */

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __MACOSX__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#elif defined __LINUX__ || defined __CYGWIN__
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include "geom356.h"
#include "debug.h"
#include "maze.h"

#define max(a, b) ((a < b ? b : a))

// Define Window Defaults.
#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600
#define DEFAULT_WINDOW_XORIG 0
#define DEFAULT_WINDOW_YORIG 0
#define WINDOW_TITLE "3D Interactive Maze Game"

// the maze itself
maze_t* maze ;

// Window data.
int win_width ;
int win_height ;

// Viewing data.
// Eye is located distance eye_r from origin; angle eye_theta from +x axis
// in xz-plane; angle eye_phi from xz-plane.
float eye_r, eye_theta, eye_phi ;
point3_t look_at = {0.0f, 0.0f, 0.0f} ;          // Look-at position (world coordinates).
vector3_t up_dir = {0.0f, 1.0f, 0.0f} ;              // Up direction.
#define EYE_DIST_INCR .1f
#define EYE_THETA_INCR 2.0*M_PI/180 ;
#define EYE_PHI_INCR 2.0*M_PI/180 ;

// View-volume specification in camera frame basis.
float view_plane_near = 4.0f ;
float view_plane_far = 100.0f ; 


// Display callback:  draws the maze.
void initialize_maze(int, int) ;
void draw_maze() ;

// Callbacks.
void handle_display(void) ;
void handle_resize(int, int) ;
void handle_key(unsigned char, int, int) ;
void handle_special_key(int, int, int) ;

// Application functions.
void init() ;

// GL initialization.
void init_gl(int, int) ;

// Application data.
bool do_print_position ;    // Whether or not to print the position.

// Materials and lights.  Note that colors are RGBA colors.  OpenGL lights
// have diffuse, specular, and ambient components; we'll default to setting
// the first two equal to each other and the OpenGL default for ambient
// (black).  And the position is in homogeneous coordinates, so a w-value
// of 0 means "infinitely far away."
typedef struct _material_t {
    GLfloat ambient[4] ;
    GLfloat diffuse[4] ;
    GLfloat specular[4] ;
    GLfloat phong_exp ;
} material_t ;

typedef struct _light_t {
    GLfloat position[4] ;
    GLfloat color[4] ;
} light_t ;

GLfloat BLACK[4] = {0.0, 0.0, 0.0, 1.0} ;

light_t far_light = {
    {20.0, 10.0, 0.0, 1.0},
    {0.75, 0.75, 0.75, 1.0}
} ;

material_t gold = {
    {.10f, .084f, 0.0f, 1.0f},
    {1.0f, .84f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    1000.0f
} ;

material_t red_plastic = {
    {1.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    1000.0f
} ;

material_t green_plastic = {
    {0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    1000.0f
} ;

material_t blue_plastic = {
    {0.0f, 0.0f, 1.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    1000.0f
} ;

int main(int argc, char **argv) {

    // Initialize the drawing window.
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH ) ;
    glutInitWindowSize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT) ;
    glutInitWindowPosition(DEFAULT_WINDOW_XORIG, DEFAULT_WINDOW_YORIG) ;
    glutInit(&argc, argv) ;

    // Handle command line arguments.
    do_print_position = false ;
    for (int i=0; i<argc; ++i) {
        if (strcmp(argv[i], "--position") == 0) do_print_position = true ;
    }

    // Create the main window.
    debug("Creating window") ;
    glutCreateWindow(WINDOW_TITLE) ;

    debug("Setting callbacks") ;
    glutReshapeFunc(handle_resize) ;
    glutKeyboardFunc(handle_key) ;
    glutSpecialFunc(handle_special_key) ;
    glutDisplayFunc(handle_display) ;

    // Initialize the maze.
    int maze_width = atoi(argv[1]) ;
    int maze_height = atoi(argv[2]) ;
    initialize_maze(maze_height, maze_width) ;


    // Initialize GL.
    init_gl(maze_width, maze_height) ;

    // Enter the main event loop.
    debug("Main loop") ;
    glutMainLoop() ;

    return EXIT_SUCCESS ;
}

/**
 * Initialize OpenGL. Set our world frame to be the width and height of the maze.
 */
void init_gl(int maze_height, int maze_width) {
    debug("init_gl") ;
    // GL initialization.
    glEnable(GL_DEPTH_TEST) ;
    // glEnable(GL_CULL_FACE) ;
    // glCullFace(GL_BACK) ;
    glEnable(GL_LIGHTING) ;
    glEnable(GL_LIGHT0) ;
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1) ;
    
    // Background color.
    //glClearColor(0.0f, 0.0f, 0.0f, 0.0f) ;
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f) ;


    // Viewpoint position.
    eye_r = sqrt(48) ;
    eye_theta = M_PI_4 ;
    eye_phi = M_PI_4 ;

    // Set the lights.
    set_lights() ;

    // Set the viewpoint.
    set_camera() ;

}

/*  Initialize the maze by building all possible walls.
 */
void initialize_maze(int maze_height, int maze_width) {

    maze = make_maze(maze_height, maze_width, time(NULL)) ;

}




/** Set the camera transformation.  Viewpoint is given by the eye
 *  coordinates; we look at the origin with up-direction along the
 *  +y axis.
 */
void set_camera() {
    debug("set_camera()") ;

    // Cartesian coordinates of the viewpoint.
    float eye_x, eye_y, eye_z ;
    eye_y = eye_r*sin(eye_phi) ;
    float eye_xz_r = eye_r*cos(eye_phi) ;
    eye_x = eye_xz_r*cos(eye_theta) ;
    eye_z = eye_xz_r*sin(eye_theta) ;

    // Set the model-view (i.e., camera) transform.
    glMatrixMode(GL_MODELVIEW) ;
    glLoadIdentity() ;
    gluLookAt(eye_x, eye_y, eye_z,
            look_at.x, look_at.y, look_at.z,
            up_dir.x, up_dir.y, up_dir.z) ;
}

/** Set the projection and viewport transformations.  We use perspective
 *  projection and always match the aspect ratio of the screen window
 *  with vertical field-of-view 60 degrees and always map to the entire
 *  screen window.
 */
void set_projection_viewport() {
    debug("set_projection_viewport()") ;

    // Set perspective projection transform.
    glMatrixMode(GL_PROJECTION) ;
    glLoadIdentity() ;
    gluPerspective(60.0, (GLdouble)win_width/win_height, view_plane_near, 
            view_plane_far) ;

    // Set the viewport transform.
    glViewport(0, 0, win_width, win_height) ;
}

/** Set the light colors.  Since the position of the light
 *  is subject to the current model-view transform, and we have
 *  specified the light position in world-frame coordinates,
 *  we want to set the light position after setting the camera
 *  transformation; since the camera transformation may change in response
 *  to keyboard events, we ensure this by setting the light position
 *  in the display callback.
 *
 *  It is also easy to "attach" a light to the viewer.  In that case,
 *  just specify the light position in the camera frame and make sure
 *  to set its position while the camera transformation is the identity!
 */
void set_lights() {
    debug("set_lights()") ;

    light_t* light = &far_light ;
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light->color) ;
    glLightfv(GL_LIGHT0, GL_AMBIENT, BLACK) ;
    glLightfv(GL_LIGHT0, GL_SPECULAR, light->color) ;
}



/** Handle a resize event by recording the new width and height.
 *  
 *  @param width the new width of the window.
 *  @param height the new height of the window.
 */
void handle_resize(int width, int height) {
    debug("handle_resize(%d, %d)\n", width, height) ;

    win_width = width ;
    win_height = height ;

    set_projection_viewport() ;

    glutPostRedisplay() ;
}

/** Draw the coordinate axes as line segments from -100 to +100 along
 *  the corresponding axis.
 */
void draw_axes() {
    // Disable lighting so that we can set the color with glColor*.
    // Remember to enable it when done!
    glDisable(GL_LIGHTING) ;
    glBegin(GL_LINES) ;
    glColor3f(0.0, 0.0, 1.0) ;
    glVertex3f(0.0f, 0.0f, -100.0f) ;
    glVertex3f(0.0f, 0.0f, 100.0f) ;
    glColor3f(1.0, 0.0, 0.0) ;
    glVertex3f(-100.0f, 0.0f, 0.0f) ;
    glVertex3f(100.0f, 0.0f, 0.0f) ;
    glColor3f(0.0, 1.0, 0.0) ;
    glVertex3f(0.0f, -100.0f, 0.0f) ;
    glVertex3f(0.0f, 100.0f, 0.0f) ;
    glEnd() ;
    glEnable(GL_LIGHTING) ;
}


/**
 * Draw a square of side-length 2 in the xy-plane centered at the origin. This
 * is used to color the start and end cells of the maze.
 */
void draw_square() {
    debug("draw_square()");

    // Specify the material for the square.
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green_plastic.diffuse) ;
    glMaterialfv(GL_FRONT, GL_SPECULAR, green_plastic.specular) ;
    glMaterialf(GL_FRONT, GL_SHININESS, green_plastic.phong_exp) ;

    glBegin(GL_QUADS);
    // z=0 plane.
    glNormal3f(0.0, 0.0, 0.0) ;
    glVertex3f(-1.0, -1.0, 0.0) ;
    glVertex3f(1.0, -1.0, 0.0) ;
    glVertex3f(1.0, 1.0, 0.0) ;
    glVertex3f(-1.0, 1.0, 0.0) ;
    glEnd();
}

/**
 * Draw a wall in the maze in the world frame by translating and rotating a
 * draw_square object..
 * @param row - the row of the maze cell.
 * @param col - the column of the maze cell.
 * @param direction - in which direction to draw the wall. This is an unsigned
 * char defined in maze.h and maze.c.
 */
void draw_wall(float row, float col, unsigned char direction) {
    debug("draw_wall()");

    // Make sure we're talking about the m-v xfrm stack.
    glMatrixMode(GL_MODELVIEW) ;

    // Push a copy of the current m-v xfrm onto the stack.
    glPushMatrix() ;

    // Define the model xfrm.
    glTranslatef(row, 0.0, col);

    // Rotate wall depending on direction.

    switch (direction) {
        debug("1");
        case NORTH:
            glTranslatef(0.0f, 0.0f, 1.0f);
        case EAST:
            glTranslatef(1.0f, 0.0f, 0.0f);
            glRotatef(90.0f, 0, 1, 0);
        case SOUTH:
        case WEST:
            glRotatef(90.0f, 0, 1, 0);
        default:
            debug("default %x", direction);
            //assert(0) ;
    }
    //assert(0) ;
    ;

    // Draw the wall.
    draw_rect();

    // Undo the model xfrm.
    glPopMatrix();
}


/**
 * Draw a rectangular solid length 1, width .25, height 1, along the x-axis
 * centered at the origin. This object should be used create the floor and
 * walls of the maze.
 */
void draw_rect() {
    debug("draw_rect()");

    // Specify the material for the wall.
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, gold.diffuse) ;
    glMaterialfv(GL_FRONT, GL_SPECULAR, gold.specular) ;
    glMaterialf(GL_FRONT, GL_SHININESS, gold.phong_exp) ;

    glBegin(GL_QUADS);
    // z=.125 plane.
    glNormal3f(0.0, 0.0, 1.0) ;
    glVertex3f(-0.5, -0.5, 0.125) ;
    glVertex3f(0.5, -0.5, 0.125) ;
    glVertex3f(0.5, 0.5, 0.125) ;
    glVertex3f(-0.5, 0.5, 0.125) ;

    // z=-.125 plane.
    glNormal3f(0.0, 0.0, -1.0) ;
    glVertex3f(-0.5, -0.5, -0.125) ;
    glVertex3f(-0.5, 0.5, -0.125) ;
    glVertex3f(0.5, 0.5, -0.125) ;
    glVertex3f(0.5, -0.5, -0.125) ;

    // x=0.5 plane.
    glNormal3f(1.0, 0.0, 0.0) ;
    glVertex3f(0.5, -0.5, 0.125) ;
    glVertex3f(0.5, -0.5, -0.125) ;
    glVertex3f(0.5, 0.5, -0.125) ;
    glVertex3f(0.5, 0.5, 0.125) ;

    // x=-0.5 plane.
    glNormal3f(-1.0, 0.0, 0.0) ;
    glVertex3f(-0.5, -0.5, 0.125) ;
    glVertex3f(-0.5, 0.5, 0.125) ;
    glVertex3f(-0.5, 0.5, -0.125) ;
    glVertex3f(-0.5, -0.5, -0.125) ;

    // y=0.5 plane.
    glNormal3f(0.0, 1.0, 0.0) ;
    glVertex3f(-0.5, 0.5, 0.125) ;
    glVertex3f(0.5, 0.5, 0.125) ;
    glVertex3f(0.5, 0.5, -0.125) ;
    glVertex3f(-0.5, 0.5, -0.125) ;

    // y=-0.5 plane.
    glNormal3f(0.0, -1.0, 0.0) ;
    glVertex3f(-0.5, -0.5, 0.125) ;
    glVertex3f(-0.5, -0.5, -0.125) ;
    glVertex3f(0.5, -0.5, -0.125) ;
    glVertex3f(0.5, -0.5, 0.125) ;
    glEnd();
}

/**
 * Draw the maze by transforming draw_rect() and draw_square() objects. The maze will be drawn on the x-z plane. 
 */
void draw_maze() {
    debug("draw_maze()");

    cell_t* maze_start = get_start(maze) ;
    cell_t* maze_end = get_end(maze) ;
    
    int maze_width = get_ncols(maze) ;
    int maze_height = get_ncols(maze) ;

    // Draw squares on start and end cell

    // Make sure we're talking about the m-v xfrm stack.
    glMatrixMode(GL_MODELVIEW) ;

    // Draw the walls.  First draw the west and south exterior walls, then
    // draw any north or west walls of each cell.
    for (float i=0.0f; i<maze_width; ++i) {
        for (float j=0.0f; j<maze_height; ++j) {
            
            // Draw west exterior walls - if i==0
            if (i==0.0f) {
                draw_rect(i, j, SOUTH);
            }


            // Draw south exterior walls - if j==0
            if (has_wall(maze, get_cell(maze, j, i), NORTH)) {

                // Push a copy of the current m-v xfrm onto the stack.
                glPushMatrix() ;

                // Define the model xfrm.
                glTranslatef(i, 0.0, j);

                // Draw the wall.
                draw_rect();

                // Undo the model xfrm.
                glPopMatrix();
            }

        }
    }

}

/** Draw num_faces of the cube.
 */

void draw_cube() {
    debug("draw_cube()") ;

    // Specify the material for the cube.
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, gold.diffuse) ;
    glMaterialfv(GL_FRONT, GL_SPECULAR, gold.specular) ;
    glMaterialf(GL_FRONT, GL_SHININESS, gold.phong_exp) ;

    // Draw cube as a sequence of GL_QUADS.  Because we are using GL
    // lighting, we must specify the normal to associate to each vertex.
    // The normal is part of the state, so if we want to use the same
    // normal for many vertices, we only have to set the normal once.
    glBegin(GL_QUADS) ;

    // z=1 plane.
    glNormal3f(0.0, 0.0, 1.0) ;
    glVertex3f(-1.0, -1.0, 1.0) ;
    glVertex3f(1.0, -1.0, 1.0) ;
    glVertex3f(1.0, 1.0, 1.0) ;
    glVertex3f(-1.0, 1.0, 1.0) ;

    // z=-1 plane.
    glNormal3f(0.0, 0.0, -1.0) ;
    glVertex3f(-1.0, -1.0, -1.0) ;
    glVertex3f(-1.0, 1.0, -1.0) ;
    glVertex3f(1.0, 1.0, -1.0) ;
    glVertex3f(1.0, -1.0, -1.0) ;

    // x=1 plane.
    glNormal3f(1.0, 0.0, 0.0) ;
    glVertex3f(1.0, -1.0, 1.0) ;
    glVertex3f(1.0, -1.0, -1.0) ;
    glVertex3f(1.0, 1.0, -1.0) ;
    glVertex3f(1.0, 1.0, 1.0) ;

    // x=-1 plane.
    glNormal3f(-1.0, 0.0, 0.0) ;
    glVertex3f(-1.0, -1.0, 1.0) ;
    glVertex3f(-1.0, 1.0, 1.0) ;
    glVertex3f(-1.0, 1.0, -1.0) ;
    glVertex3f(-1.0, -1.0, -1.0) ;

    // y=1 plane.
    glNormal3f(0.0, 1.0, 0.0) ;
    glVertex3f(-1.0, 1.0, 1.0) ;
    glVertex3f(1.0, 1.0, 1.0) ;
    glVertex3f(1.0, 1.0, -1.0) ;
    glVertex3f(-1.0, 1.0, -1.0) ;

    // y=-1 plane.
    glNormal3f(0.0, -1.0, 0.0) ;
    glVertex3f(-1.0, -1.0, 1.0) ;
    glVertex3f(-1.0, -1.0, -1.0) ;
    glVertex3f(1.0, -1.0, -1.0) ;
    glVertex3f(1.0, -1.0, 1.0) ;

    glEnd() ;
}

/** Draw a string on the screen.  The string will be drawn in the current
 *  color and at the current raster position.
 */
void draw_string(char* s) {
    for (char *c=s; *c!='\0'; ++c) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, *c) ;
    }
}

/** Print the position (distance, theta, and phi) on the screen.
 */
void print_position() {
    glColor3f(1.0f, 1.0f, 1.0f) ;

    glWindowPos2s(10, 50) ;
    char* s ;
    asprintf(&s, "Eye distance: %f", eye_r) ;
    draw_string(s) ;
    free(s) ;

    glWindowPos2s(10, 30) ;
    asprintf(&s, "Theta = %f", eye_theta) ;
    draw_string(s) ;
    free(s) ;

    glWindowPos2s(10, 10) ;
    asprintf(&s, "Phi = %f", eye_phi) ;
    draw_string(s) ;
    free(s) ;
}

/** Handle a display request by clearing the screen, drawing the axes
 *  and cube, and printing the viewpoint position.
 */
void handle_display() {
    debug("handle_display()") ;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;

    // Note that the lights are sent through the pipeline, and a light's
    // position is modified by the model-view transformation.  By setting the
    // light's position here, we ensure that the light lives in a fixed
    // place in the world.  See set_lights() for more info.
    glLightfv(GL_LIGHT0, GL_POSITION, far_light.position) ;

    draw_axes();
    
    draw_rect();
    draw_square();
    //draw_maze();

    draw_wall(3.0, 0.0, NORTH);
    draw_wall(3.0, 0.0, WEST);
    draw_wall(3.0, 0.0, SOUTH);
    draw_wall(3.0, 0.0, EAST);

    //draw_cube();
    // Draw several cubes, all as transforms of the basic cube.
    // To do so, we *start* the model-view (viewing) transform with
    // a "model" transform (which, in previous sample code, we think
    // of as the identity).  It is usually easiest to think of the model
    // transform as transforming vertices that are specified in the world
    // frame (both source and target) rather than as a change-of-frame
    // transformation.

    // To "insert" a model transformation into the beginning of the
    // model-view transform, we need to multiply the current m-v xfrm
    // on the left by the corresponding model xfrm.  That's just what the
    // glTranslate, glRotate, etc., functions do.  

    // We are likely to want a different model transform for each object
    // that we draw.  We could undo the previous tranform by multiplying
    // by the inverse of the previous transform.  But GL provides an
    // easier solution:  the "m-v xfrm" is actually on a stack of xfrms,
    // and the current m-v xfrm is whatever is on the top of that stack.
    // To make it easy to "undo" a model xfrm, we first push a copy of the
    // current xfrm onto the stack (glPushStack()), then multiply by the
    // model xfrm.  When we want to undo the model xfrm, we just pop the
    // stack.
    
    // Make sure we're talking about the m-v xfrm stack.
    /*
    glMatrixMode(GL_MODELVIEW) ;

    // Push a copy of the current m-v xfrm onto the stack.
    glPushMatrix() ;

    // Define the model xfrm.
    glTranslatef(0.0, 0.0, -5.0) ;

    // Draw the cube.
    draw_cube() ;

    // Undo the model xfrm.
    glPopMatrix() ;

    // Remember that in terms of our conventions for reprsenting xfrms as
    // matrices, the GL transform function F (e.g., glTranslate) multiplies
    // the current matrix on the right by the matrix defined by F.  The
    // upshot is:  the last operation specified is the first operation
    // applied.  So here we rotate the cube then translate it.  You will
    // almost always want to do your translation last!
    glPushMatrix() ;
    glTranslatef(-10.0, 2.0, 0.0) ;
    glRotatef(40, 10, 0, 1) ;
    draw_cube() ;
    glPopMatrix() ;
    */

    if (do_print_position) print_position() ;

    glFlush() ;
}

/** Handle keyboard events:
 *  
 *  - SPACE:  increment the number of faces of the cube to draw.
 *  - +, -:  increment/decrement the distance to the origin of the viewpoint.
 *
 *  Redisplays will be requested from every key event.
 *
 *  @param key the key that was pressed.
 *  @param x the mouse x-position when <code>key</code> was pressed.
 *  @param y the mouse y-position when <code>key</code> was pressed.
 */
void handle_key(unsigned char key, int x, int y) {

    switch (key) {
        case '+':
            debug("handle_key() - increase eye dist.");
            eye_r += EYE_DIST_INCR ;
            set_camera() ;
            break ;
        case '-':
            debug("handle_key() - decrease eye dist.");
            eye_r = max(0.0f, eye_r - EYE_DIST_INCR) ;
            set_camera() ;
            break ;
        default:
            break ;
    }
    glutPostRedisplay() ;
}

/** Handle keyboard events:
 *
 *  - LEFT, RIGHT:  increment/decrement theta.
 *  - UP, DOWN:  increment/decrement phi.
 *
 *  Redisplays will be requested for every key event.
 *
 *  @param key the key that was pressed.
 *  @param x the mouse x-position when <code>key</code> was pressed.
 *  @param y the mouse y-position when <code>key</code> was pressed.
 */
void handle_special_key(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT:
            eye_theta += EYE_THETA_INCR ;
            if (eye_theta >= 2*M_PI) eye_theta -= 2*M_PI ;
            break ;
        case GLUT_KEY_RIGHT:
            eye_theta -= EYE_THETA_INCR ;
            if (eye_theta < 0) eye_theta += 2*M_PI ;
            break ;
        case GLUT_KEY_UP:
            eye_phi += EYE_PHI_INCR ;
            if (eye_phi >= 2*M_PI) eye_phi -= 2*M_PI ;
            break ;
        case GLUT_KEY_DOWN:
            eye_phi -= EYE_PHI_INCR ;
            if (eye_phi < 0) eye_phi += 2*M_PI ;
            break ;
        default:
            break ;
    }
    set_camera() ;
    glutPostRedisplay() ;

}
