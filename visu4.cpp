/* 	HMIN318
	2018-2019
	Gérard Subsol
	(adapted of a program written by Benjamin Gilles)

	Compilation:
	(Windows version) g++ -o visu4.exe visu4.cpp -O2 -lgdi32
	(Linux  version) g++  -o visu4.exe visu4.cpp -O2 -L/usr/X11R6/lib  -lm  -lpthread -lX11

	Execution :
	visu4.exe knix.hdr
*/

/* Warning! the MPR value does not deal with the voxel size. It is assumed isotropic. */

#include "CImg.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <array>
#include <string>

using namespace cimg_library;
using namespace std;


void displayImage(int dim[], CImg<> img, int img_nb) {
	/* Create the display window of size 512x512 */
	char buf[32];
	sprintf_s(buf, "Image %d", img_nb);
	const char* p = buf;
	CImgDisplay disp(512, 512, p);


	/* The 3 displayed slices of the MPR visualisation */
	int displayedSlice[] = { dim[0] / 2,dim[1] / 2,dim[2] / 2 };

	/* Slice corresponding to mouse position: */
	unsigned int coord[] = { 0,0,0 };

	/* The display window corresponds to a MPR view which is decomposed into the following 4 quadrants:
	2=original slice size=x y        0 size=z y
	1= size=x z                     -1 corresponds to the 4th quarter where there is nothing displayed */
	int plane = 2;

	/* For a first drawing, activate the redrawing flag */
	bool redraw = true;
	bool blured = false;
	/* Manage the display windows: ESC, or closed -> close the main window */
	while (!disp.is_closed() && !disp.is_keyESC()) // Main loop
	{
		/* List of events */

		/* Resizing */
		if (disp.is_resized())
		{
			disp.resize();
		}
		/* Movement of the mouse */

		/* If the mouse is inside the display window, find the active quadrant
		and the relative position within the active quadrant */
		if (disp.mouse_x() >= 0 && disp.mouse_y() >= 0)
		{

			unsigned int mX = disp.mouse_x() * (dim[0] + dim[2]) / disp.width();
			unsigned int mY = disp.mouse_y() * (dim[1] + dim[2]) / disp.height();

			if (mX >= dim[0] && mY < dim[1])
			{
				plane = 0;
				coord[1] = mY;
				coord[2] = mX - dim[0];
				coord[0] = displayedSlice[0];
			}
			else
			{
				if (mX < dim[0] && mY >= dim[1])
				{
					plane = 1;
					coord[0] = mX;
					coord[2] = mY - dim[1];
					coord[1] = displayedSlice[1];
				}
				else
				{
					if (mX < dim[0] && mY < dim[1])
					{
						plane = 2;
						coord[0] = mX;
						coord[1] = mY;
						coord[2] = displayedSlice[2];
					}
					else
					{
						plane = -1;
						coord[0] = 0;
						coord[1] = 0;
						coord[2] = 0;
					}
				}
			}
			redraw = true;
		}

		/* Click Right button to get a position */
		if (disp.button() & 2 && (plane != -1))
		{
			for (unsigned int i = 0; i < 3; i++)
			{
				displayedSlice[i] = coord[i];
			}
			redraw = true;
		}
		/* Wheel interaction */
		if (disp.wheel())
		{
			displayedSlice[plane] = displayedSlice[plane] + disp.wheel();

			if (displayedSlice[plane] < 0)
			{
				displayedSlice[plane] = 0;
			}
			else
			{
				if (displayedSlice[plane] >= (int)dim[plane])
				{
					displayedSlice[plane] = (int)dim[plane] - 1;
				}
			}

			/* Flush all mouse wheel events in order to not repeat the wheel event */
			disp.set_wheel();

			redraw = true;
		}

		/* If any change, we need to redraw the main windows */
		if (redraw)
		{
			/* Create a 2D image based on the MPR projections given by a projection point
			which is the intersection of the displayed slices */

			CImg<> mpr_img;
			mpr_img = img.get_projections2d(displayedSlice[0], displayedSlice[1], displayedSlice[2]);

			/* The MPR image has a given size. It needs to be resized in order to fit at best in the display window */
			mpr_img.resize(512, 512);

			/* Display the MPR image in the display windows */
			disp.display(mpr_img);

			/* To avoid repetitive continuous redrawing */
			redraw = false;
		}
	}
}

