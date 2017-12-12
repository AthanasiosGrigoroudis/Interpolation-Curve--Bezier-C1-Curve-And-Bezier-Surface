// Curves.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream> /* for cout */
#include <GL\glut.h> /* declare always last for exit(0) error */

using namespace std;

// Set constants
const float PI = 3.1415;
const float WORLD_WIDTH = 100.0;
const float WORLD_HEIGHT = 100.0;
const int CAMERA_DISTANCE = 70; // The distance of camera from the center of the axis (at the surface mode)

const float BACKGROUND_COLOR[] = { 1.0, 1.0, 1.0 };
const float POINTS_COLOR[] = { 0.0, 0.0, 0.0 };
const float SURFACE_POINTS_COLOR[] = { 0.7, 0.0, 0.0 };
const float LINE_COLOR[] = { 1.0, 0.0, 0.0 };
const float C1_LINE_COLOR[] = { 0.8, 0.8, 0.8 };
const float SURFACE_LINE_COLOR[] = { 0.0, 0.0, 0.0 };

const int POINTS_SIZE = 6;
const int SURFACE_POINT_SIZE = 4;
const float MAX_POINTER_DISTANCE = 20.0;

const int CURVE_POINTS = 100;
const int SURFACE_LINES = 10;

// Polynomial to Bezier transformation matrix
float polyToBezier[4][4] = {
	{1, 0, 0, 0},
	{-0.833, 3, -1.5, 0.333},
	{0.333, -1.5, 3, -0.8333},
	{0, 0, 0, 1}
};

// Control points for every mode
float ctrlPointsMode1[4][3] = { { -40, 0, 0 },{ -10, -10, 0 },{ 15, 10, 0 }, {40, -5, 0} };
int ctrlPointsMode1Size = 4;
float ctrlPointsMode2[7][3] = { { 0, -10, 0 },{ -35, 0, 0 },{ -25, 20, 0 }, { 0, 40, 0 },{ 20, 20, 0 },{ 35, 0, 0}, {0, -10, 0} };
int ctrlPointsMode2Size = 7;
float ctrlPointsMode3[7][3] = { { -40, 0, 0 },{ -15, 20, 0 },{ -20, 35, 0 },{ 0, 35, 0 }, { 20, 35, 0 },{ 15, 20, 0 },{ 40, 0, 0 } };
int ctrlPointsMode3Size = 7;
float const EDGES_X = 0; // The x coordinate of the square that edges belong to (Y=Z, X=EDGES_X)
float const EDGES_YZ = 30; // The maximum y and z axis values of the square edges belong to (max Y = max Z = EDGES_YZ)
float const OTHER_X = 20; // The x coordinate of the square that the rest control points belong to
float ctrlPointsMode4[4][4][3] = {
	{ { EDGES_X, EDGES_YZ / 2, EDGES_YZ },{ OTHER_X, EDGES_YZ / 2, 5 },{ OTHER_X, EDGES_YZ / 2, -5 },{ EDGES_X, EDGES_YZ / 2, -EDGES_YZ / 2 } },
	{ { OTHER_X, 5, EDGES_YZ / 2 },{ OTHER_X, 5, 5 },{ OTHER_X, 5, -5 },{ OTHER_X, 5, -EDGES_YZ / 2 } },
	{ { OTHER_X, -5, EDGES_YZ / 2 },{ OTHER_X, -5, 5 },{ OTHER_X, -5, -5 },{ OTHER_X, -5, -EDGES_YZ / 2 } },
	{ { EDGES_X, -EDGES_YZ / 2, EDGES_YZ },{ OTHER_X, -EDGES_YZ / 2, 5 },{ OTHER_X, -EDGES_YZ / 2, -5 },{ EDGES_X, -EDGES_YZ / 2, -EDGES_YZ / 2 } }
};
// Current control points
float ctrlPoints[7][3];
int ctrlPointsSize;
// Temporary control points, to hold the original position when they are being dragged
float tempCtrlPoints[7][3];

int mode; // The currently selected mode
float camera_angle = 0; // The angle of the camera

