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

// ---------- 1-тапсырма: параметрлерді осы жерден өзгертіп байқайсың ----------
struct CamParams {
    float fovDeg   = 45.0f;    // 30, 45, 90, 120
    float nearPlane = 0.1f;    // 0.1, 1.0, 2.5, 4.0
    float farPlane  = 100.0f;  // 100.0, 5.0, 2.0
    bool  ortho     = false;   // P пернесі: perspective <-> ortho (2-тапсырма)
    bool  wireframe = false;   // Tab пернесі (қосымша)
};
CamParams gCam;

// Бір рет басылғанда ғана true
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

// Тінтуір дөңгелегі: fov 10°–90° аралығында (қосымша)
void handleScroll(GLFWwindow*, double, double dy) {
    gCam.fovDeg = std::clamp(gCam.fovDeg - (float)dy * 2.0f, 10.0f, 90.0f);
}

void readInput(GLFWwindow* w) {
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(w, true);

    if (keyTapped(w, GLFW_KEY_SPACE)) {
        gPaused = !gPaused;
    }
    if (keyTapped(w, GLFW_KEY_P)) {
        gCam.ortho = !gCam.ortho;
        std::cout << (gCam.ortho ? "Rezhim: ortho\n" : "Rezhim: perspective\n");
    }
    if (keyTapped(w, GLFW_KEY_TAB)) {
        gCam.wireframe = !gCam.wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, gCam.wireframe ? GL_LINE : GL_FILL);
    }
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

// Үш бөлек uniform: model -> view -> projection тәртібімен көбейтіледі
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
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* win = glfwCreateWindow(gWidth, gHeight, "Week 7 - MVP kubtar", nullptr, nullptr);
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

    // Нақты framebuffer өлшемін алып, жаһандық айнымалыларға жазамыз
    glfwGetFramebufferSize(win, &gWidth, &gHeight);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "Basqaru: Space - pauza, P - ortho/perspective, Tab - wireframe, scroll - fov, ESC - shygu\n";

    // ---------- КУБ: 36 вершина, әрқайсысы 6 float (позиция + түс) ----------
    // Әр қырдың өз түсі бар, сондықтан бір вершинаны қырлар бөлісе алмайды (8 емес, 36)
    const float cubeData[] = {
        // артқы қыр (қызыл)
        -0.5f, -0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.2f, 0.2f,
        // алдыңғы қыр (жасыл)
        -0.5f, -0.5f,  0.5f,  0.2f, 0.9f, 0.3f,
         0.5f, -0.5f,  0.5f,  0.2f, 0.9f, 0.3f,
         0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.3f,
         0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.3f,
        -0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.3f,
        -0.5f, -0.5f,  0.5f,  0.2f, 0.9f, 0.3f,
        // сол жақ қыр (көк)
        -0.5f,  0.5f,  0.5f,  0.2f, 0.4f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.2f, 0.4f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.2f, 0.4f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.2f, 0.4f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.2f, 0.4f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.2f, 0.4f, 1.0f,
        // оң жақ қыр (сары)
         0.5f,  0.5f,  0.5f,  1.0f, 0.9f, 0.2f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.9f, 0.2f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.9f, 0.2f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.9f, 0.2f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.9f, 0.2f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.9f, 0.2f,
        // төменгі қыр (қызғылт)
        -0.5f, -0.5f, -0.5f,  0.9f, 0.3f, 0.8f,
         0.5f, -0.5f, -0.5f,  0.9f, 0.3f, 0.8f,
         0.5f, -0.5f,  0.5f,  0.9f, 0.3f, 0.8f,
         0.5f, -0.5f,  0.5f,  0.9f, 0.3f, 0.8f,
        -0.5f, -0.5f,  0.5f,  0.9f, 0.3f, 0.8f,
        -0.5f, -0.5f, -0.5f,  0.9f, 0.3f, 0.8f,
        // жоғарғы қыр (көгілдір)
        -0.5f,  0.5f, -0.5f,  0.2f, 0.9f, 0.9f,
         0.5f,  0.5f, -0.5f,  0.2f, 0.9f, 0.9f,
         0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.9f,
         0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.9f,
        -0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.9f,
        -0.5f,  0.5f, -0.5f,  0.2f, 0.9f, 0.9f
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

    // Uniform орындарын циклге дейін бір рет кэштейміз
    int locModel = glGetUniformLocation(prog, "uModel");
    int locView  = glGetUniformLocation(prog, "uView");
    int locProj  = glGetUniformLocation(prog, "uProjection");

    // ---------- 3-тапсырма: үш куб, әрқайсысы өз орнында және өз жылдамдығымен ----------
    const glm::vec3 cubePos[3] = {
        glm::vec3(-1.5f, 0.0f,  0.0f),
        glm::vec3( 0.0f, 0.0f, -1.0f),
        glm::vec3( 1.5f, 0.0f,  0.0f)
    };
    const float spinSpeed[3] = { 0.8f, 1.5f, 2.4f };   // радиан/сек

    float clock = 0.0f;                                // пауза кезінде тоқтайды
    float prevTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(win)) {
        float nowT = (float)glfwGetTime();
        float dt = nowT - prevTime;
        prevTime = nowT;

        readInput(win);
        if (!gPaused) clock += dt;

        glClearColor(0.07f, 0.08f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);   // тереңдік тесті 8-аптада қосылады
        glUseProgram(prog);

        // Матрицалар цикл ішінде құрылады (әйтпесе куб қозғалмайды)
        // aspect терезе өлшемінен есептеледі; биіктік 0 болса бөлуден сақтаймыз
        float aspect = (gHeight > 0) ? (float)gWidth / (float)gHeight : 1.0f;

        glm::mat4 projection;
        if (gCam.ortho) {
            projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f,
                                    gCam.nearPlane, gCam.farPlane);
        } else {
            projection = glm::perspective(glm::radians(gCam.fovDeg), aspect,
                                          gCam.nearPlane, gCam.farPlane);
        }

        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),   // камера
                                     glm::vec3(0.0f, 0.0f, 0.0f),   // қарайтын нүкте
                                     glm::vec3(0.0f, 1.0f, 0.0f));  // жоғары бағыт

        // projection мен view үш кубқа ортақ — бір рет жібереміз
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(vao);
        for (int i = 0; i < 3; i++) {
            float angle = clock * spinSpeed[i];
            glm::mat4 model(1.0f);
            model = glm::translate(model, cubePos[i]);
            model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));  // Y осі
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
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