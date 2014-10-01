#include "CImg.h"
#include <string>
#include <time.h>
#include <math.h>
#include "patch.h"
using namespace cimg_library;

const int MAX_INT = ~(1 << 31);

CImg<unsigned char> source;
CImg<unsigned char> quilt;
CImg<unsigned char> diffs;
std::vector< std::vector<Patch> > patches;
int spectrum;
int ss;
int sw;
int patchW;
int patchH;

Patch::Patch(int t, int l, int b, int r) {
	top = t;
	left = l;
	bottom = b;
	right = r;
	// img = i;
	rightDiff = MAX_INT;
	botDiff = MAX_INT;
	rPatch = NULL;
	bPatch = NULL;
}

/*Patch& Patch::operator=(const Patch& p) {
    img = p.img;
    top = p.top;
    left = p.left;
    bottom = p.bottom;
    right = p.right;
    rightDiff = p.rightDiff;
    botDiff = p.botDiff;
    rPatch = p.rPatch;
    bPatch = p.bPatch;
    return *this;
}*/

int Patch::compRight(Patch &currPatch) {
	int w = fmin(currPatch.right - currPatch.left, sw);
	int h = currPatch.bottom - currPatch.top;
	int currentDiff = 0;
	std::vector< std::vector<int> > diffMat = std::vector< std::vector<int> >(w, std::vector<int>(h, 0));
	for (int i = 0; i < w; ++i) {
		for (int j = 0; j < h; ++j) {
			for (int c = 0; c < spectrum; ++c) {
				int diff = abs((int) source(right - w + i, top + j, c) - (int) source(currPatch.left + i, currPatch.top + j, c));
				currentDiff += diff;
				diffMat[i][j] = diffMat[i][j] + diff;
			}
		}
	}
	if (currentDiff <= rightDiff) {
		rightDiff = currentDiff;
		diffMatRight = diffMat;
		rPatch = &currPatch;
	}
	return currentDiff;
}

int Patch::compBottom(Patch &currPatch, CImg<unsigned char> &image) {
	int w = currPatch.right - currPatch.left;
	int h = fmin(currPatch.bottom - currPatch.top, sw);
	int currentDiff = 0;
	std::vector< std::vector<int> > diffMat = std::vector< std::vector<int> >(w, std::vector<int>(h, 0));
	for (int i = 0; i < w; ++i) {
		for (int j = 0; j < h; ++j) {
			for (int c = 0; c < spectrum; ++c) {
				int diff = abs((int) image(left + i, bottom - h + j, c) - (int) image(currPatch.left + i, currPatch.top + j, c));
				currentDiff += diff;
				diffMat[i][j] = diffMat[i][j] + diff;
			}
		}
	}
	if (currentDiff <= botDiff) {
		botDiff = currentDiff;
		diffMatBot = diffMat;
		bPatch = &currPatch;
	}
	return currentDiff;
}

std::vector< std::vector<bool> > Patch::findSeam(std::vector< vector<int> > &diffMat) {
	int w = diffMat.size();
	int h = diffMat[0].size();
	std::vector< std::vector<int> > weightMat = std::vector< std::vector<int> >(w, std::vector<int>(h, 0));

	//filling each cell in the weight matrix with the cost to get to that cell from any of the three adjacent cells above
	for (int i = 0; i < w; ++i) {
		weightMat[i][0] = diffMat[i][0];
	}
	for (int j = 1; j < h; ++j) {
		for (int i = 0; i < w; ++i) {
			int tl = MAX_INT;
			int t = MAX_INT;
			int tr = MAX_INT;
			if (i != 0) {
				tl = weightMat[i - 1][j - 1];
			}
			t = weightMat[i][j - 1];
			if (i != w - 1) {
				tr = weightMat[i + 1][j - 1];
			}
			weightMat[i][j] = diffMat[i][j] + min(min(tl, t), tr);
		}
	}

	//tracing back and dividing seam matrix in two
	std::vector< std::vector<bool> > seam = std::vector< std::vector<bool> >(w, std::vector<bool>(h, 0));

	int min = weightMat[0][h - 1];
	int minIndex = 0;
	for (int i = 1; i < w; ++i) {
		if (weightMat[i][h - 1] < min) {
			minIndex = i;
			min = weightMat[i][h - 1];
		}
	}
	for (int i = 0; i < w; ++i) {
		seam[i][h - 1] = i <= minIndex;
	}
	for (int j = h - 2; j >= 0; j--) {
		int l = MAX_INT;
		int m = MAX_INT;
		int r = MAX_INT;
		if (minIndex != 0) {
			l = diffMat[minIndex - 1][j];
		}
		m = diffMat[minIndex][j];
		if (minIndex != w - 1) {
			r = diffMat[minIndex + 1][j];
		}

		if (l < m && l < r) {
			minIndex--;
		} else if (r < m) {
			minIndex++;
		}

		for (int i = 0; i < w; ++i) {
			seam[i][j] = i <= minIndex;
		}
	}

	return seam;

	/*  for (int i = 0; i < w; ++i) {
	        printf("[ ");
	        for (int j = 0; j < h; ++j) {
	            printf(" %d", (bool) rightSeam[i][j]);
	        }
	        printf(" ]\n");
	    }*/
}

