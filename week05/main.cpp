#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>

const int WIDTH  = 1280;
const int HEIGHT = 720;

int mode = 1;
int primIndex = 0;
bool wireframe = false;

GLenum primitives[] = { GL_TRIANGLES, GL_LINE_LOOP, GL_LINE_STRIP, GL_POINTS };
const char* primNames[] = { "GL_TRIANGLES", "GL_LINE_LOOP", "GL_LINE_STRIP", "GL_POINTS" };

bool prevKeys[GLFW_KEY_LAST + 1] = { false };

bool pressedOnce(GLFWwindow* window, int key) {
    bool now = glfwGetKey(window, key) == GLFW_PRESS;
    bool fired = now && !prevKeys[key];
    prevKeys[key] = now;
    return fired;
}

void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (pressedOnce(window, GLFW_KEY_1)) { mode = 1; std::cout << "Rezhim 1: tortburysh (EBO)\n"; }
    if (pressedOnce(window, GLFW_KEY_2)) { mode = 2; std::cout << "Rezhim 2: indeks {0,1,2}, 3 dana\n"; }
    if (pressedOnce(window, GLFW_KEY_3)) { mode = 3; std::cout << "Rezhim 3: besburysh\n"; }
    if (pressedOnce(window, GLFW_KEY_4)) { mode = 4; std::cout << "Rezhim 4: eki tortburysh\n"; }
    if (pressedOnce(window, GLFW_KEY_5)) { mode = 5; std::cout << "Rezhim 5: eki tortburysh, qarama-qarsy\n"; }

    if (pressedOnce(window, GLFW_KEY_P)) {
        primIndex = (primIndex + 1) % 4;
        std::cout << "Primitiv: " << primNames[primIndex] << "\n";
    }

    if (pressedOnce(window, GLFW_KEY_TAB)) {
        wireframe = !wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    }
}

unsigned int makeShader(const char* vsSrc, const char* fsSrc) {
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSrc, nullptr);
    glCompileShader(vs);
    int ok;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(vs, 1024, nullptr, log); std::cerr << log << "\n"; }

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(fs, 1024, nullptr, log); std::cerr << log << "\n"; }

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { char log[1024]; glGetProgramInfoLog(prog, 1024, nullptr, log); std::cerr << log << "\n"; }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform vec2 uOffset;

out vec3 vColor;

void main() {
    gl_Position = vec4(aPos.xy + uOffset, aPos.z, 1.0);
    vColor = aColor;
}
)";

const char* fragmentSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() { FragColor = vec4(vColor, 1.0); }
)";