// Related to point dragging
bool isPointSelected;
int selected_index; // The index of the latest selected point
float mouseWorldXY[2]; // The world coordinates of the last mouse click
int mouseButton = GLUT_RIGHT_BUTTON; // Which click of the button of the mouse is pressed (left, right, etc)
int mouseState = GLUT_UP; // The state of the mouse button (up, down)

// Declare functions
void updateMode(int new_mode);

void initEnvironment()
{
	// Attributes
	glEnable(GL_BLEND);
	glClearColor(BACKGROUND_COLOR[0], BACKGROUND_COLOR[1], BACKGROUND_COLOR[2], BACKGROUND_COLOR[3]); // Set background color

	// Initialize variables
	isPointSelected = false;
	updateMode(1); // Should be the last initialization
}

/*
	Draws the current control points.
*/
void drawPoints()
{
	glColor3fv(POINTS_COLOR); // Set color
	glEnable(GL_POINT_SMOOTH);
	glEnableClientState(GL_VERTEX_ARRAY);
	glPointSize(POINTS_SIZE); // Set size
	glVertexPointer(3, GL_FLOAT, 0, ctrlPoints); // Set point dimensions
	glDrawArrays(GL_POINTS, 0, ctrlPointsSize); // Draw the proper number of points
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_POINT_SMOOTH);
}

/*
	Draws the current control points from a 3D array.
*/
void drawPointsOfSurface()
{
	glColor3fv(SURFACE_POINTS_COLOR); // Set color
	glEnable(GL_POINT_SMOOTH);
	glEnableClientState(GL_VERTEX_ARRAY);
	glPointSize(SURFACE_POINT_SIZE); // Set size
	glVertexPointer(3, GL_FLOAT, 0, ctrlPointsMode4); // Set point dimensions
	glDrawArrays(GL_POINTS, 0, 16); // Draw the proper number of points
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_POINT_SMOOTH);
}

/*
	Draws a line between the two given points.
*/
void drawLine(float point1[3], float point2[3])
{
	glColor3fv(C1_LINE_COLOR);
	glBegin(GL_LINES);
	glVertex3fv(point1);
	glVertex3fv(point2);
	glEnd();
}

/*
	Converts the given screen point to world coordinates.
*/
void screenToWorldPosition(int screenXY[2], float worldXY[2])
{
	GLint viewport[4];
	// Get the current viewport, where the last two positions are screen's width/height
	glGetIntegerv(GL_VIEWPORT, viewport);
	// Convert screen to world coordinates
	worldXY[0] = ((float)screenXY[0] / viewport[2] * WORLD_WIDTH) - (WORLD_WIDTH / 2.0);
	// Screen y is reversed to world y.
	worldXY[1] = (WORLD_HEIGHT - (float)screenXY[1] / viewport[3] * WORLD_HEIGHT) - (WORLD_HEIGHT / 2.0);
}

/*
	Converts the given world point to screen coordinates.
*/
void worldToScreenPosition(float worldXY[2], int screenXY[2])
{
	GLint viewport[4];
	// Get the current viewport, where the last two positions are screen's width/height
	glGetIntegerv(GL_VIEWPORT, viewport);
	// Convert world to screen coordinates
	screenXY[0] = (int)(((worldXY[0] + WORLD_WIDTH / 2.0) / WORLD_WIDTH) * viewport[2]);
	// World y is reversed to screen y.
	screenXY[1] = (int)(viewport[3] - ((worldXY[1] + WORLD_HEIGHT / 2.0) / WORLD_HEIGHT) *  viewport[3]);
}

/*
	Returns the euclidean distance between the given points.
*/
float euclideanDistance(int x1, int y1, int x2, int y2)
{
	int dx = x1 - x2;
	int dy = y1 - y2;
	return sqrt(pow(dx, 2) + pow(dy, 2));
}

/*
	Finds and returns the index of the point that is closest to the target point and the distance is smaller than the given maximum distance.
	Returns -1 if no such point was found.
*/
int closestPointIndex(int target_point[2], float points[][3], int no_points, float max_distance)
{
	int index = -1;
	float distance = FLT_MAX;
	// Find the closest point the the target point
	for (int i = 0; i < no_points; i++)
	{
		// Convert world point to screen point
		int screen_point[2];
		worldToScreenPosition(points[i], screen_point);
		// Calculate distance of the current point to the target point
		float new_dist = euclideanDistance(target_point[0], target_point[1], screen_point[0], screen_point[1]);
		if (new_dist < distance && new_dist < max_distance)
		{
			index = i;
			distance = new_dist;
		}
	}
	return index;
}