float distancePoints(array<int, 4> a, array<int, 4> b) {
	return pow(pow(b[0] - a[0], 2) + pow(b[1] - a[1], 2) + pow(b[2] - a[2], 2), 0.5);
}

void writeToFile(vector<vector<array<int, 4>>> data, string name) {
	printf("Saving txt to %s\n", name.c_str());
	ofstream myfile(name.c_str(), ios::out);
	if (myfile.is_open()) {
		for (int i = 0; i < data.size(); i++) {
			myfile << "ARRAY " << i << "\n";
			for (int j = 0; j < data[i].size(); j++) {
				myfile << data[i][j][0] << " " << data[i][j][1] << " " << data[i][j][2] << "\n";
			}
		}
		myfile.close();
	}else {
		printf("FAILED\n");
	}
}

void writeToObj(vector<vector<array<int, 4>>> data, string name) {
	printf("Saving obj to %s\n", name.c_str());
	ofstream myfile(name.c_str(), ios::out);
	if (myfile.is_open()) {
		for (int i = 0; i < data.size(); i++) {
			for (int j = 0; j < data[i].size(); j++) {
				myfile << "v " << data[i][j][0] << ".000000 " << data[i][j][1] << ".000000 " << data[i][j][2] << ".000000\n";
			}
		}
		int count = 1;
		for (int i = 0; i < data.size(); i++) {
			myfile << "| ";
			for (int j = 0; j < data[i].size()-1; j++) {
				myfile << count << " ";
				count++;
			}
			myfile << count << "\n";
			count++;
		}
		myfile.close();
	}
	else {
		printf("FAILED\n");
	}
}

void pathCreation(vector<vector<array<int, 4>>> allTheBarys) {
	printf("Cell path reconstruction starting with %i frames\n", allTheBarys.size());
	vector<vector<array<int, 4>>> orderedBarys;
	for (int i = 0; i < allTheBarys[0].size(); i++) {
		vector<array<int, 4>> newBranch;
		newBranch.push_back(allTheBarys[0][i]);
		orderedBarys.push_back(newBranch);
	}

	vector<array<int, 4>> barys1;
	vector<array<int, 4>> barys2;

	for (int frame = 0; frame < allTheBarys.size() - 1; frame++) {
		printf("Comparing frames %i and %i\n", frame, frame + 1);
		barys1 = allTheBarys[frame];
		barys2 = allTheBarys[frame + 1];
		printf("Matching frames\n");
		while (!barys1.empty() && !barys2.empty()) {  //combining the two frames
			float minDistance = INFINITY;
			int minFrom1 = 0, minFrom2 = 0;
			for (int i = 0; i < barys1.size(); i++) {
				for (int j = 0; j < barys2.size(); j++) {
					float distance = distancePoints(barys1[i], barys2[j]);
					if (distance < minDistance) {
						minDistance = distance;
						minFrom1 = i;
						minFrom2 = j;
					}
				}
			}
			for (int i = 0; i < orderedBarys.size(); i++) {
				if (find(orderedBarys[i].begin(), orderedBarys[i].end(), barys1[minFrom1]) != orderedBarys[i].end()) {
					orderedBarys[i].push_back(barys2[minFrom2]);
					barys1.erase(barys1.begin() + minFrom1);
					barys2.erase(barys2.begin() + minFrom2);
					break;
				}
			}
		}
		printf("Adding new elements\n");
		if (!barys2.empty()) { //add unmatched elements as new 
			for (int i = 0; i < barys2.size(); i++) {
				vector<array<int, 4>> newBranch;
				newBranch.push_back(barys2[i]);
				orderedBarys.push_back(newBranch);
			}
		}
	}
	printf("Cell path reconstruction ended with %i paths\n", orderedBarys.size());
	writeToFile(orderedBarys, "orderedElements.txt");
	writeToObj(orderedBarys, "orderedElements.obj");
}

