/*
 *  Authors:     Jake Cooper
 *  Date:        November 4th, 2013
 *  File name:   WireFrame.c
 *  Description: Generate HTML5 and SVG code to display wire frame.
 *               The wire frame is specified in an input text file identified WIREFRAME_INPUT_FILENAME.
 *               The HTML5/SVG code is output into a text file identified by HTML5_SVG_OUTPUT_FILENAME.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

//The name of the input file
#define WIREFRAME_INPUT_FILENAME ("input.txt")
// The name of the output file
#define HTML5_SVG_OUTPUT_FILENAME ("output.html")

/* ========================================================================= */
/*                              Type Definitions                             */
/* ========================================================================= */

// Object colours
#define OBJECT_COLOR_0  ("magenta")
#define OBJECT_COLOR_1  ("cyan")
#define OBJECT_COLOR_2  ("blue")
#define OBJECT_COLOR_3  ("purple")
#define CANVAS_SIZE_X (500)
#define CANVAS_SIZE_Y (500)
//The amount to rotate around the X axis (in radians)
#define ROTATION_ANGLE_X (20*(M_PI/180)) //i.e. 20 degrees
//The amount to rotate around the Y axis (in radians)
#define ROTATION_ANGLE_Y (0*(M_PI/180)) //i.e. 0 degrees
//The amount to rotate around the Z axis (in radians)
#define ROTATION_ANGLE_Z (-45*(M_PI/180)) //i.e. 45 degrees

#define MATRIX_MAX (4)
#define MAX_WIREFRAME_EDGES (5000)
#define POINTS_PER_EDGE  (6)

typedef float Matrix[MATRIX_MAX][MATRIX_MAX];

/* ========================================================================= */
/*                       Library Function  Declarations                      */
/*            These functions are defined at the end of the file.            */
/* ========================================================================= */


/* writePrologue
   This function writes the initial part of the HTML5 file (which sets up the
   graphics interface). This must be called before any graphical data is written
   to the file.
*/
void writePrologue(FILE *f);

/* writeEpilogue
   This function writes the final part of the HTML5 file. This must be called
   after all data has been written, but before the file is closed.
   (The caller is responsible for closing the file afterwards).
*/
void writeEpilogue(FILE *f);

/* writeEdge
   This function writes an edge into the HTML5 file referenced by f.
   The edge segment begins at point (x1,y1) and ends at point (x2,y2).
   The edge is drawn with the specified colour.
*/
void writeEdge(FILE *f, float x1, float y1, float x2, float y2, char colour[]);

/* projectionMatrix
   This function sets the provided matrix to a 2x4 projection matrix, which
   is used to convert points in 3 dimensions to points in 2 dimensions.
*/

void projectionMatrix(Matrix outM);

/* rotationMatrixX
   Sets the matrix outM to contain a 4x4 rotation matrix for a rotation
   of the given angle (which is provided in radians) around the X axis.

*/
void rotationMatrixX(float angle, Matrix outM);

/* rotationMatrixY
   Sets the matrix outM to contain a 4x4 rotation matrix for a rotation
   of the given angle (which is provided in radians) around the Y axis.
*/

void rotationMatrixY(float angle, Matrix outM);

/* rotationMatrixZ
   Sets the matrix outM to contain a 4x4 rotation matrix for a rotation
   of the given angle (which is provided in radians) around the Z axis.

*/
void rotationMatrixZ(float angle, Matrix outM);

/* scalingMatrix
   Sets the matrix outM to contain a 4x4 scaling matrix where the three
   scaling factors are xs, ys, zs.

*/
void scalingMatrix(float xs, float ys, float zs, Matrix outM);

/* translationMatrix
   Sets the matrix outM to contain a 4x4 translation matrix where the three
   translation factors are xt, yt, zt.

*/
void translationMatrix(float xt, float yt, float zt, Matrix outM);

/* readWireFrame
   Reads a wireframe from the file WIREFRAME_INPUT_FILENAME.  The wireframe is
   returned via parameter wireFrame.  The function returns the number of edges
   in the wireframe.
*/
int readWireFrame(Matrix wireFrame[]);

/* matMul
   Computes the matrix product A*B, which is stored in the matrix C.
   ARows and ACols contain the number of rows and columns of the matrix A.
   BCols contains the number of columns in B (the number of rows in B is the same
   as the number of columns in A).
*/
void matMul(Matrix A, Matrix B, int ARows, int ACols, int BCols, Matrix C){
	int x,y,z;
	for(x=0; x<ARows; x++)
	for(y=0; y<BCols; y++){
		C[x][y] = 0;
		for(z=0; z<ACols; z++){
			C[x][y]+=(A[x][z])*(B[z][y]);
		}
	}
} /* matMul */