/*
	Modifies the given control points so that when berzier is used,
	it will have the effect of a polynomial curve of the initial control points.
*/
void polynomialToBezierControlPoints(float berzier_points[4][3])
{
	// Multiplies the control points with the proper matrix to convert them from polynomial to bezier
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			berzier_points[i][j] = 0; // Set initial value of every cell
			for (int k = 0; k < 4; k++)
			{
				berzier_points[i][j] += polyToBezier[i][k] * ctrlPoints[k][j];
			}
		}
	}
}

void displayCurve(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Clear the window

	switch (mode)
	{
	case 1: // Display polynomial curve using bezier curve
		float bezierPoints[4][3];
		polynomialToBezierControlPoints(bezierPoints);
		glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 4, &bezierPoints[0][0]);
		glEnable(GL_MAP1_VERTEX_3);
		// Draw bezier
		glColor3fv(LINE_COLOR);
		glBegin(GL_LINE_STRIP);
		for (int i = 0; i <= CURVE_POINTS; i++)
			glEvalCoord1f((float)i / CURVE_POINTS);
		glEnd();
		// Draw control points
		drawPoints();
		break;
	case 2: // Display 6 degree bezier curve
		glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 7, &ctrlPoints[0][0]);
		glEnable(GL_MAP1_VERTEX_3);
		// Draw bezier
		glColor3fv(LINE_COLOR);
		glBegin(GL_LINE_STRIP);
		for (int i = 0; i <= CURVE_POINTS; i++)
			glEvalCoord1f((float)i / CURVE_POINTS);
		glEnd();
		// Draw control points
		drawPoints();
		break;
	case 3: // Display two cubic bezier curves being C1 continuous
		// Set first bezier
		glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 4, &ctrlPoints[0][0]);
		glEnable(GL_MAP1_VERTEX_3);
		// Draw first curve
		glColor3fv(LINE_COLOR);
		glBegin(GL_LINE_STRIP);
		for (int i = 0; i <= CURVE_POINTS; i++)
			glEvalCoord1f((float)i / CURVE_POINTS);
		glEnd();
		// Set second bezier
		glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 4, &ctrlPoints[3][0]);
		glEnable(GL_MAP1_VERTEX_3);
		// Draw second curve
		glColor3fv(LINE_COLOR);
		glBegin(GL_LINE_STRIP);
		for (int i = 0; i <= CURVE_POINTS; i++)
			glEvalCoord1f((float)i / CURVE_POINTS);
		glEnd();
		// Draw control points and line
		drawLine(ctrlPoints[2], ctrlPoints[4]);
		drawPoints();
		break;
	case 4: // Display a bezier surface
		glColor3fv(SURFACE_LINE_COLOR);
		glMap2f(GL_MAP2_VERTEX_3, 0.0, 1.0, 3, 4, 0.0, 1.0, 12, 4, &ctrlPointsMode4[0][0][0]);
		glEnable(GL_MAP2_VERTEX_3);
		// Draw surface
		for (int j = 0; j <= SURFACE_LINES; j++) {
			glBegin(GL_LINE_STRIP);
			for (int i = 0; i <= SURFACE_LINES; i++)
				glEvalCoord2f((float)i / SURFACE_LINES, (float)j / SURFACE_LINES);
			glEnd();
			glBegin(GL_LINE_STRIP);
			for (int i = 0; i <= SURFACE_LINES; i++)
				glEvalCoord2f((float)j / SURFACE_LINES, (float)i / SURFACE_LINES);
			glEnd();
		}
		// Draw control points
		drawPointsOfSurface();
	}
	glutSwapBuffers(); // Swap buffers
	return;
}

