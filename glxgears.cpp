/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This is a port of the infamous "gears" demo to straight GLX (i.e. no GLUT)
 * Port by Brian Paul  23 March 2001
 *
 * See usage() below for command line options.
 */


#include <vector>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

#ifndef GLX_MESA_swap_control
#define GLX_MESA_swap_control 1
typedef int (*PFNGLXGETSWAPINTERVALMESAPROC)(void);
#endif


#define BENCHMARK

#ifdef BENCHMARK

/* XXX this probably isn't very portable */

#include <sys/time.h>
#include <unistd.h>

/* return current time (in seconds) */
static double
current_time(void)
{
   struct timeval tv;
#ifdef __VMS
   (void) gettimeofday(&tv, NULL );
#else
   struct timezone tz;
   (void) gettimeofday(&tv, &tz);
#endif
   return (double) tv.tv_sec + tv.tv_usec / 1000000.0;
}

#else /*BENCHMARK*/

/* dummy */
static double
current_time(void)
{
   /* update this function for other platforms! */
   static double t = 0.0;
   static int warn = 1;
   if (warn) {
      fprintf(stderr, "Warning: current_time() not implemented!!\n");
      warn = 0;
   }
   return t += 1.0;
}

#endif /*BENCHMARK*/



#ifndef M_PI
#define M_PI 3.14159265
#endif


/** Event handler results: */
#define NOP 0
#define EXIT 1
#define DRAW 2

static GLfloat view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
static GLint gear1, gear2, gear3;
static GLfloat angle = 0.0;

static GLboolean fullscreen = GL_FALSE; /* Create a single fullscreen window */
static GLboolean stereo = GL_FALSE;     /* Enable stereo.  */
static GLint samples = 0;               /* Choose visual with at least N samples. */
static GLboolean animate = GL_TRUE;     /* Animation */
static GLfloat eyesep = 5.0;            /* Eye separation. */
static GLfloat fix_point = 40.0;        /* Fixation point distance.  */
static GLfloat left, right, asp;        /* Stereo frustum params.  */

static GLuint shaderProgram = 0;        /* Shader program */

struct vertex {
  GLfloat position[3];
  GLfloat normal[3];
  GLfloat color[3];
};

/*
 *
 *  Draw a gear wheel.  You'll probably want to call this function when
 *  building a display list since we do a lot of trig here.
 * 
 *  Input:  inner_radius - radius of hole at center
 *          outer_radius - radius at center of teeth
 *          width - width of gear
 *          teeth - number of teeth
 *          tooth_depth - depth of tooth
 */
