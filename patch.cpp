#include "patch.h"
#include "CImg.h"
#include <vector>

Patch::Patch(int t, int l, int b, int r) {
	top = t;
	left = l;
	bottom = b;
	right = r;
}

int Patch::compRight(Patch& right) {
	return 0;
}