int main() {

    if (!glfwInit()) {
        std::cerr << "GLFW iske qosylmady\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
                                          "Seminar 5 - EBO",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Tereze zhasalmady.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(1);

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD zhuktelmedi\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";
    std::cout << "Pernelar: 1-5 rezhim, P primitiv turi, TAB wireframe, ESC shygu\n";

    const int STRIDE = 6 * sizeof(float);

    // =================================================================
    //  1. ТӨРТБҰРЫШ: 4 вершина + 6 индекс (базалық бөлім)
    // =================================================================
    float quadVertices[] = {
        // позиция          // түс
         0.3f,  0.3f, 0.0f,  1.0f, 0.0f, 0.0f,   // 0 - оң жоғарғы
         0.3f, -0.3f, 0.0f,  0.0f, 1.0f, 0.0f,   // 1 - оң төменгі
        -0.3f, -0.3f, 0.0f,  0.0f, 0.0f, 1.0f,   // 2 - сол төменгі
        -0.3f,  0.3f, 0.0f,  1.0f, 1.0f, 0.0f    // 3 - сол жоғарғы
    };

    unsigned int quadIndices[] = {
        0, 1, 3,    // бірінші үшбұрыш
        1, 2, 3     // екінші үшбұрыш
    };

    unsigned int vaoQuad, vboQuad, eboQuad;
    glGenVertexArrays(1, &vaoQuad);
    glGenBuffers(1, &vboQuad);
    glGenBuffers(1, &eboQuad);

    glBindVertexArray(vaoQuad);                       // VAO бірінші

    glBindBuffer(GL_ARRAY_BUFFER, vboQuad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboQuad);   // EBO — VAO байланып тұрғанда
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    // ЕСКЕРТУ: glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) ЖАЗЫЛМАЙДЫ — VAO-дан EBO жоғалып кетеді

    // =================================================================
    //  1-ТАПСЫРМА: индекстерді бұзу — {0, 1, 2}, сызу кезінде саны 3
    //  Вершина деректері сол күйінде (vboQuad ортақ), тек индекс бөлек
    // =================================================================
    unsigned int brokenIndices[] = { 0, 1, 2 };

    unsigned int vaoBroken, eboBroken;
    glGenVertexArrays(1, &vaoBroken);
    glGenBuffers(1, &eboBroken);

    glBindVertexArray(vaoBroken);

    glBindBuffer(GL_ARRAY_BUFFER, vboQuad);           // дәл сол вершина деректері
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboBroken);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(brokenIndices), brokenIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // =================================================================
    //  2-ТАПСЫРМА: бесбұрыш — 5 вершина, 3 үшбұрыш, 9 индекс
    //  Бұрыштар: -90, -18, 54, 126, 198 градус, радиус 0.3
    //  0-вершина барлық үшбұрышта қайталанады (орталық рөлін атқарады)
    // =================================================================
    float pentVertices[5 * 6];
    float angles[5] = { -90.0f, -18.0f, 54.0f, 126.0f, 198.0f };
    float pentColors[5][3] = {
        {1.0f, 0.0f, 0.0f},
        {1.0f, 0.6f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.5f, 1.0f}
    };
    const float PI = 3.14159265f;
    for (int i = 0; i < 5; i++) {
        float a = angles[i] * PI / 180.0f;
        pentVertices[i * 6 + 0] = std::cos(a) * 0.3f;
        pentVertices[i * 6 + 1] = std::sin(a) * 0.3f;
        pentVertices[i * 6 + 2] = 0.0f;
        pentVertices[i * 6 + 3] = pentColors[i][0];
        pentVertices[i * 6 + 4] = pentColors[i][1];
        pentVertices[i * 6 + 5] = pentColors[i][2];
    }

    unsigned int pentIndices[] = {
        0, 1, 2,
        0, 2, 3,
        0, 3, 4
    };

    unsigned int vaoPent, vboPent, eboPent;
    glGenVertexArrays(1, &vaoPent);
    glGenBuffers(1, &vboPent);
    glGenBuffers(1, &eboPent);

    glBindVertexArray(vaoPent);

    glBindBuffer(GL_ARRAY_BUFFER, vboPent);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pentVertices), pentVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboPent);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(pentIndices), pentIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // -----------------------------------------------------------------
    unsigned int shader = makeShader(vertexSrc, fragmentSrc);
    int locOffset = glGetUniformLocation(shader, "uOffset");

    glPointSize(10.0f);   // GL_POINTS үшін

    float lastFrame = (float)glfwGetTime();
    float angle = 0.0f;
    float speed = 1.5f;

    while (!glfwWindowShouldClose(window)) {

        // --- уақыт ---
        float now = (float)glfwGetTime();
        float dt = now - lastFrame;
        lastFrame = now;

        // --- ЖАҢАРТУ ---
        processInput(window);
        angle += speed * dt;

        GLenum prim = primitives[primIndex];

        // --- СЫЗУ ---
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);

        if (mode == 1) {
            // базалық: шеңбер бойымен қозғалатын төртбұрыш
            glBindVertexArray(vaoQuad);
            glUniform2f(locOffset, std::cos(angle) * 0.4f, std::sin(angle) * 0.4f);
            glDrawElements(prim, 6, GL_UNSIGNED_INT, 0);   // 6 = индекс саны, вершина емес
        }
        else if (mode == 2) {
            // 1-тапсырма: бір ғана үшбұрыш (индекс {0,1,2}, саны 3)
            glBindVertexArray(vaoBroken);
            glUniform2f(locOffset, std::cos(angle) * 0.4f, std::sin(angle) * 0.4f);
            glDrawElements(prim, 3, GL_UNSIGNED_INT, 0);
        }
        else if (mode == 3) {
            // 2-тапсырма: бесбұрыш, 9 индекс
            glBindVertexArray(vaoPent);
            glUniform2f(locOffset, std::cos(angle) * 0.4f, std::sin(angle) * 0.4f);
            glDrawElements(prim, 9, GL_UNSIGNED_INT, 0);
        }
        else if (mode == 4) {
            // 3-тапсырма: екі төртбұрыш, бір VAO, екі uniform + draw жұбы
            glBindVertexArray(vaoQuad);
            glUniform2f(locOffset, -0.4f, 0.0f);
            glDrawElements(prim, 6, GL_UNSIGNED_INT, 0);
            glUniform2f(locOffset, 0.4f, 0.0f);
            glDrawElements(prim, 6, GL_UNSIGNED_INT, 0);
        }
        else if (mode == 5) {
            // қосымша: екеуі де қозғалады, қарама-қарсы бағытта
            glBindVertexArray(vaoQuad);
            glUniform2f(locOffset, std::cos(angle) * 0.4f, std::sin(angle) * 0.4f);
            glDrawElements(prim, 6, GL_UNSIGNED_INT, 0);
            glUniform2f(locOffset, -std::cos(angle) * 0.4f, -std::sin(angle) * 0.4f);
            glDrawElements(prim, 6, GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- тазалау ---
    glDeleteVertexArrays(1, &vaoQuad);
    glDeleteVertexArrays(1, &vaoBroken);
    glDeleteVertexArrays(1, &vaoPent);
    glDeleteBuffers(1, &vboQuad);
    glDeleteBuffers(1, &vboPent);
    glDeleteBuffers(1, &eboQuad);
    glDeleteBuffers(1, &eboBroken);
    glDeleteBuffers(1, &eboPent);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}