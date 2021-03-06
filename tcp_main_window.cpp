#include <stdio.h>
#include <GLFW/glfw3.h>

#include "decoder.h"
extern "C" {
	#include "tcp.h"
	#include <libgen.h>
}

int main(int argc, const char** argv) {

	if (argc < 2)
	{
		printf("Usage: ./client filename\n");
		return -1;
	}

	if (!glfwInit()) {
        err_log("Couldn't init GLFW");
        return 1;
    }

	GLFWwindow *window;
	char *path = strdupa(argv[1]);
	const char *filename = basename(path);

	// Should start download before preparing the window.
	// TODO: Synchronise the download and playback.
	run_tcp_client(path, filename);

    decoder_t dec;
    if (!quicsy_decoder_open(&dec, filename)) {
        err_log("Main Window: Couldn't open video file");
        return 1;
    }

    window = glfwCreateWindow(dec.width, dec.height, "QUIC streamer", NULL, NULL);
    if (!window) {
        err_log("Couldn't open window");
        return 1;
    }

	log("opening the input file (%s) and loading format (container) header", argv[1]);

    glfwMakeContextCurrent(window);

// Generate texture
    GLuint tex_handle;
    glGenTextures(1, &tex_handle);
    glBindTexture(GL_TEXTURE_2D, tex_handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Allocate frame buffer
    const int frame_width = dec.width;
    const int frame_height = dec.height;
	uint8_t* frame_data = new uint8_t[frame_width * frame_height * 4];

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set up orphographic projection
        int window_width, window_height;
        glfwGetFramebufferSize(window, &window_width, &window_height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, window_width, window_height, 1, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        // Read a new frame and load it into texture
        int64_t pts;
        if (!quicsy_decoder_read_frame(&dec, frame_data, &pts)) {
            err_log("Couldn't load video frame\n");
            return 1;
        }

        static bool first_frame = true;
        if (first_frame) {
            glfwSetTime(0.0);
            first_frame = false;
        }

        double pt_in_seconds = pts * (double)dec.time_base.num / (double)dec.time_base.den;
        while (pt_in_seconds > glfwGetTime()) {
            glfwWaitEventsTimeout(pt_in_seconds - glfwGetTime());
        }

        glBindTexture(GL_TEXTURE_2D, tex_handle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame_width, frame_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame_data);

        // Render whatever you want
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex_handle);
        glBegin(GL_QUADS);
		glTexCoord2d(0, 0);
		glVertex2i(0,0);
		glTexCoord2d(1, 0);
		glVertex2i(window_width, 0);
		glTexCoord2d(1, 1);
		glVertex2i(window_width, window_height);
		glTexCoord2d(0, 1);
		glVertex2i(0, window_height);
        glEnd();
        glDisable(GL_TEXTURE_2D);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    quicsy_decoder_close(&dec);

	if (remove(filename) < 0)
	{
		printf("Unable to delete file");
	}

	return 0;
}