#include "Server.hpp"


int main(int argc, char** argv) {
	srand((unsigned int)time(nullptr));

	Server MainServer;
	MainServer.Run();

	return 0;
}