/* computeTransformationMatrix
   This function creates matrices for a number of transforms and combines them
   into a signle transform matriix which is returned in parameter M.

   You are to modify this function to include three rotations as follows:

   by ROTATION_ANGLE_X about the x axis
   by ROTATION_ANGLE_Y about the y axis
   by ROTATION_ANGLE_Y about the z axis

   The rotations are to apply BEFORE the scaling, translation and projection
   transformations.

*/
void computeTransformationMatrix(Matrix M, float scale, float xt, float yt, float zt) {
	// returns final transformation in M
	Matrix P;   // projection matrix
	Matrix S;   // scaling matrix
	Matrix T;   // translation matrix
	Matrix X,Y,Z; //rotation matrices
	Matrix YZ, XYZ; //resulting matrices

	rotationMatrixX(ROTATION_ANGLE_X, X);
	rotationMatrixY(ROTATION_ANGLE_Y, Y);
	rotationMatrixZ(ROTATION_ANGLE_Z, Z);
	projectionMatrix(P);
	scalingMatrix(scale, scale, -scale, S);  // note -scale for z because SVG vertical axis goes downward
	translationMatrix(xt, yt, zt, T);

	Matrix SXYZ, TSXYZ;
	// compute final transformation matrix M using matrix multiplication M = P * T * S * R_X * R_Y * R_Z
	matMul(Y, Z, 4, 4, 4, YZ);
	matMul(X,YZ, 4, 4, 4, XYZ);
	matMul(S, XYZ, 4, 4, 4, SXYZ);
	matMul(T, SXYZ, 4, 4, 4, TSXYZ);
	matMul(P, TSXYZ, 2, 4, 4, M);
} /*computeTransformationMatrix*/


void drawWireframe(FILE* outFile, Matrix wireFrame[], int noEdges, Matrix M, char col[]){
	Matrix R;
		int edge;
		for (edge=0; edge<noEdges; edge++) {
			// transform edge
			matMul(M, wireFrame[edge], 2, 4, 2, R);
			// generate SVG for edge
			writeEdge(outFile, R[0][0], R[1][0], R[0][1], R[1][1], col);
			printf("%7.2f %7.2f %7.2f %7.2f\n", R[0][0], R[1][0], R[0][1], R[1][1]);
		} /*for*/
}

/* generateSVGFile
   This function opens the file HTML5_SVG_OUTPUT_FILENAME for writing
   and writes the SVG required to display the wireFrame on a web page.
*/

void generateSVGfile(Matrix wireFrame[], int noEdges) {

	FILE *outFile = fopen(HTML5_SVG_OUTPUT_FILENAME, "w");
	writePrologue(outFile);

    Matrix M;   // compute final transformation matrix
	computeTransformationMatrix(M, 200, 125, 0, 125);
	drawWireframe(outFile, wireFrame, noEdges, M, OBJECT_COLOR_0);

	computeTransformationMatrix(M, 150, 375, 0, 125);
	drawWireframe(outFile, wireFrame, noEdges, M, OBJECT_COLOR_1);

	computeTransformationMatrix(M, 100, 125, 0, 375);
	drawWireframe(outFile, wireFrame, noEdges, M, OBJECT_COLOR_2);

	computeTransformationMatrix(M, 50, 375, 0, 375);
	drawWireframe(outFile, wireFrame, noEdges, M, OBJECT_COLOR_3);

	writeEpilogue(outFile);

	fclose(outFile);
} /*generateSVGfile*/



int main(){
	Matrix wireFrame[MAX_WIREFRAME_EDGES];

	int noEdges = readWireFrame(wireFrame);
	generateSVGfile(wireFrame, noEdges);

	return EXIT_SUCCESS;
} /* main */


/* ========================================================================= */
/*                           Library Functions                               */
/*        These are declared above, and will be useful to generate           */
/*            				the output file.           					     */
/* ========================================================================= */


void writePrologue(FILE *f){
	if (f == NULL){
		printf("writePrologue error: output file == NULL\n");
		exit(EXIT_FAILURE);
	} /*if*/
	fputs("<!DOCTYPE html>\n", f);
	fputs("<html>\n", f);
	fputs("<head>\n", f);
	fputs("<title>CSC 111 Assignment 6 Part II</title>\n", f);
	fputs("</head>\n", f);
	fputs("<body>\n", f);
	fprintf(f,"<svg width=\"%dpx\" height=\"%dpx\">\n", CANVAS_SIZE_X, CANVAS_SIZE_Y);
} /* writePrologue */

void writeEpilogue(FILE *f){
	if (f == NULL){
		printf("writeEpilogue error: output file == NULL\n");
		exit(EXIT_FAILURE);
	} /*if*/
	fputs("</svg>\n",f);
	fputs("</body>\n",f);
	fputs("</html>\n",f);
} /* writeEpilogue */

