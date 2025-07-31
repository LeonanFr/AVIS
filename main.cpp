#include <iostream>
#include <thread>

#include "AudioCapturer.h"
#include "OverlayWindow.h"


int main()
{
	std::atomic g_direction = 0;

	std::thread overlayThread(RunOverlay, std::ref(g_direction));

	try
	{
		AudioCapturer capturer(g_direction);
		capturer.run();
	} catch(const std::exception& e)
	{
		std::cerr << "Erro: " << e.what() << '\n';
	}

	// Properly join the overlay thread before exiting
	if (overlayThread.joinable()) {
		overlayThread.join();
	}

	return 0;
}