void Patch::findRightSeam() {
	rightSeam = findSeam(diffMatRight);
}

void Patch::findBottomSeam() {
	std::vector< std::vector<int> > diffMat = std::vector< std::vector<int> >(diffMatBot[0].size(), std::vector<int>(diffMatBot.size(), 0));
	for (size_t i = 0; i < diffMatBot.size(); ++i) {
		for (size_t j = 0; j < diffMatBot[0].size(); ++j) {
			diffMat[j][i] = diffMatBot[i][j];
		}
	}
	std::vector< std::vector<bool> > seam = findSeam(diffMat);
	botSeam = std::vector< std::vector<bool> >(seam[0].size(), std::vector<bool>(seam.size(), 0));
	for (size_t i = 0; i < seam.size(); ++i) {
		for (size_t j = 0; j < seam[0].size(); ++j) {
			botSeam[j][i] = seam[i][j];
		}
	}
	// for (size_t i = 0; i < botSeam.size(); ++i) {
	//  printf("[ ");
	//  for (size_t j = 0; j < botSeam[0].size(); ++j) {
	//      printf(" %d", (bool) botSeam[i][j]);
	//  }
	//  printf(" ]\n");
	// }
}

bool loadImg(char *fName) {
	source = CImg<unsigned char>(fName);
	spectrum = source.spectrum();
	quilt = CImg<unsigned char>(ss * patchW - sw * (patchW - 1), ss * patchH, 1, source.spectrum(), 0);
	patches = std::vector< std::vector<Patch> >(ss * patchW, std::vector<Patch>(ss * patchH, Patch(0, 0, 0, 0)));
	return true;
}

