// ========================================================================================================================== //
// *** This Project is Developed Under the course CSE400 at Shahjalal University.
// *** Project Author: Aqib Ashef and S.M. Ariful Islam
// *** ThesisProject.cpp : Defines the entry point for the application.
// ==========================================================================================================================

#include "stdafx.h"
#include "ThesisProject.h"
#include "queue"


#define MAX_LOADSTRING 100

#define make_pair(a,b) pair<int,int>(a,b)

// ================================= Variable Region : All the global variables will be declared here with categorization =========================== //

TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];
HDC hDC = NULL;    // Private GDI Device Context. GDI : http://en.wikipedia.org/wiki/Graphics_Device_Interface
HWND hWnd = NULL; // holds our window handle. 
HINSTANCE hInstance; // holds the instance of the application
HGLRC hRC = NULL; // Permanent Rendering Context

bool keys[256]; // Array Used for the keyboard routine. this will help us to track multiple key press, like : "Ctrl + s"
bool active = TRUE; // window active flag. set to true by default. 
bool fullscreen = FALSE; // fullscreen flag. set to true by default. indicates whether the screen should be windowed or fullscreen.

int screenWidth;          // screen width.
int screenHeight;          // screen height.

GLfloat rotationTriangle;  // rotation for the triangle.
GLfloat rotationQuad;      // rotation for the cube.

GLuint texture[1];
TextureHelper textureHelper;

VideoCapture videoCapture;

int dirX[8] = { 0, 1, 0, -1 , 1 , -1 , 1 , -1 };
int dirY[8] = { 1, 0, -1, 0 , 1 , -1 , -1 , 1 };


// ================================================================================================================================================== //

// ================================ Function prototype declaration region : all the function prototype declaration will go here. ====================== //

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); // a call back function to process window action.

int InitGL(GLvoid); // this function is used for setting up opengl components.
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag); // creates the window.
int DrawGLScene(GLvoid); // this is where all the drawing goes.
GLvoid ReSizeGLScene(GLsizei width, GLsizei height); // this function is used to handle the properties when the window is resized when full screen mode is not used.
GLvoid KillGLWindow(GLvoid); // this function is called before closing the window.

void doSomething(void); // Does Something
vector <vector<pair<int, int> > > getShapes(Mat shapeHolder);// Gets shapes

// =================================================================================================================================================== //

// this function is the entry point of this project . what ever this program does, it starts from here.
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	//printf("Here");
	doSomething();
	/*try{
		videoCapture.open(0);
		if (videoCapture.isOpened()){
			
			videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 400.0f);
			videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 600.0f);
			
			
		}

	}
	catch (Exception e){
		return -1;
	}*/
	


	MSG msg;                                // Windows Message Structure
	BOOL    done = FALSE;                         // Bool Variable To Exit Loop

	screenHeight = GetSystemMetrics(SM_CYSCREEN);  // gets the height of the screen.
	screenWidth = GetSystemMetrics(SM_CXSCREEN);   // gets the width of the screen.

	// Create Our OpenGL Window
	if (!CreateGLWindow("Thesis Project", screenWidth, screenHeight, 16, fullscreen))
	{
		return 0;                           // Quit If Window Was Not Created
	}

	while (!done)                                // Loop That Runs Until done=TRUE
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))           // Is There A Message Waiting?
		{
			if (msg.message == WM_QUIT)               // Have We Received A Quit Message?
			{
				done = TRUE;                  // If So done=TRUE
			}
			else                            // If Not, Deal With Window Messages
			{
				TranslateMessage(&msg);             // Translate The Message
				DispatchMessage(&msg);              // Dispatch The Message
			}
		}
		else                                // If There Are No Messages
		{
			// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
			if (active)                     // Program Active?
			{
				if (keys[VK_ESCAPE])                // Was ESC Pressed?
				{
					done = TRUE;              // ESC Signalled A Quit
				}
				else                        // Not Time To Quit, Update Screen
				{
					DrawGLScene();              // Draw The Scene
					SwapBuffers(hDC);           // Swap Buffers (Double Buffering)
				}
			}
			if (keys[VK_F1])                    // Is F1 Being Pressed?
			{
				keys[VK_F1] = FALSE;              // If So Make Key FALSE
				KillGLWindow();                 // Kill Our Current Window
				fullscreen = !fullscreen;             // Toggle Fullscreen / Windowed Mode
				// Recreate Our OpenGL Window
				if (!CreateGLWindow("Thesis Project", screenWidth, screenHeight, 16, fullscreen))
				{
					return 0;               // Quit If Window Was Not Created
				}
			}
		}
	}
	// Shutdown
	
	KillGLWindow();                             // Kill The Window
	return (msg.wParam);                            // Exit The Program
}

