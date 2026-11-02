#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

// ---------- терезе өлшемі (resize кезінде екеуі де жаңарады) ----------
int gWidth  = 1280;
int gHeight = 720;

static bool gPaused = false;                         // Space — пауза
static bool gKeyHeld[GLFW_KEY_LAST + 1] = { false };

// ---------- камера және 8-аптаның тексеру режимдері ----------
struct CamParams {
    float fovDeg    = 45.0f;
    float nearPlane = 0.1f;     // 2-тапсырма: 0.0001 қойып Z-fighting жасаймыз
    float farPlane  = 100.0f;   // 2-тапсырма: 1000 / 100 салыстыру
    bool  ortho     = false;    // P: perspective <-> ortho
    bool  wireframe = false;    // Tab: wireframe
    bool  depthOn   = true;     // D: тереңдік тесті қосу/өшіру (1-тапсырма скриншоты үшін)
    bool  cullOn    = true;     // C: culling қосу/өшіру
    bool  cullFront = false;    // G: GL_FRONT/GL_BACK (3-тапсырма диагностикасы)
    bool  zFightDemo = false;   // Z: екі куб бетпе-бет (2-тапсырма)
};
CamParams gCam;

bool keyTapped(GLFWwindow* w, int key) {
    bool down = glfwGetKey(w, key) == GLFW_PRESS;
    bool tap  = down && !gKeyHeld[key];
    gKeyHeld[key] = down;
    return tap;
}

void handleResize(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
    gWidth  = w;
    gHeight = h;
}

// Тінтуір дөңгелегі: fov 10°–90° (қосымша)
void handleScroll(GLFWwindow*, double, double dy) {
    gCam.fovDeg = std::clamp(gCam.fovDeg - (float)dy * 2.0f, 10.0f, 90.0f);
}

// Күйді GL-ға қолданады: пернеден кейін шақырылады
void applyGlState() {
    if (gCam.depthOn) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (gCam.cullOn)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
    glCullFace(gCam.cullFront ? GL_FRONT : GL_BACK);
    glPolygonMode(GL_FRONT_AND_BACK, gCam.wireframe ? GL_LINE : GL_FILL);
}

void readInput(GLFWwindow* w) {
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(w, true);

    if (keyTapped(w, GLFW_KEY_SPACE)) gPaused = !gPaused;

    if (keyTapped(w, GLFW_KEY_P)) {
        gCam.ortho = !gCam.ortho;
        std::cout << (gCam.ortho ? "Rezhim: ortho\n" : "Rezhim: perspective\n");
    }
    if (keyTapped(w, GLFW_KEY_TAB)) gCam.wireframe = !gCam.wireframe;

    if (keyTapped(w, GLFW_KEY_D)) {
        gCam.depthOn = !gCam.depthOn;
        std::cout << "Depth test: " << (gCam.depthOn ? "ON" : "OFF") << "\n";
    }
    if (keyTapped(w, GLFW_KEY_C)) {
        gCam.cullOn = !gCam.cullOn;
        std::cout << "Culling: " << (gCam.cullOn ? "ON" : "OFF") << "\n";
    }
    if (keyTapped(w, GLFW_KEY_G)) {
        gCam.cullFront = !gCam.cullFront;
        std::cout << "CullFace: " << (gCam.cullFront ? "GL_FRONT" : "GL_BACK") << "\n";
    }
    if (keyTapped(w, GLFW_KEY_N)) {
        gCam.nearPlane = (gCam.nearPlane < 0.01f) ? 0.1f : 0.0001f;
        std::cout << "near = " << gCam.nearPlane << "\n";
    }
    if (keyTapped(w, GLFW_KEY_F)) {
        gCam.farPlane = (gCam.farPlane > 500.0f) ? 100.0f : 1000.0f;
        std::cout << "far = " << gCam.farPlane << "\n";
    }
    if (keyTapped(w, GLFW_KEY_Z)) {
        gCam.zFightDemo = !gCam.zFightDemo;
        std::cout << "Z-fight demo: " << (gCam.zFightDemo ? "ON" : "OFF") << "\n";
    }
    applyGlState();
}

// ---------- шейдер көмекшілері ----------
unsigned int compileStage(GLenum type, const char* src) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    int status = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
    if (!status) {
        char msg[1024];
        glGetShaderInfoLog(id, 1024, nullptr, msg);
        std::cerr << "Shader qate: " << msg << "\n";
    }
    return id;
}