bool getPatches() {
	for (int y1 = 0; y1 < quilt.height(); y1 += ss - sw) {
		for (int x1 = 0; x1 < quilt.width(); x1 += ss - sw) {
			// for (int x1 = 0; x1 < ss * 2; x1 += ss) {
			// for (int y1 = 0; y1 < ss; y1 += ss) {

			int minDiff = MAX_INT;
			for (int d = 0; d < 300; ++d) {
				int left = rand() % (source.width() - ss);
				int top = rand() % (source.height() - ss);
				Patch p = Patch(top, left, top + ss, left + ss);
				if (x1 != 0 && y1 != 0) {
					if (patches[x1 - ss + sw][y1].compRight(p) + patches[x1][y1 - ss + sw].compBottom(p, source) < minDiff) {
						minDiff = patches[x1 - ss + sw][y1].rightDiff + patches[x1][y1 - ss].botDiff;
						patches[x1][y1] = p;
					}
				} else if (x1 != 0) {
					if (patches[x1 - ss + sw][y1].compRight(p) < minDiff) {
						minDiff = patches[x1 - ss + sw][y1].rightDiff;
						patches[x1][y1] = p;
					}
				} else if (y1 != 0) {
					if (patches[x1][y1 - ss + sw].compBottom(p, source) < minDiff) {
						minDiff = patches[x1][y1 - ss].botDiff;
						patches[x1][y1] = p;
					}
				} else {
					patches[x1][y1] = p;
					break;
				}
			}
			int x2;
			int y2 = y1 + ss > quilt.height() ? quilt.height() : y1 + ss;
			Patch p2 = patches[x1][y1];
			if (x1 != 0) {
				Patch p1 = patches[x1 - ss + sw][y1];
				p1.findRightSeam();
				int w = fmin(p2.right - p2.left, sw);
				int h = fmin(p2.bottom - p2.top, y2 - y1);

				for (int i = 0; i < w; ++i) {
					for (int j = 0; j < h; ++j) {
						if (p1.rightSeam[i][j]) {
							for (int c = 0; c < spectrum; c++) {
								quilt(x1 + i, y1 + j, c) = source(p1.right - sw + i, p1.top + j, c);
							}
						} else {
							for (int c = 0; c < spectrum; c++) {
								quilt(x1 + i, y1 + j, c) = source(p2.left + i, p2.top + j, c);
							}
						}
					}
				}
				x2 = x1 + ss > quilt.width() ? quilt.width() : x1 + ss - sw;
				for (int i = 0; x1 + w + i < x2; i++) {
					for (int j = 0; j + y1 < y2; j++) {
						for (int c = 0; c < spectrum; c++) {
							quilt(x1 + w + i, y1 + j, c) = source(p2.left + w + i, p2.top + j, c);
						}
					}
				}
			} else {
				x2 = x1 + ss - sw;
				for (int i = 0; i + x1 < x2; i++) {
					for (int j = 0; j + y1 < y2; j++) {
						for (int c = 0; c < spectrum; c++) {
							quilt(x1 + i, y1 + j, c) = source(p2.left + i, p2.top + j, c);
						}
					}
				}
			}

			//row finished, stitch this row with the one above
			// if (y1 != 0 && x2 == quilt.width()) {
			// 	Patch p1 = Patch(y1 - ss + sw, 0, y1, quilt.width());
			// 	Patch p2 = Patch(y1, 0, y2, quilt.width());
			// 	p1.compBottom(p2, quilt);
			// 	p1.findBottomSeam();
			// 	int w = quilt.width();
			// 	int h = fmin(p2.bottom - p2.top, sw);
			// 	for (int i = 0; i < w; ++i) {
			// 		for (int j = 0; j < h; ++j) {
			// 			if (p1.botSeam[i][j]) {
			// 				for (int c = 0; c < spectrum; c++) {
			// 					quilt(i, y1 - sw + j, c) = quilt(i, p1.bottom - sw + j, c);
			// 				}
			// 			} else {
			// 				for (int c = 0; c < spectrum; c++) {
			// 					quilt(i, y1 - sw + j, c) = quilt(i, p2.top + j, c);
			// 				}
			// 			}
			// 		}
			// 	}
			// 	for (int i = 0; i < quilt.width(); ++i) {
			// 		for (int j = 0; y1 + h + j < y2; ++j) {
			// 			for (int c = 0; c < spectrum; c++) {
			// 				quilt(i, y1 + j, c) = quilt(i, p2.top + h + j, c);
			// 			}
			// 		}
			// 	}
			// 	// quilt.display();
			// 	if (y2 == quilt.height()) {
			// 		quilt.crop(0, 0, 0, 0, quilt.width(), quilt.height() - h, 1, spectrum);
			// 	}
			// }

		}
	}
	return true;
}

int main(int argc, char *argv[]) {
	char *fName = argv[1];
	patchW = 7;
	patchH = 7;
	ss = 70;
	if (argc > 1) {
		loadImg(fName);
	} else {
		printf("You must put in a file name.\n");
		return 0;
	}
	srand (time(NULL));
	sw = 15;
	getPatches();

	// Patch p(0, 0, 60, 60);
	// Patch p2(0, 60, 60, 120);
	// p.compBottom(p2);
	// p.findBottomSeam();

	quilt.display();

	return 0;
}
