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

static bool gPaused = false;                         // T — сахнаның айналуын тоқтату
static bool gKeyHeld[GLFW_KEY_LAST + 1] = { false };

// ---------- 9-апта: камера (10-аптада осыған yaw мен pitch қосылады) ----------
struct Camera {
    glm::vec3 pos   = glm::vec3(0.0f, 0.0f, 3.0f);    // камера орны
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);   // қарайтын бағыт
    glm::vec3 up    = glm::vec3(0.0f, 1.0f, 0.0f);    // жоғары бағыт
};
Camera gCamera;

const float CAM_SPEED     = 2.5f;    // бірлік/сек
const float CAM_MAX_RANGE = 20.0f;   // камера сахнадан алыс кетпесін

// ---------- 7-8 апта: проекция және тексеру режимдері ----------
struct ViewParams {
    float fovDeg    = 45.0f;
    float nearPlane = 0.1f;
    float farPlane  = 100.0f;
    bool  ortho     = false;    // O
    bool  wireframe = false;    // Tab
    bool  depthOn   = true;     // 1
    bool  cullOn    = true;     // 2
    bool  cullFront = false;    // 3
    bool  zFightDemo = false;   // 6
};
ViewParams gView;

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

// Тінтуір дөңгелегі: fov 10°–90°
void handleScroll(GLFWwindow*, double, double dy) {
    gView.fovDeg = std::clamp(gView.fovDeg - (float)dy * 2.0f, 10.0f, 90.0f);
}

void applyGlState() {
    if (gView.depthOn) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (gView.cullOn)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
    glCullFace(gView.cullFront ? GL_FRONT : GL_BACK);
    glPolygonMode(GL_FRONT_AND_BACK, gView.wireframe ? GL_LINE : GL_FILL);
}

// Камераны пернетақтамен жылжыту. dt-ге көбейтілгендіктен жылдамдық FPS-ке тәуелсіз
void moveCamera(GLFWwindow* w, float dt) {
    float step = CAM_SPEED * dt;
    if (glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) step *= 2.0f;   // жүгіру

    // Оң жақ бағыт: cross(front, up). normalize болмаса A/D жылдамдығы қисаюға тәуелді болады
    glm::vec3 right = glm::normalize(glm::cross(gCamera.front, gCamera.up));

    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) gCamera.pos += step * gCamera.front;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) gCamera.pos -= step * gCamera.front;
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) gCamera.pos -= right * step;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) gCamera.pos += right * step;

    // Жоғары-төмен: әлемнің Y осі бойымен (ойындағы адам сияқты)
    if (glfwGetKey(w, GLFW_KEY_SPACE)      == GLFW_PRESS) gCamera.pos += step * glm::vec3(0.0f, 1.0f, 0.0f);
    if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) gCamera.pos -= step * glm::vec3(0.0f, 1.0f, 0.0f);

    // Қосымша: камера орнынан 20 бірліктен аспасын
    float dist = glm::length(gCamera.pos);
    if (dist > CAM_MAX_RANGE) gCamera.pos = gCamera.pos / dist * CAM_MAX_RANGE;
}

void readInput(GLFWwindow* w) {
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(w, true);

    if (keyTapped(w, GLFW_KEY_T)) gPaused = !gPaused;

    // Қосымша: R — бастапқы орынға қайту, P — координатты шығару
    if (keyTapped(w, GLFW_KEY_R)) gCamera = Camera();
    if (keyTapped(w, GLFW_KEY_P)) {
        std::cout << "camPos: " << gCamera.pos.x << ", "
                  << gCamera.pos.y << ", " << gCamera.pos.z << "\n";
    }

    if (keyTapped(w, GLFW_KEY_O)) {
        gView.ortho = !gView.ortho;
        std::cout << (gView.ortho ? "Rezhim: ortho\n" : "Rezhim: perspective\n");
    }
    if (keyTapped(w, GLFW_KEY_TAB)) gView.wireframe = !gView.wireframe;

    // 8-аптаның тексеру пернелері енді сандар қатарында (WASD-пен шатаспауы үшін)
    if (keyTapped(w, GLFW_KEY_1)) {
        gView.depthOn = !gView.depthOn;
        std::cout << "Depth test: " << (gView.depthOn ? "ON" : "OFF") << "\n";
    }
    if (keyTapped(w, GLFW_KEY_2)) {
        gView.cullOn = !gView.cullOn;
        std::cout << "Culling: " << (gView.cullOn ? "ON" : "OFF") << "\n";
    }
    if (keyTapped(w, GLFW_KEY_3)) {
        gView.cullFront = !gView.cullFront;
        std::cout << "CullFace: " << (gView.cullFront ? "GL_FRONT" : "GL_BACK") << "\n";
    }
    if (keyTapped(w, GLFW_KEY_4)) {
        gView.nearPlane = (gView.nearPlane < 0.01f) ? 0.1f : 0.0001f;
        std::cout << "near = " << gView.nearPlane << "\n";
    }
    if (keyTapped(w, GLFW_KEY_5)) {
        gView.farPlane = (gView.farPlane > 500.0f) ? 100.0f : 1000.0f;
        std::cout << "far = " << gView.farPlane << "\n";
    }
    if (keyTapped(w, GLFW_KEY_6)) {
        gView.zFightDemo = !gView.zFightDemo;
        std::cout << "Z-fight demo: " << (gView.zFightDemo ? "ON" : "OFF") << "\n";
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
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* win = glfwCreateWindow(gWidth, gHeight, "Week 9 - camera, WASD", nullptr, nullptr);
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
    std::cout << "Kamera: WASD - zhurý, Space/Shift - zhogary/tomen, Ctrl - zhugiru, R - bastapqy, P - koordinat\n";
    std::cout << "Basqa: T - pauza, O - ortho, Tab - wireframe, scroll - fov, 1..6 - 8-apta tekseru, ESC - shygu\n";

    int depthBits = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH,
                                          GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depthBits);
    std::cout << "depth bits: " << depthBits << "\n";

    // ---------- КУБ: 36 вершина, барлық қыр сыртынан қарағанда CCW ----------
    const float cubeData[] = {
        // артқы қыр (қызыл), -Z
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

    float clock = 0.0f;                 // кубтардың айналу уақыты (T-мен тоқтайды)
    float prevTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(win)) {
        float nowT = (float)glfwGetTime();
        float dt = nowT - prevTime;
        prevTime = nowT;

        readInput(win);
        moveCamera(win, dt);            // dt есептелгеннен кейін
        if (!gPaused) clock += dt;

        glClearColor(0.07f, 0.08f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog);

        float aspect = (gHeight > 0) ? (float)gWidth / (float)gHeight : 1.0f;

        glm::mat4 projection;
        if (gView.ortho) {
            projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f,
                                    gView.nearPlane, gView.farPlane);
        } else {
            projection = glm::perspective(glm::radians(gView.fovDeg), aspect,
                                          gView.nearPlane, gView.farPlane);
        }

        // Мақсат нүктесі камерамен бірге жылжиды: pos + front (тұрақты нүкте емес)
        glm::mat4 view = glm::lookAt(gCamera.pos,
                                     gCamera.pos + gCamera.front,
                                     gCamera.up);

        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(vao);

        if (!gView.zFightDemo) {
            for (int i = 0; i < CUBE_COUNT; i++) {
                glm::mat4 model(1.0f);
                model = glm::translate(model, cubePos[i]);
                model = glm::rotate(model, clock * (0.5f + i * 0.3f),
                                    glm::vec3(0.3f, 1.0f, 0.0f));
                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, VERTEX_COUNT);
            }
        } else {
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