void projectionMatrix(Matrix outM){
	outM[0][0] = 1; outM[0][1] = 0; outM[0][2] = 0; outM[0][3] = 0;
	outM[1][0] = 0; outM[1][1] = 0; outM[1][2] = 1; outM[1][3] = 0;
} /* projectionMatrix */

void rotationMatrixX(float angle, Matrix outM){
	outM[0][0] = 1; outM[0][1] = 0;           outM[0][2] = 0;          outM[0][3] = 0;
	outM[1][0] = 0; outM[1][1] = cos(angle);  outM[1][2] = -sin(angle); outM[1][3] = 0;
	outM[2][0] = 0; outM[2][1] = sin(angle); outM[2][2] = cos(angle); outM[2][3] = 0;
	outM[3][0] = 0; outM[3][1] = 0;           outM[3][2] = 0;          outM[3][3] = 1;
} /* rotationMatrixX */

void rotationMatrixY(float angle, Matrix outM){
	outM[0][0] = cos(angle);  outM[0][1] = 0; outM[0][2] = -sin(angle);  outM[0][3] = 0;
	outM[1][0] = 0;           outM[1][1] = 1; outM[1][2] = 0;           outM[1][3] = 0;
	outM[2][0] = sin(angle); outM[2][1] = 0; outM[2][2] = cos(angle);  outM[2][3] = 0;
	outM[3][0] = 0;           outM[3][1] = 0; outM[3][2] = 0;           outM[3][3] = 1;
} /* rotationMatrixY */

void rotationMatrixZ(float angle, Matrix outM){
	outM[0][0] = cos(angle); outM[0][1] = -sin(angle); outM[0][2] = 0; outM[0][3] = 0;
	outM[1][0] = sin(angle); outM[1][1] = cos(angle);  outM[1][2] = 0; outM[1][3] = 0;
	outM[2][0] = 0;          outM[2][1] = 0;           outM[2][2] = 1; outM[2][3] = 0;
	outM[3][0] = 0;          outM[3][1] = 0;           outM[3][2] = 0; outM[3][3] = 1;
} /* rotationMatrixZ */

void scalingMatrix(float xs, float ys, float zs, Matrix outM){
	outM[0][0] = xs; outM[0][1] = 0;  outM[0][2] = 0;  outM[0][3] = 0;
	outM[1][0] = 0;  outM[1][1] = ys; outM[1][2] = 0;  outM[1][3] = 0;
	outM[2][0] = 0;  outM[2][1] = 0;  outM[2][2] = zs; outM[2][3] = 0;
	outM[3][0] = 0;  outM[3][1] = 0;  outM[3][2] = 0;  outM[3][3] = 1;
} /* scaleingMatrix */

void translationMatrix(float xt, float yt, float zt, Matrix outM){
	outM[0][0] = 1; outM[0][1] = 0; outM[0][2] = 0; outM[0][3] = xt;
	outM[1][0] = 0; outM[1][1] = 1; outM[1][2] = 0; outM[1][3] = yt;
	outM[2][0] = 0; outM[2][1] = 0; outM[2][2] = 1; outM[2][3] = zt;
	outM[3][0] = 0; outM[3][1] = 0; outM[3][2] = 0; outM[3][3] = 1;
} /* translationMatrix */

void writeEdge(FILE *f, float x1, float y1, float x2, float y2, char colour[]){
	if (f == NULL){
		printf("writeEdge error: output file == NULL\n");
		exit(EXIT_FAILURE);
	} /*if*/
	fprintf(f,"<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\", y2=\"%.1f\" style=\"stroke: %s;\" />\n",
			x1, y1, x2, y2, colour);
} /* writeEdge */

int readWireFrame(Matrix wireFrame[]) {
	FILE *inFile = fopen(WIREFRAME_INPUT_FILENAME, "r");
	if (inFile == NULL){
		printf("Error: Unable to open input file %s\n", WIREFRAME_INPUT_FILENAME);
		exit(EXIT_FAILURE);
	} /*if*/

	int edge = 0;
	int noItemsRead;

	while(true) {
		noItemsRead = fscanf(inFile, "%f %f %f %f %f %f",
				             &wireFrame[edge][0][0],
				             &wireFrame[edge][1][0],
				             &wireFrame[edge][2][0],
				             &wireFrame[edge][0][1],
				             &wireFrame[edge][1][1],
				             &wireFrame[edge][2][1]);
		if (noItemsRead != POINTS_PER_EDGE) break;

		wireFrame[edge][3][0] = 1;
		wireFrame[edge][3][1] = 1;
		edge++;
		if(edge == MAX_WIREFRAME_EDGES) break;  // wireFrame is full
	} /*while*/

	fclose(inFile);

	return edge;
} /*readWireFrame*/
