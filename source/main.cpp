#include "state.h"


int main()
{
	state state = {};
	while (state.process()) {
		state.on_update();
	}
}