/* Main program */
int main(int argc, char** argv) {

	int startingIndex = 0;
	int endingIndex = 4;
	string naming = "stack-";
	string path = "DATA1\\";

	vector<vector<array<int, 4>>> allTheBarys;

	for (int img_nb = startingIndex; img_nb <= endingIndex; img_nb++) {
		printf("Loading image %i\n", img_nb);
		/* Create and load the 3D image */
		CImg<> img;
		float voxelsize[3];

		/* Load in Analyze format and get the voxel size in an array */
		string imageName = path + naming + to_string(img_nb) + ".hdr";

		const char* c = imageName.c_str();

		img.load_analyze(c, voxelsize);

		/* Get the image dimensions */
		int dim[] = { img.width(),img.height(),img.depth() };
		printf("Reading %s. Dimensions=%d %d %d\n", c, dim[0], dim[1], dim[2]);
		printf("Voxel size=%f %f %f\n", voxelsize[0], voxelsize[1], voxelsize[2]);

		//displayImage(dim, img, img_nb);


		//Apply median filter
		printf("Median\n");
		img.blur_median(3);
		//displayImage(dim, img, img_nb);
		//Apply threshold
		printf("Threshold\n");
		img.threshold(30);
		//displayImage(dim, img, img_nb);
		//Eroding like never before
		printf("Erosion\n");
		img.erode(3);
		//Back to fat
		printf("Dilation\n");
		img.dilate(3);
		//displayImage(dim, img, img_nb);
		//Label it up
		printf("Labeling\n");
		img.label(0);
		//displayImage(dim, img, img_nb);
		//calculate min and max
		printf("Min-Max\n");
		float min = *img.data(256, 256, 12, 0);
		int x_m = 0; int y_m = 0; int z_m = 0;
		float max = *img.data(256, 256, 12, 0);
		int x_M = 0; int y_M = 0; int z_M = 0;

		for (int i = 0; i < dim[0]; i++) {
			for (int j = 0; j < dim[1]; j++) {
				for (int k = 0; k < dim[2]; k++) {
					if (*img.data(i, j, k, 0) < min) { min = *img.data(i, j, k, 0); x_m = i; y_m = j; z_m = k; }
					if (*img.data(i, j, k, 0) > max) { max = *img.data(i, j, k, 0); x_M = i; y_M = j; z_M = k; }
				}
			}
		}
		printf("Barycenters\n");
		vector<array<int, 4>> barycenters;
		array<int, 4> val = { 0,0,0,0 };
		vector<array<int, 4>> sums;
		for (int i = 0; i <= max - min; i++) {
			sums.push_back(val);
		}

		for (int i = 0; i < dim[0]; i++) {
			for (int j = 0; j < dim[1]; j++) {
				for (int k = 0; k < dim[2]; k++) {
					int value = *img.data(i, j, k, 0);
					sums[value][0] += i;
					sums[value][1] += j;
					sums[value][2] += k;
					sums[value][3] ++;
				}
			}
		}
		printf("Barycenters being processed\n");
		for (int i = 0; i < sums.size(); i++) {
			array<int, 4> bary = { sums[i][0] / sums[i][3], sums[i][1] / sums[i][3], sums[i][2] / sums[i][3], 0 };
			barycenters.push_back(bary);
		}
		printf("Barycenters calculated\n");
		allTheBarys.push_back(barycenters);
		printf("Finished image %i with %i elements detected\n", img_nb, barycenters.size());

	}

	
	writeToFile(allTheBarys, "unorderedElementsTxt");

	pathCreation(allTheBarys);
	return 0;
}


