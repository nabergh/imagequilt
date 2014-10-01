#ifndef PATCH_H
#define PATCH_H

#include <vector>
#include "CImg.h"

using namespace std;


class Patch{

public:
	int top, left, bottom, right, rightDiff, botDiff;
	vector< vector<int> > diffMatRight, diffMatBot;
	vector< vector<bool> > rightSeam, botSeam;
	Patch* rPatch;
	Patch* bPatch;
	// cimg_library::CImg<unsigned char>& img;
	
	Patch(int, int, int, int);
	// Patch& operator=(const Patch&);

	int compRight(Patch& right);
	int compBottom(Patch& right, cimg_library::CImg<unsigned char> & img);

	vector< vector<bool> > findSeam(vector< vector<int> >&);
	void findRightSeam();
	void findBottomSeam();

};



#endif