// this function is used to initialize all the opengl components.
int InitGL(GLvoid){

	glViewport(0, 0, screenWidth, screenHeight); // Resize the current viewport
	glMatrixMode(GL_PROJECTION);     // select the projection matrix
	glLoadIdentity();                // reset the projection matrix

	// Calculate theaspect ratio of the window
	gluPerspective(45.0f, (GLfloat)screenWidth / (GLfloat)screenHeight, 0.1f, 100.0f);

	glMatrixMode(GL_MODELVIEW);       // select the model view matrix
	glLoadIdentity();                 // reset the modelview matrix

	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);               // enabling smooth shader
	glClearColor(0.0f, 0.0f, 0.0f, 0.1f);  // black background

	glClearDepth(1.0f);                    // depth buffer setup
	glEnable(GL_DEPTH_TEST);               // enable depth testing
	glDepthFunc(GL_LEQUAL);                // the type of depth test to do

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);     // really nice perspective calculations
	return TRUE;                           // initialization went ok
}

// this function creates and registers the window to the OS.
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag){


	GLuint      PixelFormat;                        // Holds The Results After Searching For A Match

	WNDCLASS    wc;                         // Windows Class Structure

	DWORD       dwExStyle;                      // Window Extended Style
	DWORD       dwStyle;                        // Window Style

	RECT WindowRect;                            // Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left = (long)0;                        // Set Left Value To 0
	WindowRect.right = (long)width;                       // Set Right Value To Requested Width
	WindowRect.top = (long)0;                         // Set Top Value To 0
	WindowRect.bottom = (long)height;                     // Set Bottom Value To Requested Height

	fullscreen = fullscreenflag;                      // Set The Global Fullscreen Flag

	hInstance = GetModuleHandle(NULL);            // Grab An Instance For Our Window
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;       // Redraw On Move, And Own DC For Window
	wc.lpfnWndProc = (WNDPROC)WndProc;                // WndProc Handles Messages
	wc.cbClsExtra = 0;                        // No Extra Window Data
	wc.cbWndExtra = 0;                        // No Extra Window Data
	wc.hInstance = hInstance;                    // Set The Instance
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);          // Load The Default Icon
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);          // Load The Arrow Pointer
	wc.hbrBackground = NULL;                     // No Background Required For GL
	wc.lpszMenuName = NULL;                     // We Don't Want A Menu
	wc.lpszClassName = _TEXT("ThesisProject");                 // Set The Class Name

	if (!RegisterClass(&wc))                        // Attempt To Register The Window Class
	{
		MessageBox(NULL, _TEXT("Failed To Register The Window Class."), _TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;                           // Exit And Return FALSE
	}

	if (fullscreen){                             // Attempt Fullscreen Mode?
		DEVMODE dmScreenSettings;                   // Device Mode
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));       // Makes Sure Memory's Cleared
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);       // Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth = width;            // Selected Screen Width
		dmScreenSettings.dmPelsHeight = height;           // Selected Screen Height
		dmScreenSettings.dmBitsPerPel = bits;             // Selected Bits Per Pixel
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL){
			// If The Mode Fails, Offer Two Options.  Quit Or Run In A Window.
			fullscreen = FALSE;
		}
	}

	if (fullscreen){                             // Are We Still In Fullscreen Mode?
		dwExStyle = WS_EX_APPWINDOW;                  // Window Extended Style
		dwStyle = WS_POPUP;                       // Windows Style
		ShowCursor(TRUE);                      // Hide Mouse Pointer
	}
	else{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;           // Window Extended Style
		dwStyle = WS_OVERLAPPEDWINDOW;                    // Windows Style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);     // Adjust Window To True Requested Size

	if (!(hWnd = CreateWindowEx(dwExStyle,              // Extended Style For The Window
		_TEXT("ThesisProject"),               // Class Name
		_TEXT("Thesis Project"),                  // Window Title
		WS_CLIPSIBLINGS |           // Required Window Style
		WS_CLIPCHILDREN |           // Required Window Style
		dwStyle,                // Selected Window Style
		0, 0,                   // Window Position
		WindowRect.right - WindowRect.left,   // Calculate Adjusted Window Width
		WindowRect.bottom - WindowRect.top,   // Calculate Adjusted Window Height
		NULL,                   // No Parent Window
		NULL,                   // No Menu
		hInstance,              // Instance
		NULL)))
	{
		KillGLWindow();                         // Reset The Display
		MessageBox(NULL, _TEXT("Window Creation Error."), _TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	static  PIXELFORMATDESCRIPTOR pfd =                  // pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),                  // Size Of This Pixel Format Descriptor
		1,                              // Version Number
		PFD_DRAW_TO_WINDOW |                        // Format Must Support Window
		PFD_SUPPORT_OPENGL |                        // Format Must Support OpenGL
		PFD_DOUBLEBUFFER,                       // Must Support Double Buffering
		PFD_TYPE_RGBA,                          // Request An RGBA Format
		bits,                               // Select Our Color Depth
		0, 0, 0, 0, 0, 0,                       // Color Bits Ignored
		0,                              // No Alpha Buffer
		0,                              // Shift Bit Ignored
		0,                              // No Accumulation Buffer
		0, 0, 0, 0,                         // Accumulation Bits Ignored
		16,                             // 16Bit Z-Buffer (Depth Buffer)
		0,                              // No Stencil Buffer
		0,                              // No Auxiliary Buffer
		PFD_MAIN_PLANE,                         // Main Drawing Layer
		0,                              // Reserved
		0, 0, 0                             // Layer Masks Ignored
	};

	if (!(hDC = GetDC(hWnd)))                         // Did We Get A Device Context?
	{
		KillGLWindow();                         // Reset The Display
		MessageBox(NULL, _TEXT("Can't Create A GL Device Context."), _TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;                           // Return FALSE
	}

	if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd)))             // Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();                         // Reset The Display
		MessageBox(NULL, _TEXT("Can't Find A Suitable PixelFormat."), _TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;                           // Return FALSE
	}

	if (!SetPixelFormat(hDC, PixelFormat, &pfd))               // Are We Able To Set The Pixel Format?
	{
		KillGLWindow();                         // Reset The Display
		MessageBox(NULL, _TEXT("Can't Set The PixelFormat."), _TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;                           // Return FALSE
	}

	if (!(hRC = wglCreateContext(hDC)))                   // Are We Able To Get A Rendering Context?
	{
		KillGLWindow();                         // Reset The Display
		MessageBox(NULL, _TEXT("Can't Create A GL Rendering Context."), _TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;                           // Return FALSE
	}

	if (!wglMakeCurrent(hDC, hRC))                        // Try To Activate The Rendering Context
	{
		KillGLWindow();                         // Reset The Display
		MessageBox(NULL, _TEXT("Can't Activate The GL Rendering Context."), _TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;                           // Return FALSE
	}

	ShowWindow(hWnd, SW_SHOW);                       // Show The Window
	SetForegroundWindow(hWnd);                      // Slightly Higher Priority
	SetFocus(hWnd);                             // Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);                       // Set Up Our Perspective GL Screen

	if (!InitGL())                              // Initialize Our Newly Created GL Window
	{
		KillGLWindow();                         // Reset The Display
		MessageBox(NULL, _TEXT("Initialization Failed."), _TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;                           // Return FALSE
	}

	return TRUE;                                // Success
}


// this function is used to draw objects in the scene.
int DrawGLScene(GLvoid){

	/*if (!videoCapture.isOpened())
	{
		return FALSE;
	}
	else 
	{
		try{
			double fps = videoCapture.get(CV_CAP_PROP_FPS);
			cout << fps << endl;
			Mat videoFrame;
			Mat showFrame;
			bool isFrameRead = videoCapture.read(videoFrame);
			if (!isFrameRead)
			{
				return FALSE;
			}
			else
			{
				flip(videoFrame, videoFrame, 1);
				cvtColor(videoFrame, showFrame, CV_RGB2GRAY);
				
				Canny(showFrame, showFrame, 1, 2);
				
				double fps = videoCapture.get(CV_CAP_PROP_FPS);
				char text[255];
				sprintf_s(text, "Frame Rate: %lf", fps);
				putText(videoFrame, text, cvPoint(200, 400), CV_FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255, 0), 1, 8, false);
				
				imshow("Camera", videoFrame);				
				flip(videoFrame, videoFrame, 0);

				imshow("Show", showFrame);
				
				cvtColor(videoFrame, videoFrame, CV_BGR2RGB);
				textureHelper.bind(videoFrame);				
			}

			if (!videoFrame.empty())
			{
				videoFrame.release();
			}
			if (!showFrame.empty())
			{
				showFrame.release();
			}
			
		}
		catch (Exception e){
			return FALSE;
		}
	}*/


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);         // Clear The Screen And The Depth Buffer
	glLoadIdentity();
	glTranslatef(-0.5f, -0.5f, -7.0f);                   // Move Right 3 Units
	
	
	glFlush();
	return TRUE;                                 // Everything Went OK
}

// this function resizes and reinitializes the property of the opengl window when window is resized.
GLvoid ReSizeGLScene(GLsizei width, GLsizei height){
	if (height == 0){              // prevent a divide by zero
		height = 1;                // Making height equal one
	}

	glViewport(0, 0, width, height); // Resize the current viewport
	glMatrixMode(GL_PROJECTION);     // select the projection matrix
	glLoadIdentity();                // reset the projection matrix

	// Calculate theaspect ratio of the window
	gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);

	glMatrixMode(GL_MODELVIEW);       // select the model view matrix
	glLoadIdentity();                 // reset the modelview matrix
}
// this function is called before closing the application to properly close the window.
GLvoid KillGLWindow(GLvoid)
{
	if (videoCapture.isOpened())
	{
		videoCapture.release();
	}

	if (fullscreen)
	{
		ChangeDisplaySettings(NULL, 0); // If so Switch back to the Desktop.
		ShowCursor(TRUE); // Show the mouse pointer.
	}
	if (hRC)
	{
		if (!wglMakeCurrent(NULL, NULL))
		{
			MessageBox(NULL, _TEXT("Release of DC and RC Failed"), _TEXT("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
		}
		if (!wglDeleteContext(hRC))   // Are we able to delete the rendering context?
		{
			MessageBox(NULL, _TEXT("Release of Rendering Context Failed"), _TEXT("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
		}
		hRC = NULL; // set rendering context to null.
	}

	if (hDC && !ReleaseDC(hWnd, hDC))                    // Are We Able To Release The DC
	{
		MessageBox(NULL, _TEXT("Release Device Context Failed."), _TEXT("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
		hDC = NULL;                           // Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))                   // Are We Able To Destroy The Window?
	{
		MessageBox(NULL, _TEXT("Could Not Release hWnd."), _TEXT("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
		hWnd = NULL;                          // Set hWnd To NULL
	}

	if (!UnregisterClass(_TEXT("ThesisProject"), hInstance))               // Are We Able To Unregister Class
	{
		MessageBox(NULL, _TEXT("Could Not Unregister Class."), _TEXT("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
		hInstance = NULL;                         // Set hInstance To NULL
	}
}

LRESULT CALLBACK WndProc(HWND    hWnd,                   // Handle For This Window
	UINT    uMsg,                   // Message For This Window
	WPARAM  wParam,                 // Additional Message Information
	LPARAM  lParam)                 // Additional Message Information
{
	switch (uMsg)                               // Check For Windows Messages
	{
	case WM_ACTIVATE:                       // Watch For Window Activate Message
	{
		if (!HIWORD(wParam))                    // Check Minimization State
		{
			active = TRUE;                    // Program Is Active
		}
		else
		{
			active = FALSE;                   // Program Is No Longer Active
		}

		return 0;                       // Return To The Message Loop
	}
	case WM_SYSCOMMAND:                     // Intercept System Commands
	{
		switch (wParam)                     // Check System Calls
		{
		case SC_SCREENSAVE:             // Screensaver Trying To Start?
		case SC_MONITORPOWER:               // Monitor Trying To Enter Powersave?
			return 0;                   // Prevent From Happening
		}
		break;                          // Exit
	}
	case WM_CLOSE:                          // Did We Receive A Close Message?
	{
		PostQuitMessage(0);                 // Send A Quit Message
		return 0;                       // Jump Back
	}
	case WM_KEYDOWN:                        // Is A Key Being Held Down?
	{
		keys[wParam] = TRUE;                    // If So, Mark It As TRUE
		return 0;                       // Jump Back
	}
	case WM_KEYUP:                          // Has A Key Been Released?
	{
		keys[wParam] = FALSE;                   // If So, Mark It As FALSE
		return 0;                       // Jump Back
	}
	case WM_SIZE:                           // Resize The OpenGL Window
	{
		ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));       // LoWord=Width, HiWord=Height
		return 0;                       // Jump Back
	}
	}
	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void doSomething(void)
{
	namedWindow("Input", CV_WINDOW_FULLSCREEN);
	namedWindow("Output", CV_WINDOW_FULLSCREEN);

	Mat image = imread("4.jpg", CV_LOAD_IMAGE_COLOR);;
	Mat output;
	Mat imageGrey;
	Mat shapeHolder;
	Mat subs;
	
	
	
	if (!image.data)                              // Check for invalid input
	{
		cout << "Could not open or find the image" << std::endl;
	}
	else
	{
		cout << "Found the image" << std::endl;
		
		cvtColor(image, imageGrey, CV_BGR2GRAY);
		threshold(imageGrey, imageGrey, 150.0, 255.0,THRESH_BINARY);
		Canny(imageGrey, output, 3, 150);
		// The smallest value between threshold1 and threshold2 is used for edge linking.
		// The largest value is used to find initial segments of strong edges
		

		dilate(output, shapeHolder ,Mat());
		
		//imshow("Output", imageGrey);

		subtract(shapeHolder, imageGrey, shapeHolder);

		imshow("Input", image);
		imshow("Output",shapeHolder);
		vector <vector<pair<int, int> > > foundShapes = getShapes(shapeHolder);
		
	}
}

vector < vector<pair<int, int> > > getShapes(Mat shapeHolder)
{
	vector<vector<pair<int, int> > > ret;
	vector<pair<int, int> > elem;
	ret.clear();

	Mat check = Mat::zeros(shapeHolder.rows, shapeHolder.cols,CV_8UC1);

	for (int i = 0; i < shapeHolder.rows; i++)
	{
		for (int j = 0; j < shapeHolder.cols; j++)
		{
			uchar val = shapeHolder.at<uchar>(i, j);
			uchar curFlag = check.at<uchar>(i, j);
			if (val == (uchar)0 || curFlag == (uchar)1 ) continue;
			
			elem.clear();

			queue<pair<int,int>> q;
			q.push(make_pair(i, j));

			while (!q.empty())
			{
				pair<int, int> cur = q.front();
				q.pop();
				if (check.at<uchar>(cur.first, cur.second) == (uchar)1)
					continue;
				check.at<uchar>(cur.first, cur.second) = (uchar)1;
				//elem.push_back(make_pair(cur.second,cur.first) );
				elem.push_back(cur);

				pair<int, int> nxt;

				for (int k = 0; k < 8; k++)
				{
					nxt.first = cur.first + dirX[k];
					nxt.second = cur.second + dirY[k];

					if (nxt.first < 0 || nxt.first >= shapeHolder.rows || nxt.second < 0 || nxt.second >= shapeHolder.cols)
						continue;
					if (check.at<uchar>(nxt.first, nxt.second) == (uchar)1)
						continue;
					if (shapeHolder.at<uchar>(nxt.first, nxt.second) == (uchar)0)
						continue;

					q.push(nxt);
				}

			}
			ret.push_back(elem);
		}
	}

	
	return ret;
}