void mouseClicked(int button, int state, int x, int y)
{
	mouseButton = button;
	mouseState = state;
	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			// Get mouse world position
			int mouseScreenXY[2] = { x, y };
			screenToWorldPosition(mouseScreenXY, mouseWorldXY);
			// Select the closest control point if the distance is less than the maximum allowed distance
			selected_index = closestPointIndex(mouseScreenXY, ctrlPoints, ctrlPointsSize, MAX_POINTER_DISTANCE);
			if (selected_index != -1) { // If a point was selected
				// Temporarily save the original position of all points (because more than one may be moved with dragging)
				for (int i = 0; i < ctrlPointsSize; i++)
				{
					for (int j = 0; j < 3; j++)
					{
						tempCtrlPoints[i][j] = ctrlPoints[i][j];
					}
				}
				isPointSelected = true;
			}
		}
		else if (state == GLUT_UP) {
			// Deselect the point
			isPointSelected = false;
		}
	}
}

void mouseMotion(int x, int y)
{
	if (mode != 4 && mouseButton == GLUT_LEFT_BUTTON && mouseState == GLUT_DOWN)
	{
		// Convert current mouse position to world coordinates
		int currScreenXY[2] = { x,y };
		float currWorldXY[2];
		screenToWorldPosition(currScreenXY, currWorldXY);
		// Calculate world offset between last mouse click and current mouse position
		int offsetWorldXY[2];
		offsetWorldXY[0] = currWorldXY[0] - mouseWorldXY[0];
		offsetWorldXY[1] = currWorldXY[1] - mouseWorldXY[1];
		// Update point
		ctrlPoints[selected_index][0] = tempCtrlPoints[selected_index][0] + offsetWorldXY[0];
		ctrlPoints[selected_index][1] = tempCtrlPoints[selected_index][1] + offsetWorldXY[1];
		if (mode == 2)
		{
			// The first and last control point should be at the same position
			if (selected_index == 0) // First control point is selected
			{
				ctrlPoints[ctrlPointsMode2Size - 1][0] = ctrlPoints[selected_index][0];
				ctrlPoints[ctrlPointsMode2Size - 1][1] = ctrlPoints[selected_index][1];
			}
			else if (selected_index == ctrlPointsMode2Size - 1) // Last control point is selected
			{
				ctrlPoints[0][0] = ctrlPoints[selected_index][0];
				ctrlPoints[0][0] = ctrlPoints[selected_index][1];
			}
		}
		else if (mode == 3)
		{
			// If the middle control point is selected, then the previous and next points should be moved the same distance
			if (selected_index == 3)
			{
				ctrlPoints[selected_index - 1][0] = tempCtrlPoints[selected_index - 1][0] + offsetWorldXY[0];
				ctrlPoints[selected_index - 1][1] = tempCtrlPoints[selected_index - 1][1] + offsetWorldXY[1];
				ctrlPoints[selected_index + 1][0] = tempCtrlPoints[selected_index + 1][0] + offsetWorldXY[0];
				ctrlPoints[selected_index + 1][1] = tempCtrlPoints[selected_index + 1][1] + offsetWorldXY[1];
			}
			// If previous point is selected then the next point should be moved the same distance at the line defined by middle (B) and previous (A) points
			// P(c) = A + c(B-A), for c = 2 => P(c) = 2B - A
			else if (selected_index == 2)
			{
				ctrlPoints[selected_index + 2][0] = (2 * ctrlPoints[selected_index + 1][0]) - ctrlPoints[selected_index][0];
				ctrlPoints[selected_index + 2][1] = (2 * ctrlPoints[selected_index + 1][1]) - ctrlPoints[selected_index][1];
			}
			// If next point is selected then the previous point should be moved the same distance at the line defined by middle (B) and next (A) points
			// P(c) = A + c(B-A), for c = 2 => P(c) = 2B - A
			else if (selected_index == 4)
			{
				ctrlPoints[selected_index - 2][0] = (2 * ctrlPoints[selected_index - 1][0]) - ctrlPoints[selected_index][0];
				ctrlPoints[selected_index - 2][1] = (2 * ctrlPoints[selected_index - 1][1]) - ctrlPoints[selected_index][1];
			}

		}
		// Redisplay
		glutPostRedisplay();
	}
}

