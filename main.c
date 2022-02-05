#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <render/render.h>

#include <font/font.h>

#include <lib/object.h>
#include <lib/config.h>
#include <lib/event.h>

#include <models/triangle.h>
#include <models/torus.h>
#include <models/mandelbrot.h>
#include <models/mandelbulb.h>
#include <models/stl.h>
#include <models/particles.h>
#include <models/particles2.h>

struct Object* CreateGltf(struct Render* r, struct Config* cfg);

struct App {
    struct EventProducer events;
    struct DrawContext ctx;
    struct ObjectVec objs;
    struct Render* r;

    struct EventConsumer** cons;
    int n_cons;
    int cap_cons;
    unsigned char key_mask[1000];
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

static void subscribe(struct EventProducer* prod, struct EventConsumer* cons) {
    struct App* app = (struct App*)prod;
    if (app->n_cons >= app->cap_cons) {
        app->cap_cons = (1+app->cap_cons)*2;
        app->cons = realloc(app->cons, app->cap_cons*sizeof(struct EventConsumer*));
    }
    app->cons[app->n_cons++] = cons;
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

    if (key >= 0 && key < sizeof(app->key_mask) / sizeof(app->key_mask[0])) {
        if (action == GLFW_PRESS) {
            app->key_mask[key] = 1;
        } else if (action == GLFW_RELEASE) {
            app->key_mask[key] = 0;
        }
    }

    for (i = 0; i < app->n_cons; i++) {
        app->cons[i]->key_event(app->cons[i], key, scancode, action, mods, app->key_mask);
    }
}

typedef struct Object* (*ConstructorT)(struct Render*, struct Config* cfg, struct EventProducer*);

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
    r->show_fps = strcmp(cfg_gets_def(cfg, "render:fps", "off"), "on") == 0;
    r->triple_buffer = strcmp(cfg_gets_def(cfg, "render:triple_buffer", "off"), "on") == 0;
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
    int enable_labels = 0;
    struct ObjectAndConstructor constructors[] = {
        {"torus", (ConstructorT)CreateTorus},
        {"triangle", (ConstructorT)CreateTriangle},
        {"mandelbrot", (ConstructorT)CreateMandelbrot},
        {"mandelbulb", (ConstructorT)CreateMandelbulb},
        {"stl", (ConstructorT)CreateStl},
        {"particles", (ConstructorT)CreateParticles},
        {"particles2", (ConstructorT)CreateParticles2},
        {"gltf", (ConstructorT)CreateGltf},
        {"test", NULL},
        {NULL, NULL}
    };
    int i, j;
    ConstructorT constr = (ConstructorT)CreateTorus;
    const char* name = "torus";
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
                    name = constructors[j].name;
                    break;
                }
            }
        }
    }

    app.events.subscribe = subscribe;

    glfwSetErrorCallback(error_callback);

    /* Initialize the library */
    if (!glfwInit()) {
        return -1;
    }

    cfg = cfg_new("main.ini", argc, argv);
    cfg_print(cfg);
    fill_render_config(&rcfg, cfg);
    enable_labels = rcfg.show_fps;

    if (!strcmp(rcfg.api, "opengl")) {
        render = rend_opengl_new(rcfg);
    } else if (!strcmp(rcfg.api, "vulkan")) {
        render = rend_vulkan_new(rcfg);
    } else {
        fprintf(stderr, "Unknown render: '%s'\n", rcfg.api);
        return -1;
    }

    /* Create a windowed mode window and its OpenGL context */
    char caption[256];
    snprintf(caption, sizeof(caption), "GraphToys (%s)", rcfg.api);
    window = glfwCreateWindow(640, 480, caption, NULL, NULL);
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
        ovec_add(&app.objs, CreateTriangle(render, cfg_section(cfg, "triangle")));
        ovec_add(&app.objs, CreateTorus(render, cfg_section(cfg, "torus")));
    } else {
        ovec_add(&app.objs, constr(render, cfg_section(cfg, name), (struct EventProducer*) &app));
    }


    float scale_w, scale_h;
    glfwGetWindowContentScale(window, &scale_w, &scale_h);


    if (enable_labels) {
        font = font_new(render, 0, 16*64, 150*scale_w, 150*scale_h);
        fps = label_new(font);
        label_set_pos(fps, 10, 10);
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
        frames ++;

        if (t2 - t1 > 1.0) {
            double fp = frames/(t2-t1);
            double ms = 1000.*(t2-t1)/frames;
            if (enable_labels) {
                label_set_vtext(fps, "FPS:%.2f, %.1fms", fp, ms);
            }
            printf("FPS:%.2f, %.1fms\n", fp, ms);
            frames = 0;
            t1 = t2;
        }
    }

    ovec_free(&app.objs);
    label_free(fps);
    label_free(text);
    font_free(font);
    rend_free(render);
    cfg_free(cfg);
    glfwTerminate();
    return 0;
}