static GLint gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
     GLint teeth, GLfloat tooth_depth, GLfloat red, GLfloat green, GLfloat blue)
{
   GLfloat r0 = inner_radius, r1 = outer_radius - tooth_depth / 2.0, r2 = outer_radius + tooth_depth / 2.0;
   GLfloat da = 2.0 * M_PI / teeth / 4.0;
   std::vector<vertex> buffer;

   for (size_t i = 0; i < (size_t)teeth; i++) {
      GLfloat angle = i * 2.0 * M_PI / teeth;
      GLfloat angle2 = (i+1) * 2.0 * M_PI / teeth;

      GLfloat u = r2 * cos(angle + da) - r1 * cos(angle);
      GLfloat v = r2 * sin(angle + da) - r1 * sin(angle);
      GLfloat len = sqrt(u * u + v * v);
      u /= len;
      v /= len;
      GLfloat u2 = r1 * cos(angle + 3 * da) - r2 * cos(angle + 2 * da);
      GLfloat v2 = r1 * sin(angle + 3 * da) - r2 * sin(angle + 2 * da);
      GLfloat len2 = sqrt(u2 * u2 + v2 * v2);
      u2 /= len2;
      v2 /= len2;

      /* draw front face */
      buffer.push_back({ r0 * cos(angle), r0 * sin(angle), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle), r1 * sin(angle), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle), r0 * sin(angle), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle), r0 * sin(angle), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });

      buffer.push_back({ r0 * cos(angle), r0 * sin(angle), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle2), r1 * sin(angle2), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle), r0 * sin(angle), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle2), r1 * sin(angle2), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle2), r0 * sin(angle2), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });

      /* draw front sides of teeth */
      buffer.push_back({ r1 * cos(angle), r1 * sin(angle), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle), r1 * sin(angle), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5f, 0.0, 0.0, 1.0, red, green, blue });

      /* draw back face */
      buffer.push_back({ r1 * cos(angle), r1 * sin(angle), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle), r0 * sin(angle), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle), r0 * sin(angle), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle), r1 * sin(angle), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle), r0 * sin(angle), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });

      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle), r0 * sin(angle), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle2), r0 * sin(angle2), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle2), r0 * sin(angle2), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle2), r1 * sin(angle2), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });

      /* draw back sides of teeth */
      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle), r1 * sin(angle), -width * 0.5f, 0.0, 0.0, -1.0, red, green, blue });

      /* draw outward faces of teeth */
      buffer.push_back({ r1 * cos(angle), r1 * sin(angle), width * 0.5f, v, -u, 0.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle), r1 * sin(angle), -width * 0.5f, v, -u, 0.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5f, v, -u, 0.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle), r1 * sin(angle), width * 0.5f, v, -u, 0.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5f, v, -u, 0.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5f, v, -u, 0.0, red, green, blue });

      buffer.push_back({ r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5f, cos(angle), sin(angle), 0.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5f, cos(angle), sin(angle), 0.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), -width * 0.5f, cos(angle), sin(angle), 0.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5f, cos(angle), sin(angle), 0.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), -width * 0.5f, cos(angle), sin(angle), 0.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5f, cos(angle), sin(angle), 0.0, red, green, blue });

      buffer.push_back({ r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5f, v2, -u2, 0.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), -width * 0.5f, v2, -u2, 0.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5f, v2, -u2, 0.0, red, green, blue });
      buffer.push_back({ r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5f, v2, -u2, 0.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5f, v2, -u2, 0.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5f, v2, -u2, 0.0, red, green, blue });

      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5f, cos(angle2), sin(angle2), 0.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5f, cos(angle2), sin(angle2), 0.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle2), r1 * sin(angle2), -width * 0.5f, cos(angle2), sin(angle2), 0.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5f, cos(angle2), sin(angle2), 0.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle2), r1 * sin(angle2), -width * 0.5f, cos(angle2), sin(angle2), 0.0, red, green, blue });
      buffer.push_back({ r1 * cos(angle2), r1 * sin(angle2), width * 0.5f, cos(angle2), sin(angle2), 0.0, red, green, blue });

      /* draw inside radius cylinder */
      buffer.push_back({ r0 * cos(angle), r0 * sin(angle), -width * 0.5f, -cos(angle), -sin(angle), 0.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle), r0 * sin(angle), width * 0.5f, -cos(angle2), -sin(angle2), 0.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle2), r0 * sin(angle2), width * 0.5f, -cos(angle), -sin(angle), 0.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle), r0 * sin(angle), -width * 0.5f, -cos(angle2), -sin(angle2), 0.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle2), r0 * sin(angle2), width * 0.5f, -cos(angle2), -sin(angle2), 0.0, red, green, blue });
      buffer.push_back({ r0 * cos(angle2), r0 * sin(angle2), -width * 0.5f, -cos(angle), -sin(angle), 0.0, red, green, blue });
   }

   GLuint vao;
   glGenVertexArrays(1, &vao);
   glBindVertexArray(vao);   

   GLuint vbo;
   glGenBuffers(1, &vbo);
   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * buffer.size(), buffer.data(), GL_STATIC_DRAW);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)12);
   glEnableVertexAttribArray(2);
   glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)24);

   return vao;
}