/*
	Sets the view of the camera to be orthogonal if isOrthogonal is true, or perspective with the proper camera angle if false.
*/
void setViewOrthogonal(bool isOrthogonal)
{
	if (isOrthogonal)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-(WORLD_WIDTH / 2.0), WORLD_WIDTH / 2.0, -(WORLD_HEIGHT / 2.0), WORLD_HEIGHT / 2.0, 0.0, 200.0); // Set view
		glMatrixMode(GL_MODELVIEW);
	}
	else {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(60, 1.0, 5, 200.0);
		gluLookAt(0.0, 0.0, CAMERA_DISTANCE, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
		gluLookAt(
			0, 0, 0, // the position of the camera
			sin(camera_angle * PI / 180), 0, -cos(camera_angle * PI / 180), // rotation vector
			0, 1, 0); // the up-vector
		glMatrixMode(GL_MODELVIEW);
	}
}

void keyboardInputHandler(int Key, int x, int y)
{
	if (mode == 4)
	{
		switch (Key)
		{
		case GLUT_KEY_LEFT: // Rotate camera left
			camera_angle -= 3;
			break;
		case GLUT_KEY_RIGHT: // Rotate camera right
			camera_angle += 3;
			break;
		};
		// Clamp camera's angle
		if (camera_angle > 360.0)
		{
			camera_angle -= 360;
		}
		else if (camera_angle < 0.0)
		{
			camera_angle += 360;
		}
		// Update camera view
		setViewOrthogonal(false);
		glutPostRedisplay();
	}
}

/*
	Sets the proper control points and updates the current mode.
*/
void updateMode(int new_mode)
{
	// Update mode
	mode = new_mode;
	// Update mode control points
	switch (mode)
	{
	case 1:
		setViewOrthogonal(true);
		for (int i = 0; i < ctrlPointsMode1Size; i++)
		{
			ctrlPoints[i][0] = ctrlPointsMode1[i][0];
			ctrlPoints[i][1] = ctrlPointsMode1[i][1];
			ctrlPoints[i][2] = 0;
			ctrlPointsSize = ctrlPointsMode1Size;
		}
		break;
	case 2:
		setViewOrthogonal(true);
		for (int i = 0; i < ctrlPointsMode2Size; i++)
		{
			ctrlPoints[i][0] = ctrlPointsMode2[i][0];
			ctrlPoints[i][1] = ctrlPointsMode2[i][1];
			ctrlPoints[i][2] = 0;
			ctrlPointsSize = ctrlPointsMode2Size;
		}
		break;
	case 3:
		setViewOrthogonal(true);
		for (int i = 0; i < ctrlPointsMode3Size; i++)
		{
			ctrlPoints[i][0] = ctrlPointsMode3[i][0];
			ctrlPoints[i][1] = ctrlPointsMode3[i][1];
			ctrlPoints[i][2] = 0;
			ctrlPointsSize = ctrlPointsMode3Size;
		}
		break;
	case 4:
		camera_angle = 0;
		setViewOrthogonal(false);
	}
	glutPostRedisplay();
}

void menu(int id) {
	if (id != 5) {
		updateMode(id);
	}
	else { // Quit
		exit(0);
	}
}

void main(int argc, char** argv)
{
	// Standard GLUT initialization
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(500, 500);

	// Create Window
	glutInitWindowPosition(500, 200);
	glutCreateWindow("Curves");
	glEnable(GL_DEPTH_TEST);

	// Create menu
	glutCreateMenu(menu);
	glutAddMenuEntry("Cubic Interpolation Curve", 1);
	glutAddMenuEntry("Bezier 6-Degree Curve", 2);
	glutAddMenuEntry("Two cubic C1 Bezier Curves", 3);
	glutAddMenuEntry("Bezier Surface", 4);
	glutAddMenuEntry("Quit", 5);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	glutMouseFunc(mouseClicked);
	glutMotionFunc(mouseMotion);
	glutSpecialFunc(keyboardInputHandler);
	glutDisplayFunc(displayCurve);

	initEnvironment(); /* Initialize environment-view properties */

	glutMainLoop(); /* Enter event loop */
}