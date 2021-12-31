#include <stdio.h>
#include <string.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "glad/gl.h"

#include <render/render.h>

#include "font/font.h"

#include "object.h"
#include "triangle.h"
#include "torus.h"
#include "mandelbrot.h"
#include "mandelbulb.h"

struct App {
    struct DrawContext ctx;
    struct ObjectVec objs;
};

static void glInfo() {
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    struct App* app = glfwGetWindowUserPointer(window);
    int i;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (key == GLFW_KEY_LEFT && action != GLFW_RELEASE) {
        for (i = 0; i < app->objs.size; i++) {
            if (app->objs.objs[i]->move_left) {
                app->objs.objs[i]->move_left(app->objs.objs[i]);
            }
        }
    } else if (key == GLFW_KEY_RIGHT && action != GLFW_RELEASE) {
        for (i = 0; i < app->objs.size; i++) {
            if (app->objs.objs[i]->move_right) {
                app->objs.objs[i]->move_right(app->objs.objs[i]);
            }
        }
    } else if (key == GLFW_KEY_UP && action != GLFW_RELEASE) {
        for (i = 0; i < app->objs.size; i++) {
            if (app->objs.objs[i]->move_up) {
                app->objs.objs[i]->move_up(app->objs.objs[i]);
            }
        }
    } else if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE) {
        for (i = 0; i < app->objs.size; i++) {
            if (app->objs.objs[i]->move_down) {
                app->objs.objs[i]->move_down(app->objs.objs[i]);
            }
        }
    } else if (key == GLFW_KEY_A && action != GLFW_RELEASE) {
        for (i = 0; i < app->objs.size; i++) {
            if (app->objs.objs[i]->zoom_in) {
                app->objs.objs[i]->zoom_in(app->objs.objs[i]);
            }
        }
    } else if (key == GLFW_KEY_Z && action != GLFW_RELEASE) {
        for (i = 0; i < app->objs.size; i++) {
            if (app->objs.objs[i]->zoom_out) {
                app->objs.objs[i]->zoom_out(app->objs.objs[i]);
            }
        }
    } else if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        for (i = 0; i < app->objs.size; i++) {
            if (app->objs.objs[i]->mode) {
                app->objs.objs[i]->mode(app->objs.objs[i]);
            }
        }
    }
    //printf("%d %d %d\n", key, action, mods);
}

typedef struct Object* (*ConstructorT)();

struct ObjectAndConstructor {
    const char* name;
    ConstructorT constructor;
};

int main(int argc, char** argv)
{
    GLFWwindow* window;
    struct Object* obj;
    struct App app;
    struct Font* font;
    struct Label* fps;
    struct Label* text;
    struct Render* render;
    float t1, t2;
    long long frames = 0;
    struct ObjectAndConstructor constructors[] = {
        {"torus", CreateTorus},
        {"triangle", CreateTriangle},
        {"mandelbrot", CreateMandelbrot},
        {"mandelbulb", CreateMandelbulb},
        {NULL, NULL}
    };
    int i, j;
    ConstructorT constr = CreateTorus;
    memset(&app, 0, sizeof(app));
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            for (j = 0; constructors[j].name; j++) {
                printf("%s %s\n", argv[0], constructors[j].name);
            }
            return 0;
        }
        for (j = 0; constructors[j].name; j++) {
            if (!strcmp(constructors[j].name, argv[i])) {
                constr = constructors[j].constructor;
                break;
            }
        }
    }

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
    glfwSetWindowUserPointer(window, &app);
    glfwSetKeyCallback(window, key_callback);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);

    setbuf(stdout, NULL);
    glInfo();

    obj = constr();

    ovec_add(&app.objs, obj);

    render = rend_opengl_new();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

    font = font_new(render);
    fps = label_new(font);
    label_set_pos(fps, 100, 100);
    label_set_text(fps, "FPS:");

    text = label_new(font);
    label_set_pos(text, 100, 200);
    label_set_text(text, "Проверка русских букв");

    t1 = glfwGetTime();

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        app.ctx.ratio = width / (float) height;

        /* Render here */
        glViewport(0, 0, width, height);
        label_set_screen(fps, width, height);
        label_set_screen(text, width, height);

        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        obj->draw(obj, &app.ctx);

        glDisable(GL_DEPTH_TEST);
        label_render(fps);
        label_render(text);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
        t2 = glfwGetTime();
        if (t2 - t1 > 1.0) {
            label_set_vtext(fps, "FPS:%.2f", frames/(t2-t1));
            frames = 0;
            t1 = t2;
        }
        frames ++;
    }

    obj->free(obj);
    label_free(fps);
    label_free(text);
    font_free(font);
    rend_free(render);
    glfwTerminate();
    return 0;
}