static void
draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glRotatef(view_rotx, 1.0, 0.0, 0.0);
   glRotatef(view_roty, 0.0, 1.0, 0.0);
   glRotatef(view_rotz, 0.0, 0.0, 1.0);

   glPushMatrix();
   glTranslatef(-3.0, -2.0, 0.0);
   glRotatef(angle, 0.0, 0.0, 1.0);
   glBindVertexArray(gear1);
   glDrawArrays(GL_TRIANGLES, 0, 1320);
   glPopMatrix();

   glPushMatrix();
   glTranslatef(3.1, -2.0, 0.0);
   glRotatef(-2.0 * angle - 9.0, 0.0, 0.0, 1.0);
   glBindVertexArray(gear2);
   glDrawArrays(GL_TRIANGLES, 0, 660);
   glPopMatrix();

   glPushMatrix();
   glTranslatef(-3.1, 4.2, 0.0);
   glRotatef(-2.0 * angle - 25.0, 0.0, 0.0, 1.0);
   glBindVertexArray(gear3);
   glDrawArrays(GL_TRIANGLES, 0, 660);
   glPopMatrix();

   glPopMatrix();
}

static void
draw_gears(void)
{
   if (stereo) {
      /* First left eye.  */
      glDrawBuffer(GL_BACK_LEFT);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glFrustum(left, right, -asp, asp, 5.0, 60.0);

      glMatrixMode(GL_MODELVIEW);

      glLoadIdentity();
      glTranslatef(0.0, 0.0, -40.0);
      glTranslated(+0.5 * eyesep, 0.0, 0.0);
      draw();

      /* Then right eye.  */
      glDrawBuffer(GL_BACK_RIGHT);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glFrustum(-right, -left, -asp, asp, 5.0, 60.0);

      glMatrixMode(GL_MODELVIEW);

      glLoadIdentity();
      glTranslatef(0.0, 0.0, -40.0);
      glTranslated(-0.5 * eyesep, 0.0, 0.0);
      draw();
   }
   else {
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glFrustum(-1.0, 1.0, -asp, asp, 5.0, 60.0);

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glTranslatef(0.0, 0.0, -40.0);
      draw();
   }
}


/** Draw single frame, do SwapBuffers, compute FPS */
static void
draw_frame(Display *dpy, Window win)
{
   static int frames = 0;
   static double tRot0 = -1.0, tRate0 = -1.0;
   double dt, t = current_time();

   if (tRot0 < 0.0)
      tRot0 = t;
   dt = t - tRot0;
   tRot0 = t;

   if (animate) {
      /* advance rotation for next frame */
      angle += 70.0 * dt;  /* 70 degrees per second */
      if (angle > 3600.0)
         angle -= 3600.0;
   }

   draw_gears();
   glXSwapBuffers(dpy, win);

   frames++;
   
   if (tRate0 < 0.0)
      tRate0 = t;
   if (t - tRate0 >= 5.0) {
      GLfloat seconds = t - tRate0;
      GLfloat fps = frames / seconds;
      printf("%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds,
             fps);
      fflush(stdout);
      tRate0 = t;
      frames = 0;
   }
}


/* new window size or exposure */
static void
reshape(int width, int height)
{
   GLfloat w = fix_point * (1.0 / 5.0);
   glViewport(0, 0, (GLint) width, (GLint) height);

   asp = (GLfloat) height / (GLfloat) width;
   left = -5.0 * ((w - 0.5 * eyesep) / fix_point);
   right = 5.0 * ((w + 0.5 * eyesep) / fix_point);
}
   

static const char vertexShader[] =
"#version 330 core\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"layout(location = 0) in vec3 position;\n"
"layout(location = 1) in vec3 normal;\n"
"layout(location = 2) in vec3 color;\n"
"uniform mat4 mvp;\n"
"layout(location = 0) out vec4 vs_position;\n"
"layout(location = 1) out vec3 vs_normal;\n"
"layout(location = 2) out vec3 vs_color;\n"
"void main(){\n"
"  vs_position = vec4(position, 1);\n"
"  gl_Position = mvp * vec4(position, 1);\n"
"  vs_normal = mat3(mvp) * normal;\n"
"  vs_color = color;\n"
"}\n"
;

