/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef SWF_H
#define SWF_H

#include <iostream>
#include <fstream>
#include <list>
#include <semaphore.h>
#include "swftypes.h"
#include "frame.h"
#include "vm.h"
#include "asobjects.h"

#include <X11/Xlib.h>
#include <GL/glx.h>

class DisplayListTag;
class DictionaryTag;
class IActiveObject;
class ABCVm;

typedef void* (*thread_worker)(void*);

class SWF_HEADER
{
public:
	UI8 Signature[3];
	UI8 Version;
	UI32 FileLength;
	RECT FrameSize;
	UI16 FrameRate;
	UI16 FrameCount;
public:
	SWF_HEADER(std::istream& in);
	const RECT& getFrameSize(){ return FrameSize; }
};

class ExecutionContext;

struct fps_profiling
{
	uint64_t render_time;
	uint64_t action_time;
	uint64_t cache_time;
	uint64_t fps;
	fps_profiling():render_time(0),action_time(0),cache_time(0),fps(0){}
};

class SystemState:public ASMovieClip
{
private:
	RECT frame_size;

	//Semaphore to wait for new frames to be available
	sem_t new_frame;

	sem_t sem_dict;
	std::list < DictionaryTag* > dictionary;

	RGB Background;

	sem_t sem_run;

	bool update_request;

	sem_t mutex;

public:
	bool shutdown;
	int version;
	SWFObject getVariableByName(const std::string& name);
	void setVariableByName(const std::string& name, const SWFObject& o);
	ISWFObject* getParent() { return NULL; }
	void setShutdownFlag();

	bool performance_profiling;
	VirtualMachine vm;
	//Used only in ParseThread context
	std::list < IDisplayListElem* >* parsingDisplayList;
	ISWFObject* parsingTarget;

	//Used only in RenderThread context
	ASMovieClip* currentClip;
	ExecutionContext* execContext;

	SystemState();
	void waitToRun();
	Frame& getFrameAtFP();
	void advanceFP();
	void setFrameSize(const RECT& f);
	void setFrameCount(int f);
	RECT getFrameSize();
	void addToDictionary(DictionaryTag* r);
	void addToDisplayList(IDisplayListElem* r);
	void commitFrame();
	RGB getBackground();
	void setBackground(const RGB& bg);
	void setUpdateRequest(bool s);
	DictionaryTag* dictionaryLookup(UI16 id);
	void reset();
	void _register(){}
	SWFOBJECT_TYPE getObjectType();
	fps_profiling* fps_prof;
	ABCVm* currentVm;
};

class ParseThread
{
private:
	SystemState* m_sys;
	std::istream& f;
	pthread_t t;
	static void* worker(ParseThread*);
	static int error;
public:
	ParseThread(SystemState* s, std::istream& in);
	~ParseThread();
	void wait();
	static void setError(){error=1;}
};

enum ENGINE { SDL=0, NPAPI};
struct NPAPI_params
{
	Display* display;
	VisualID visual;
	Window window;
	int width;
	int height;
};


class InputThread
{
private:
	SystemState* m_sys;
	NPAPI_params* npapi_params;
	pthread_t t;
	static void* sdl_worker(InputThread*);
	static void* npapi_worker(InputThread*);
	std::list< IActiveObject* > listeners;
	sem_t sem_listeners;

public:
	InputThread(SystemState* s,ENGINE e, void* param=NULL);
	~InputThread();
	void wait();
	void addListener(IActiveObject* tag);
};

class RenderThread
{
private:
	SystemState* m_sys;
	NPAPI_params* npapi_params;
	pthread_t t;
	static void* sdl_worker(RenderThread*);
	static void* npapi_worker(RenderThread*);
	sem_t mutex;
	sem_t render;
	sem_t end_render;
	Frame* cur_frame;
	Frame bak_frame;
	int bak;
	static int error;

	GLXFBConfig mFBConfig;
	GLXContext mContext;
	GC mGC;
public:
	RenderThread(SystemState* s,ENGINE e, void* param=NULL);
	~RenderThread();
	void draw(Frame* f);
	void wait();
	static int setError(){error=1;}
};
#endif
