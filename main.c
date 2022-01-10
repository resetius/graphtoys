#include <stdio.h>
#include <string.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <render/render.h>

#include <font/font.h>

#include <lib/object.h>
#include <lib/config.h>

#include <models/triangle.h>
#include <models/torus.h>
#include <models/mandelbrot.h>
#include <models/mandelbulb.h>
#include <models/stl.h>

struct App {
    struct DrawContext ctx;
    struct ObjectVec objs;
    struct Render* r;
};

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void resize_callback(GLFWwindow* window, int w, int h)
{
    struct App* app = glfwGetWindowUserPointer(window);
    app->ctx.w = w;
    app->ctx.h = h;
    app->ctx.ratio = w/(float)h;
    app->r->set_viewport(app->r, w, h);
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
                app->objs.objs[i]->move_left(app->objs.objs[i], mods);
            }
        }
    } else if (key == GLFW_KEY_RIGHT && action != GLFW_RELEASE) {
        for (i = 0; i < app->objs.size; i++) {
            if (app->objs.objs[i]->move_right) {
                app->objs.objs[i]->move_right(app->objs.objs[i], mods);
            }
        }
    } else if (key == GLFW_KEY_UP && action != GLFW_RELEASE) {
        for (i = 0; i < app->objs.size; i++) {
            if (app->objs.objs[i]->move_up) {
                app->objs.objs[i]->move_up(app->objs.objs[i], mods);
            }
        }
    } else if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE) {
        for (i = 0; i < app->objs.size; i++) {
            if (app->objs.objs[i]->move_down) {
                app->objs.objs[i]->move_down(app->objs.objs[i], mods);
            }
        }
    } else if (key == GLFW_KEY_A && action != GLFW_RELEASE) {
        for (i = 0; i < app->objs.size; i++) {
            if (app->objs.objs[i]->zoom_in) {
                app->objs.objs[i]->zoom_in(app->objs.objs[i], mods);
            }
        }
    } else if (key == GLFW_KEY_Z && action != GLFW_RELEASE) {
        for (i = 0; i < app->objs.size; i++) {
            if (app->objs.objs[i]->zoom_out) {
                app->objs.objs[i]->zoom_out(app->objs.objs[i], mods);
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

typedef struct Object* (*ConstructorT)(struct Render*);

struct ObjectAndConstructor {
    const char* name;
    ConstructorT constructor;
};

static void fill_render_config(struct RenderConfig* r, struct Config* cfg) {
    const char* vsync;
    r->api = cfg_gets_def(cfg, "render:api", "opengl");
    printf("api: '%s'\n", r->api);
    vsync = cfg_gets_def(cfg, "render:vsync", "on");
    printf("vsync: %s\n", vsync);
    if (!strcmp(vsync, "on")) {
        r->vsync = 1;
    } else {
        r->vsync = 0;
    }
    printf("vsync: %d\n", r->vsync);
}

int main(int argc, char** argv)
{
    GLFWwindow* window = NULL;
    struct App app;
    struct Font* font = NULL;
    struct Label* fps = NULL;
    struct Label* text = NULL;
    struct Render* render = NULL;
    struct Config* cfg = NULL;
    struct RenderConfig rcfg;
    float t1, t2;
    long long frames = 0;
    int enable_labels = 1;
    struct ObjectAndConstructor constructors[] = {
        {"torus", CreateTorus},
        {"triangle", CreateTriangle},
        {"mandelbrot", CreateMandelbrot},
        {"mandelbulb", CreateMandelbulb},
        {"stl", CreateStl},
        {"test", NULL},
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
        } else {
            for (j = 0; constructors[j].name; j++) {
                if (!strcmp(constructors[j].name, argv[i])) {
                    constr = constructors[j].constructor;
                    break;
                }
            }
        }
    }

    glfwSetErrorCallback(error_callback);

    /* Initialize the library */
    if (!glfwInit()) {
        return -1;
    }

    cfg = cfg_new("main.ini", argc, argv);
    cfg_print(cfg);
    fill_render_config(&rcfg, cfg);

    if (!strcmp(rcfg.api, "opengl")) {
        render = rend_opengl_new(rcfg);
    } else if (!strcmp(rcfg.api, "vulkan")) {
        render = rend_vulkan_new(rcfg);
    } else {
        fprintf(stderr, "Unknown render: '%s'\n", rcfg.api);
        return -1;
    }

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwSetWindowUserPointer(window, &app);
    glfwSetKeyCallback(window, key_callback);
    //glfwSetWindowSizeCallback(window, resize_callback);
    glfwSetFramebufferSizeCallback(window, resize_callback);
    glfwGetFramebufferSize(window, &app.ctx.w, &app.ctx.h);
    app.ctx.ratio = app.ctx.w/(float)app.ctx.h;

    /* Make the window's context current */

    render->set_view_entity(render, window);

    render->init(render);

    app.r = render;

    setbuf(stdout, NULL);

    if (constr == NULL) {
        ovec_add(&app.objs, CreateTriangle(render));
        ovec_add(&app.objs, CreateTorus(render));
    } else {
        ovec_add(&app.objs, constr(render));
    }

    if (enable_labels) {
        font = font_new(render);
        fps = label_new(font);
        label_set_pos(fps, 100, 100);
        label_set_text(fps, "FPS:");


        text = label_new(font);
        label_set_pos(text, 100, 200);
        //label_set_text(text, "Проверка русских букв");
    }

    t1 = glfwGetTime();

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        render->draw_begin(render);

        app.ctx.time = (float)glfwGetTime();

        /* Render here */
        if (enable_labels) {
            label_set_screen(fps, app.ctx.w, app.ctx.h);
            label_set_screen(text, app.ctx.w, app.ctx.h);
        }

        for (i = 0; i < app.objs.size; i++) {
            app.objs.objs[i]->draw(app.objs.objs[i], &app.ctx);
        }

        if (enable_labels) {
            label_render(fps);
            //label_render(text);
        }

        /* Swap front and back buffers */
        render->draw_end(render);

        /* Poll for and process events */
        glfwPollEvents();
        t2 = glfwGetTime();
        if (t2 - t1 > 1.0) {
            if (enable_labels) {
                label_set_vtext(fps, "FPS:%.2f", frames/(t2-t1));
            }
            printf("FPS:%.2f\n", frames/(t2-t1));
            frames = 0;
            t1 = t2;
        }
        frames ++;
        //break;
    }

    for (i = 0; i < app.objs.size; i++) {
        app.objs.objs[i]->free(app.objs.objs[i]);
    }

    label_free(fps);
    label_free(text);
    font_free(font);
    rend_free(render);
    cfg_free(cfg);
    glfwTerminate();
    return 0;
}