static const char fragmentShader[] =
"#version 330 core\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"layout(location = 0) in vec4 vs_position;\n"
"layout(location = 1) in vec3 vs_normal;\n"
"layout(location = 2) in vec3 vs_color;\n"
"out vec4 color;\n"
"void main(){\n"
"  color = vec4(vs_color, 1.0);\n"
"}\n"
;

static void checkShaderError(GLuint shader) {
	int InfoLogLength;  
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(shader, InfoLogLength, NULL, VertexShaderErrorMessage.data());
		printf("%s\n", VertexShaderErrorMessage.data());
	}
}

static void
init(void)
{
   glEnable(GL_CULL_FACE);
   glEnable(GL_DEPTH_TEST);

   /* make the gears */
   gear1 = gear(1.0, 4.0, 1.0, 20, 0.7, 0.8, 0.1, 0.0);
   gear2 = gear(0.5, 2.0, 2.0, 10, 0.7, 0.0, 0.8, 0.2);
   gear3 = gear(1.3, 2.0, 0.5, 10, 0.7, 0.2, 0.2, 1.0);

   GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
   const char* fragmentShaderSource = fragmentShader;
   glShaderSource(fs, 1, &fragmentShaderSource, NULL);
   glCompileShader(fs);

   checkShaderError(fs);

   const char* vertexShaderSource = vertexShader;
   GLuint vs = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vs, 1, &vertexShaderSource, NULL);
   glCompileShader(vs);

   checkShaderError(vs);

   shaderProgram = glCreateProgram();
   glAttachShader(shaderProgram, vs);
   glAttachShader(shaderProgram, fs);
   glLinkProgram(shaderProgram);

   static GLfloat pos[4] = { 5.0, 5.0, 10.0, 0.0 };
   glLightfv(GL_LIGHT0, GL_POSITION, pos);

   glUseProgram(shaderProgram);

   float matrix[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
   glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, false, matrix);
}

/**
 * Remove window border/decorations.
 */
static void
no_border( Display *dpy, Window w)
{
   static const unsigned MWM_HINTS_DECORATIONS = (1 << 1);
   static const int PROP_MOTIF_WM_HINTS_ELEMENTS = 5;

   typedef struct
   {
      unsigned long       flags;
      unsigned long       functions;
      unsigned long       decorations;
      long                inputMode;
      unsigned long       status;
   } PropMotifWmHints;

   PropMotifWmHints motif_hints;
   Atom prop, proptype;
   unsigned long flags = 0;

   /* setup the property */
   motif_hints.flags = MWM_HINTS_DECORATIONS;
   motif_hints.decorations = flags;

   /* get the atom for the property */
   prop = XInternAtom( dpy, "_MOTIF_WM_HINTS", True );
   if (!prop) {
      /* something went wrong! */
      return;
   }

   /* not sure this is correct, seems to work, XA_WM_HINTS didn't work */
   proptype = prop;

   XChangeProperty( dpy, w,                         /* display, window */
                    prop, proptype,                 /* property, type */
                    32,                             /* format: 32-bit datums */
                    PropModeReplace,                /* mode */
                    (unsigned char *) &motif_hints, /* data */
                    PROP_MOTIF_WM_HINTS_ELEMENTS    /* nelements */
                  );
}


/*
 * Create an RGB, double-buffered window.
 * Return the window and context handles.
 */