unsigned int buildProgram(const char* vsSrc, const char* fsSrc) {
    unsigned int vs = compileStage(GL_VERTEX_SHADER, vsSrc);
    unsigned int fs = compileStage(GL_FRAGMENT_SHADER, fsSrc);

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    int status = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status) {
        char msg[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, msg);
        std::cerr << "Link qate: " << msg << "\n";
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

const char* VS_CODE = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vColor;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* FS_CODE = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() { FragColor = vec4(vColor, 1.0); }
)";

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW qosylmady\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);   // тереңдік буфері болуы үшін
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* win = glfwCreateWindow(gWidth, gHeight, "Week 8 - depth, culling", nullptr, nullptr);
    if (!win) {
        std::cerr << "Tereze ashylmady\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, handleResize);
    glfwSetScrollCallback(win, handleScroll);
    glfwSwapInterval(1);

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD qate\n";
        glfwTerminate();
        return -1;
    }

    glfwGetFramebufferSize(win, &gWidth, &gHeight);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "Basqaru: Space - pauza, P - ortho, Tab - wireframe, scroll - fov, ESC - shygu\n";
    std::cout << "Tekseru: D - depth, C - culling, G - cull FRONT/BACK, N - near, F - far, Z - z-fight demo\n";

    // Тереңдік буфері бар екенін тексереміз (24 немесе 32 болуы керек)
    int depthBits = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH,
                                          GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depthBits);
    std::cout << "depth bits: " << depthBits << "\n";

    // ---------- КУБ: 36 вершина, барлық қыр сыртынан қарағанда сағат тіліне қарсы (CCW) ----------
    // Culling жұмыс істеуі үшін әр үшбұрыштың реті сыртқы жағынан CCW болуы шарт
    const float cubeData[] = {
        // артқы қыр (қызыл), сыртқы жағы -Z
        -0.5f, -0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
        // алдыңғы қыр (жасыл), +Z
        -0.5f, -0.5f,  0.5f,  0.2f, 0.9f, 0.3f,
         0.5f, -0.5f,  0.5f,  0.2f, 0.9f, 0.3f,
         0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.3f,
         0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.3f,
        -0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.3f,
        -0.5f, -0.5f,  0.5f,  0.2f, 0.9f, 0.3f,
        // сол жақ қыр (көк), -X
        -0.5f,  0.5f,  0.5f,  0.2f, 0.4f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.2f, 0.4f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.2f, 0.4f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.2f, 0.4f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.2f, 0.4f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.2f, 0.4f, 1.0f,
        // оң жақ қыр (сары), +X
         0.5f,  0.5f,  0.5f,  1.0f, 0.9f, 0.2f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.9f, 0.2f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.9f, 0.2f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.9f, 0.2f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.9f, 0.2f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.9f, 0.2f,
        // төменгі қыр (қызғылт), -Y
        -0.5f, -0.5f, -0.5f,  0.9f, 0.3f, 0.8f,
         0.5f, -0.5f, -0.5f,  0.9f, 0.3f, 0.8f,
         0.5f, -0.5f,  0.5f,  0.9f, 0.3f, 0.8f,
         0.5f, -0.5f,  0.5f,  0.9f, 0.3f, 0.8f,
        -0.5f, -0.5f,  0.5f,  0.9f, 0.3f, 0.8f,
        -0.5f, -0.5f, -0.5f,  0.9f, 0.3f, 0.8f,
        // жоғарғы қыр (көгілдір), +Y
        -0.5f,  0.5f, -0.5f,  0.2f, 0.9f, 0.9f,
         0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.9f,
         0.5f,  0.5f, -0.5f,  0.2f, 0.9f, 0.9f,
         0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.9f,
        -0.5f,  0.5f, -0.5f,  0.2f, 0.9f, 0.9f,
        -0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.9f
    };
    const int VERTEX_COUNT = 36;

    unsigned int vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeData), cubeData, GL_STATIC_DRAW);

    const GLsizei stride = 6 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    unsigned int prog = buildProgram(VS_CODE, FS_CODE);

    int locModel = glGetUniformLocation(prog, "uModel");
    int locView  = glGetUniformLocation(prog, "uView");
    int locProj  = glGetUniformLocation(prog, "uProjection");

    // ---------- 8-апта: тереңдік тесті мен culling — циклге дейін, бір рет ----------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // ---------- бес куб ----------
    const int CUBE_COUNT = 5;
    const glm::vec3 cubePos[CUBE_COUNT] = {
        glm::vec3( 0.0f,  0.0f,  0.0f),
        glm::vec3( 2.0f,  0.5f, -2.0f),
        glm::vec3(-1.8f, -0.3f, -1.0f),
        glm::vec3( 1.2f, -0.8f, -3.5f),
        glm::vec3(-2.2f,  1.0f, -4.0f)
    };

    float clock = 0.0f;
    float prevTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(win)) {
        float nowT = (float)glfwGetTime();
        float dt = nowT - prevTime;
        prevTime = nowT;

        readInput(win);
        if (!gPaused) clock += dt;

        glClearColor(0.07f, 0.08f, 0.13f, 1.0f);
        // Тереңдік буферін де әр кадрда тазалау ШАРТ
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog);

        float aspect = (gHeight > 0) ? (float)gWidth / (float)gHeight : 1.0f;

        glm::mat4 projection;
        if (gCam.ortho) {
            projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f,
                                    gCam.nearPlane, gCam.farPlane);
        } else {
            projection = glm::perspective(glm::radians(gCam.fovDeg), aspect,
                                          gCam.nearPlane, gCam.farPlane);
        }

        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                                     glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(vao);

        if (!gCam.zFightDemo) {
            for (int i = 0; i < CUBE_COUNT; i++) {
                // model әр итерацияда жаңадан жарияланады, әйтпесе түрлендіру үстемеленеді
                glm::mat4 model(1.0f);
                model = glm::translate(model, cubePos[i]);
                model = glm::rotate(model, clock * (0.5f + i * 0.3f),
                                    glm::vec3(0.3f, 1.0f, 0.0f));
                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, VERTEX_COUNT);
            }
        } else {
            // 2-тапсырма: екі куб дәл бетпе-бет. Екінші куб Y осінен 180° бұрылған және
            // z бойынша 0.001 ғана ығысқан: оның қызыл беті бірінші кубтың жасыл бетімен беттеседі.
            glm::mat4 modelA(1.0f);
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelA));
            glDrawArrays(GL_TRIANGLES, 0, VERTEX_COUNT);

            glm::mat4 modelB(1.0f);
            modelB = glm::translate(modelB, glm::vec3(0.0f, 0.0f, 0.001f));
            modelB = glm::rotate(modelB, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelB));
            glDrawArrays(GL_TRIANGLES, 0, VERTEX_COUNT);
        }

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(prog);

    glfwTerminate();
    return 0;
}