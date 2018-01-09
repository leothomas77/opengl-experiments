/**
 * Fast Single-pass A-Buffer using OpenGL 4.0
 * Copyright Cyril Crassin, June 2010
**/

#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include "GL/glew.h"
#include <GL/freeglut.h>

#include <string>
#include <iostream>

#define INFOWIDGET_GLSTRING 1 //Not working with >=GL3 contexts

class InfoWidget{
	std::string message;
	std::string stateMessage;

	int messageStartTime;
	int messageDuration;
public:
	void print(std::string msg, int duration=3000){
#if INFOWIDGET_GLSTRING
		message=msg;
		messageDuration=duration;
		messageStartTime=glutGet(GLUT_ELAPSED_TIME);
#else
		std::cout<<msg<<"\n";
#endif
	}

	void setStateMessage(std::string msg){
#if INFOWIDGET_GLSTRING
		stateMessage=msg;
#else
		std::cout<<msg<<"\n";
#endif
	}

	void draw(){
#if INFOWIDGET_GLSTRING
		if( (glutGet(GLUT_ELAPSED_TIME)-messageStartTime) < messageDuration){
			glColor4f(0.2f, 0.2f, 0.2f, 1.0f);
			glRasterPos2f(-0.95f, -0.95f);
			glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)message.c_str() );
		}		
		glColor4f(0.3f, 0.5f, 0.2f, 1.0f);
		glRasterPos2f(-0.95f, 0.90f);
		glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)stateMessage.c_str() );
#endif
	}
};
#endif