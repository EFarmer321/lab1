//
// modified by: Elias Farmer
// date: 8/27/24
//
// original author: Gordon Griesel
// date:            Fall 2024
// purpose:         OpenGL sample program
//
// This program needs some refactoring.
// We will do this in class together.
//
//
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <chrono>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>

// Measured in milliseconds. If the box takes longer to hit the border, it will turn blue. Otherwise, it turns red.
#define BOX_HIT_FREQUENCY 900

// some structures

class Global
{
public:
	Global()
	{
		this->speed = 90;
		this->xres = 400;
		this->yres = 200;
		this->w = 20.0f;
		this->dir = 5.0f;
		this->r = 255;
		this->g = 0;
		this->b = 0;
		this->pos[0] = 0.0f + this->w;
		this->pos[1] = this->yres / 2.0f;
		this->lastHit = std::chrono::high_resolution_clock::now();
	}

	int speed;
	float w;
	float dir;
	float pos[2];
	std::chrono::high_resolution_clock::time_point lastHit;
	int xres, yres;
	int r, g, b;
} g;

class X11_wrapper
{
private:
	Display *dpy;
	Window win;
	GLXContext glc;

public:
	~X11_wrapper();
	X11_wrapper();
	void set_title();
	bool getXPending();
	XEvent getXNextEvent();
	void swapBuffers();
	void reshape_window(int width, int height);
	void check_resize(XEvent *e);
	void check_mouse(XEvent *e);
	int check_keys(XEvent *e);
} x11;

// Function prototypes
void init_opengl(void);
void physics(void);
void render(void);

int main(int argc, char *argv[])
{
	if (argc >= 2)
		g.dir = atoi(argv[1]);

	Global();
	init_opengl();
	int done = 0;

	// main game loop
	while (!done)
	{
		// look for external events such as keyboard, mouse.
		while (x11.getXPending())
		{
			XEvent e = x11.getXNextEvent();
			x11.check_resize(&e);
			x11.check_mouse(&e);
			done = x11.check_keys(&e);
		}
		physics();
		render();
		x11.swapBuffers();
		usleep(200);
	}
	return 0;
}

X11_wrapper::~X11_wrapper()
{
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

X11_wrapper::X11_wrapper()
{
	GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
	int w = g.xres, h = g.yres;
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL)
	{
		std::cout << "\n\tcannot connect to X server\n"
				  << std::endl;
		exit(EXIT_FAILURE);
	}
	Window root = DefaultRootWindow(dpy);
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	if (vi == NULL)
	{
		std::cout << "\n\tno appropriate visual found\n"
				  << std::endl;
		exit(EXIT_FAILURE);
	}
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.event_mask =
		ExposureMask | KeyPressMask | KeyReleaseMask |
		ButtonPress | ButtonReleaseMask |
		PointerMotionMask |
		StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
						InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}

void X11_wrapper::set_title()
{
	// Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "3350 Lab-1");
}

bool X11_wrapper::getXPending()
{
	// See if there are pending events.
	return XPending(dpy);
}

XEvent X11_wrapper::getXNextEvent()
{
	// Get a pending event.
	XEvent e;
	XNextEvent(dpy, &e);
	return e;
}

void X11_wrapper::swapBuffers()
{
	glXSwapBuffers(dpy, win);
}

void X11_wrapper::reshape_window(int width, int height)
{
	// Window has been resized.
	g.xres = width;
	g.yres = height;

	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
}

void X11_wrapper::check_resize(XEvent *e)
{
	// The ConfigureNotify is sent by the
	// server if the window is resized.
	if (e->type != ConfigureNotify)
		return;
	XConfigureEvent xce = e->xconfigure;
	if (xce.width != g.xres || xce.height != g.yres)
	{
		// Window size did change.
		reshape_window(xce.width, xce.height);
	}
}
//-----------------------------------------------------------------------------

void X11_wrapper::check_mouse(XEvent *e)
{
	static int savex = 0;
	static int savey = 0;

	// Weed out non-mouse events
	if (e->type != ButtonRelease &&
		e->type != ButtonPress &&
		e->type != MotionNotify)
	{
		// This is not a mouse event that we care about.
		return;
	}
	//
	if (e->type == ButtonRelease)
	{
		return;
	}
	if (e->type == ButtonPress)
	{
		if (e->xbutton.button == 1)
		{
			// Left button was pressed.
			// int y = g.yres - e->xbutton.y;
			return;
		}
		if (e->xbutton.button == 3)
		{
			// Right button was pressed.
			return;
		}
	}
	if (e->type == MotionNotify)
	{
		// The mouse moved!
		if (savex != e->xbutton.x || savey != e->xbutton.y)
		{
			savex = e->xbutton.x;
			savey = e->xbutton.y;
			// Code placed here will execute whenever the mouse moves.
		}
	}
}

int X11_wrapper::check_keys(XEvent *e)
{
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	int key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyPress)
	{
		switch (key)
		{
		case XK_a:
			// the 'a' key was pressed
			break;
		case XK_Escape:
			// Escape key was pressed
			return 1;
		}
	}
	return 0;
}

void init_opengl(void)
{
	// OpenGL initialization
	glViewport(0, 0, g.xres, g.yres);

	// Initialize projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Set 2D mode
	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();
	// Set 2D mode (no perspective)
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	// Set the screen background color
	// Set when screen is cleared
	glClearColor(0.1, 0.1, 0.1, 1.0);
}

void physics()
{
	g.pos[0] += g.dir;

	int currentDirection = g.dir;

	// Update position
	if (g.pos[0] >= (g.xres - g.w))
	{
		g.pos[0] = (g.xres - g.w);
		g.dir = -g.dir;
	}
	if (g.pos[0] <= g.w)
	{
		g.pos[0] = g.w;
		g.dir = -g.dir;
	}

	// Check if direction has changed
	if (currentDirection != g.dir)
	{
		// Update color based on bounce frequency.

		auto currentTime = std::chrono::high_resolution_clock::now();

		auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - g.lastHit).count();

		if (elapsedTime > BOX_HIT_FREQUENCY)
		{
			g.r = 0;
			g.b = 255;
		}
		else
		{
			g.r = 255;
			g.b = 0;
		}

		g.lastHit = currentTime;
	}
}

void render()
{
	// Don't render the box
	if (g.xres < g.w * 2)
		return;

	glClear(GL_COLOR_BUFFER_BIT);
	// draw the box
	glPushMatrix();
	glColor3ub(g.r, g.g, g.b);
	glTranslatef(g.pos[0], g.pos[1], 0.0f);
	glBegin(GL_QUADS);
	glVertex2f(-g.w, -g.w);
	glVertex2f(-g.w, g.w);
	glVertex2f(g.w, g.w);
	glVertex2f(g.w, -g.w);
	glEnd();
	glPopMatrix();
}