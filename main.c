#include <stdio.h>

#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "object.h"
#include "triangle.h"
#include "torus.h"

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

typedef struct Object* (*ConstructorT)();

struct ObjectAndConstructor {
    const char name[256];
    ConstructorT constructor;
};

int main(int argc, char** argv)
{
    GLFWwindow* window;
    struct Object* obj;
    struct DrawContext ctx;
    struct ObjectAndConstructor constuctors[] = {
        {"torus", CreateTorus},
        {"triangle", CreateTriangle},
        {"", NULL}
    };
    int i, j;
    ConstructorT constr = CreateTorus;

    for (i = 1; i < argc; i++) {
        for (j = 0; constuctors[j].constructor; j++) {
            if (!strcmp(constuctors[j].name, argv[i])) {
                constr = constuctors[j].constructor;
                break;
            }
        }
    }

    memset(&ctx, 0, sizeof(ctx));

    glfwSetErrorCallback(error_callback);

    /* Initialize the library */
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwSetKeyCallback(window, key_callback);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);

    obj = constr();

    const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

    glEnable(GL_DEPTH_TEST);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        ctx.ratio = width / (float) height;

        /* Render here */
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        obj->draw(obj, &ctx);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    obj->free(obj);

    glfwTerminate();
    return 0;
}