static void
make_window( Display *dpy, const char *name,
             int x, int y, int width, int height,
             Window *winRet, GLXContext *ctxRet, VisualID *visRet)
{
   int attribs[64];
   int i = 0;

   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   GLXContext ctx;
   XVisualInfo *visinfo;

   /* Singleton attributes. */
   attribs[i++] = GLX_RGBA;
   attribs[i++] = GLX_DOUBLEBUFFER;
   if (stereo)
      attribs[i++] = GLX_STEREO;

   /* Key/value attributes. */
   attribs[i++] = GLX_RED_SIZE;
   attribs[i++] = 1;
   attribs[i++] = GLX_GREEN_SIZE;
   attribs[i++] = 1;
   attribs[i++] = GLX_BLUE_SIZE;
   attribs[i++] = 1;
   attribs[i++] = GLX_DEPTH_SIZE;
   attribs[i++] = 1;
   if (samples > 0) {
      attribs[i++] = GLX_SAMPLE_BUFFERS;
      attribs[i++] = 1;
      attribs[i++] = GLX_SAMPLES;
      attribs[i++] = samples;
   }

   attribs[i++] = None;

   scrnum = DefaultScreen( dpy );
   root = RootWindow( dpy, scrnum );

   visinfo = glXChooseVisual(dpy, scrnum, attribs);
   if (!visinfo) {
      printf("Error: couldn't get an RGB, Double-buffered");
      if (stereo)
         printf(", Stereo");
      if (samples > 0)
         printf(", Multisample");
      printf(" visual\n");
      exit(1);
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   /* XXX this is a bad way to get a borderless window! */
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( dpy, root, x, y, width, height,
                        0, visinfo->depth, InputOutput,
                        visinfo->visual, mask, &attr );

   if (fullscreen)
      no_border(dpy, win);

   /* set hints and properties */
   {
      XSizeHints sizehints;
      sizehints.x = x;
      sizehints.y = y;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(dpy, win, &sizehints);
      XSetStandardProperties(dpy, win, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }

   ctx = glXCreateContext( dpy, visinfo, NULL, True );
   if (!ctx) {
      printf("Error: glXCreateContext failed\n");
      exit(1);
   }

   *winRet = win;
   *ctxRet = ctx;
   *visRet = visinfo->visualid;

   XFree(visinfo);
}


/**
 * Determine whether or not a GLX extension is supported.
 */
static int
is_glx_extension_supported(Display *dpy, const char *query)
{
   const int scrnum = DefaultScreen(dpy);
   const char *glx_extensions = NULL;
   const size_t len = strlen(query);
   const char *ptr;

   if (glx_extensions == NULL) {
      glx_extensions = glXQueryExtensionsString(dpy, scrnum);
   }

   ptr = strstr(glx_extensions, query);
   return ((ptr != NULL) && ((ptr[len] == ' ') || (ptr[len] == '\0')));
}


/**
 * Attempt to determine whether or not the display is synched to vblank.
 */
static void
query_vsync(Display *dpy, GLXDrawable drawable)
{
   int interval = 0;

#if defined(GLX_EXT_swap_control)
   if (is_glx_extension_supported(dpy, "GLX_EXT_swap_control")) {
       unsigned int tmp = -1;
       glXQueryDrawable(dpy, drawable, GLX_SWAP_INTERVAL_EXT, &tmp);
       interval = tmp;
   } else
#endif
   if (is_glx_extension_supported(dpy, "GLX_MESA_swap_control")) {
      PFNGLXGETSWAPINTERVALMESAPROC pglXGetSwapIntervalMESA =
          (PFNGLXGETSWAPINTERVALMESAPROC)
          glXGetProcAddressARB((const GLubyte *) "glXGetSwapIntervalMESA");

      interval = (*pglXGetSwapIntervalMESA)();
   } else if (is_glx_extension_supported(dpy, "GLX_SGI_swap_control")) {
      /* The default swap interval with this extension is 1.  Assume that it
       * is set to the default.
       *
       * Many Mesa-based drivers default to 0, but all of these drivers also
       * export GLX_MESA_swap_control.  In that case, this branch will never
       * be taken, and the correct result should be reported.
       */
      interval = 1;
   }


   if (interval > 0) {
      printf("Running synchronized to the vertical refresh.  The framerate should be\n");
      if (interval == 1) {
         printf("approximately the same as the monitor refresh rate.\n");
      } else if (interval > 1) {
         printf("approximately 1/%d the monitor refresh rate.\n",
                interval);
      }
   }
}

/**
 * Handle one X event.
 * \return NOP, EXIT or DRAW
 */
static int
handle_event(Display *dpy, Window win, XEvent *event)
{
   (void) dpy;
   (void) win;

   switch (event->type) {
   case Expose:
      return DRAW;
   case ConfigureNotify:
      reshape(event->xconfigure.width, event->xconfigure.height);
      break;
   case KeyPress:
      {
         char buffer[10];
         int code;
         code = XLookupKeysym(&event->xkey, 0);
         if (code == XK_Left) {
            view_roty += 5.0;
         }
         else if (code == XK_Right) {
            view_roty -= 5.0;
         }
         else if (code == XK_Up) {
            view_rotx += 5.0;
         }
         else if (code == XK_Down) {
            view_rotx -= 5.0;
         }
         else {
            XLookupString(&event->xkey, buffer, sizeof(buffer),
                          NULL, NULL);
            if (buffer[0] == 27) {
               /* escape */
               return EXIT;
            }
            else if (buffer[0] == 'a' || buffer[0] == 'A') {
               animate = !animate;
            }
         }
         return DRAW;
      }
   }
   return NOP;
}


static void
event_loop(Display *dpy, Window win)
{
   while (1) {
      int op;
      while (!animate || XPending(dpy) > 0) {
         XEvent event;
         XNextEvent(dpy, &event);
         op = handle_event(dpy, win, &event);
         if (op == EXIT)
            return;
         else if (op == DRAW)
            break;
      }

      draw_frame(dpy, win);
   }
}


static void
usage(void)
{
   printf("Usage:\n");
   printf("  -display <displayname>  set the display to run on\n");
   printf("  -stereo                 run in stereo mode\n");
   printf("  -samples N              run in multisample mode with at least N samples\n");
   printf("  -fullscreen             run in fullscreen mode\n");
   printf("  -info                   display OpenGL renderer info\n");
   printf("  -geometry WxH+X+Y       window geometry\n");
}
 

int
main(int argc, char *argv[])
{
   unsigned int winWidth = 300, winHeight = 300;
   int x = 0, y = 0;
   Display *dpy;
   Window win;
   GLXContext ctx;
   char *dpyName = NULL;
   GLboolean printInfo = GL_FALSE;
   VisualID visId;
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-display") == 0) {
         dpyName = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "-info") == 0) {
         printInfo = GL_TRUE;
      }
      else if (strcmp(argv[i], "-stereo") == 0) {
         stereo = GL_TRUE;
      }
      else if (i < argc-1 && strcmp(argv[i], "-samples") == 0) {
         samples = strtod(argv[i+1], NULL );
         ++i;
      }
      else if (strcmp(argv[i], "-fullscreen") == 0) {
         fullscreen = GL_TRUE;
      }
      else if (i < argc-1 && strcmp(argv[i], "-geometry") == 0) {
         XParseGeometry(argv[i+1], &x, &y, &winWidth, &winHeight);
         i++;
      }
      else {
         usage();
         return -1;
      }
   }

   dpy = XOpenDisplay(dpyName);
   if (!dpy) {
      printf("Error: couldn't open display %s\n",
             dpyName ? dpyName : getenv("DISPLAY"));
      return -1;
   }

   if (fullscreen) {
      int scrnum = DefaultScreen(dpy);

      x = 0; y = 0;
      winWidth = DisplayWidth(dpy, scrnum);
      winHeight = DisplayHeight(dpy, scrnum);
   }

   make_window(dpy, "glxgears", x, y, winWidth, winHeight, &win, &ctx, &visId);
   XMapWindow(dpy, win);
   glXMakeCurrent(dpy, win, ctx);
   query_vsync(dpy, win);

   glewInit();
   if (printInfo) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
      printf("VisualID %d, 0x%x\n", (int) visId, (int) visId);
   }

   init();

   /* Set initial projection/viewing transformation.
    * We can't be sure we'll get a ConfigureNotify event when the window
    * first appears.
    */
   reshape(winWidth, winHeight);

   event_loop(dpy, win);

   glDeleteLists(gear1, 1);
   glDeleteLists(gear2, 1);
   glDeleteLists(gear3, 1);
   glXMakeCurrent(dpy, None, NULL);
   glXDestroyContext(dpy, ctx);
   XDestroyWindow(dpy, win);
   XCloseDisplay(dpy);

   return 0;
}
