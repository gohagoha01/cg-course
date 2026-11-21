// stb_image: #define тек бір .cpp-да, #include-тан БҰРЫН тұруы шарт
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// Сурет қоятын папка. Басқа жерде болса, компиляцияда -DASSET_DIR="\"жол/\"" беруге болады
#ifndef ASSET_DIR
#define ASSET_DIR "assets/"
#endif

// ---------- терезе өлшемі (resize кезінде екеуі де жаңарады) ----------
int gWidth  = 1280;
int gHeight = 720;

static bool gPaused = false;                         // T — сахнаның айналуын тоқтату
static bool gKeyHeld[GLFW_KEY_LAST + 1] = { false };

// ---------- камера (9-апта) + yaw/pitch (10-апта) ----------
struct Camera {
    glm::vec3 pos   = glm::vec3(0.0f, 0.0f, 3.0f);    // камера орны
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);   // қарайтын бағыт (тінтуірден жаңарады)
    glm::vec3 up    = glm::vec3(0.0f, 1.0f, 0.0f);    // жоғары бағыт, әрқашан тұрақты
    float yaw   = -90.0f;   // -90: front (0,0,-1) болу үшін (yaw=0 болса +X-ке қарайды)
    float pitch = 0.0f;     // жоғары-төмен бұрыш
};
Camera gCamera;

const float CAM_SPEED     = 2.5f;    // бірлік/сек
const float CAM_MAX_RANGE = 20.0f;   // камера сахнадан алыс кетпесін
const float MOUSE_SENS    = 0.1f;    // 1 пиксель = 0.1 градус

// ---------- тінтуір күйі (10-апта) ----------
float gLastX = 640.0f;               // терезе ортасы
float gLastY = 360.0f;
bool  gFirstMouse = true;            // бірінші callback-те секіріп кетпеу үшін

// ---------- зум (10-апта) ----------
float gFov = 45.0f;

// ---------- 7-8 апта: проекция және тексеру режимдері ----------
struct ViewParams {
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

// Тінтуір қозғалғанда ғана шақырылады (әр кадрда емес)
void mouseCallback(GLFWwindow*, double xpos, double ypos) {
    if (gFirstMouse) {
        gLastX = (float)xpos;
        gLastY = (float)ypos;
        gFirstMouse = false;
    }

    float xoffset = (float)xpos - gLastX;
    float yoffset = gLastY - (float)ypos;   // кері: экранның y-і төмен қарай өседі, pitch жоғары
    gLastX = (float)xpos;
    gLastY = (float)ypos;

    // Тінтуір — ығысу (пиксель), сондықтан dt-ге көбейтпейміз
    gCamera.yaw   += xoffset * MOUSE_SENS;
    gCamera.pitch += yoffset * MOUSE_SENS;

    // ±89°: 90°-та front worldUp-қа параллель болып, cross нөл вектор береді (NaN)
    gCamera.pitch = std::clamp(gCamera.pitch, -89.0f, 89.0f);

    glm::vec3 dir;
    dir.x = std::cos(glm::radians(gCamera.yaw)) * std::cos(glm::radians(gCamera.pitch));
    dir.y = std::sin(glm::radians(gCamera.pitch));
    dir.z = std::sin(glm::radians(gCamera.yaw)) * std::cos(glm::radians(gCamera.pitch));
    gCamera.front = glm::normalize(dir);
}

// Дөңгелек: fov кішірейсе — жақындайды
void scrollCallback(GLFWwindow*, double, double yoffset) {
    gFov -= (float)yoffset;
    gFov = std::clamp(gFov, 1.0f, 45.0f);
}

void applyGlState() {
    if (gView.depthOn) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (gView.cullOn)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
    glCullFace(gView.cullFront ? GL_FRONT : GL_BACK);
    glPolygonMode(GL_FRONT_AND_BACK, gView.wireframe ? GL_LINE : GL_FILL);
}

// Камераны пернетақтамен жылжыту (dt-ге көбейтілген, FPS-ке тәуелсіз)
void moveCamera(GLFWwindow* w, float dt) {
    float step = CAM_SPEED * dt;
    if (glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) step *= 2.0f;   // жүгіру

    glm::vec3 right = glm::normalize(glm::cross(gCamera.front, gCamera.up));

    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) gCamera.pos += step * gCamera.front;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) gCamera.pos -= step * gCamera.front;
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) gCamera.pos -= right * step;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) gCamera.pos += right * step;

    // Жоғары-төмен: әлемнің Y осі бойымен
    if (glfwGetKey(w, GLFW_KEY_SPACE)      == GLFW_PRESS) gCamera.pos += step * glm::vec3(0.0f, 1.0f, 0.0f);
    if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) gCamera.pos -= step * glm::vec3(0.0f, 1.0f, 0.0f);

    // Камера орнынан 20 бірліктен аспасын
    float dist = glm::length(gCamera.pos);
    if (dist > CAM_MAX_RANGE) gCamera.pos = gCamera.pos / dist * CAM_MAX_RANGE;
}

void readInput(GLFWwindow* w) {
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(w, true);

    if (keyTapped(w, GLFW_KEY_T)) gPaused = !gPaused;

    // R — бастапқы орынға қайту, P — координат
    if (keyTapped(w, GLFW_KEY_R)) {
        gCamera = Camera();
        gFov = 45.0f;
    }
    if (keyTapped(w, GLFW_KEY_P)) {
        std::cout << "camPos: " << gCamera.pos.x << ", "
                  << gCamera.pos.y << ", " << gCamera.pos.z
                  << "  yaw: " << gCamera.yaw << " pitch: " << gCamera.pitch << "\n";
    }

    if (keyTapped(w, GLFW_KEY_O)) {
        gView.ortho = !gView.ortho;
        std::cout << (gView.ortho ? "Rezhim: ortho\n" : "Rezhim: perspective\n");
    }
    if (keyTapped(w, GLFW_KEY_TAB)) gView.wireframe = !gView.wireframe;

    // 8-аптаның тексеру пернелері (сандар қатарында)
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

// ---------- 11-апта: текстураны файлдан жүктеу ----------
// VBO-мен бірдей схема: Gen -> Bind -> дерек (glTexImage2D)
unsigned int loadTexture(const char* path) {
    unsigned int id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);       // осыдан кейінгі glTex* осы текстураға барады

    // wrapping (UV 0..1 сыртына шықса) және фильтрация
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);  // алыстағы: mipmap
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                // жақын: mipmap жоқ

    // OpenGL-де v осі төменнен жоғары, ал суретте жоғарыдан төмен: сондықтан аударамыз
    stbi_set_flip_vertically_on_load(true);

    int w = 0, h = 0, n = 0;
    unsigned char* data = stbi_load(path, &w, &h, &n, 0);
    if (data) {
        // Арна санынан формат: тұрақты GL_RGB болса, RGBA суретте түстер жылжиды
        GLenum fmt = (n == 4) ? GL_RGBA : (n == 1 ? GL_RED : GL_RGB);
        if (n == 3) glPixelStorei(GL_UNPACK_ALIGNMENT, 1);   // 3 арналы суретте жол ұзындығы 4-ке еселі болмауы мүмкін
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);    // ТЕК glTexImage2D-ден кейін
        std::cout << path << " " << w << "x" << h << " arna: " << n << "\n";
    } else {
        std::cerr << "Suret zhuktelmedi: " << path << "\n";
    }
    stbi_image_free(data);                  // GPU-ға көшті, CPU көшірмесі керек емес
    return id;
}

// Вершина шейдері UV-ді өткізіп жібереді (түс атрибуты алынды)
const char* VS_CODE = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vTexCoord;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    vTexCoord = aTexCoord;
}
)";

// Фрагмент шейдері суреттен түс оқиды. uTint әдетте (1,1,1): Z-fight демосында ғана өзгереді
const char* FS_CODE = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D ourTexture;   // текстура бірлігінің нөмірі (0), текстураның өзі емес
uniform vec3 uTint;

void main() {
    FragColor = texture(ourTexture, vTexCoord) * vec4(uTint, 1.0);
}
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

    GLFWwindow* win = glfwCreateWindow(gWidth, gHeight, "Week 11 - texture", nullptr, nullptr);
    if (!win) {
        std::cerr << "Tereze ashylmady\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, handleResize);
    glfwSwapInterval(1);

    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(win, mouseCallback);
    glfwSetScrollCallback(win, scrollCallback);

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD qate\n";
        glfwTerminate();
        return -1;
    }

    glfwGetFramebufferSize(win, &gWidth, &gHeight);
    gLastX = gWidth * 0.5f;
    gLastY = gHeight * 0.5f;

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "Kamera: tintuir - burylu, WASD - zhuru, Space/Shift - zhogary/tomen, Ctrl - zhugiru, scroll - zoom\n";
    std::cout << "Basqa: R - bastapqy, P - koordinat, T - pauza, O - ortho, Tab - wireframe, 1..6 - tekseru, ESC - shygu\n";

    int depthBits = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH,
                                          GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depthBits);
    std::cout << "depth bits: " << depthBits << "\n";

    // ---------- КУБ: 36 вершина, әрқайсысы 5 float (позиция xyz + UV) ----------
    // Түс жоқ: оның орнына UV. Әр жақта UV реті бірдей: (0,0)(1,0)(1,1) және (0,0)(1,1)(0,1).
    // Барлық үшбұрыш сыртынан қарағанда CCW (culling үшін).
    const float cubeData[] = {
        // алдыңғы жақ (+Z)
         -0.5f,  -0.5f,   0.5f,  0.0f, 0.0f,
          0.5f,  -0.5f,   0.5f,  1.0f, 0.0f,
          0.5f,   0.5f,   0.5f,  1.0f, 1.0f,
         -0.5f,  -0.5f,   0.5f,  0.0f, 0.0f,
          0.5f,   0.5f,   0.5f,  1.0f, 1.0f,
         -0.5f,   0.5f,   0.5f,  0.0f, 1.0f,
        // артқы жақ (-Z)
          0.5f,  -0.5f,  -0.5f,  0.0f, 0.0f,
         -0.5f,  -0.5f,  -0.5f,  1.0f, 0.0f,
         -0.5f,   0.5f,  -0.5f,  1.0f, 1.0f,
          0.5f,  -0.5f,  -0.5f,  0.0f, 0.0f,
         -0.5f,   0.5f,  -0.5f,  1.0f, 1.0f,
          0.5f,   0.5f,  -0.5f,  0.0f, 1.0f,
        // сол жақ (-X)
         -0.5f,  -0.5f,  -0.5f,  0.0f, 0.0f,
         -0.5f,  -0.5f,   0.5f,  1.0f, 0.0f,
         -0.5f,   0.5f,   0.5f,  1.0f, 1.0f,
         -0.5f,  -0.5f,  -0.5f,  0.0f, 0.0f,
         -0.5f,   0.5f,   0.5f,  1.0f, 1.0f,
         -0.5f,   0.5f,  -0.5f,  0.0f, 1.0f,
        // оң жақ (+X)
          0.5f,  -0.5f,   0.5f,  0.0f, 0.0f,
          0.5f,  -0.5f,  -0.5f,  1.0f, 0.0f,
          0.5f,   0.5f,  -0.5f,  1.0f, 1.0f,
          0.5f,  -0.5f,   0.5f,  0.0f, 0.0f,
          0.5f,   0.5f,  -0.5f,  1.0f, 1.0f,
          0.5f,   0.5f,   0.5f,  0.0f, 1.0f,
        // төменгі жақ (-Y)
         -0.5f,  -0.5f,  -0.5f,  0.0f, 0.0f,
          0.5f,  -0.5f,  -0.5f,  1.0f, 0.0f,
          0.5f,  -0.5f,   0.5f,  1.0f, 1.0f,
         -0.5f,  -0.5f,  -0.5f,  0.0f, 0.0f,
          0.5f,  -0.5f,   0.5f,  1.0f, 1.0f,
         -0.5f,  -0.5f,   0.5f,  0.0f, 1.0f,
        // жоғарғы жақ (+Y)
         -0.5f,   0.5f,   0.5f,  0.0f, 0.0f,
          0.5f,   0.5f,   0.5f,  1.0f, 0.0f,
          0.5f,   0.5f,  -0.5f,  1.0f, 1.0f,
         -0.5f,   0.5f,   0.5f,  0.0f, 0.0f,
          0.5f,   0.5f,  -0.5f,  1.0f, 1.0f,
         -0.5f,   0.5f,  -0.5f,  0.0f, 1.0f
    };
    const int VERTEX_COUNT = 36;

    unsigned int vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeData), cubeData, GL_STATIC_DRAW);

    // Екі жерде де stride 6 емес, 5; екінші атрибут: 2 компонент (UV), offset 3 float
    const GLsizei stride = 5 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    unsigned int prog = buildProgram(VS_CODE, FS_CODE);

    int locModel = glGetUniformLocation(prog, "uModel");
    int locView  = glGetUniformLocation(prog, "uView");
    int locProj  = glGetUniformLocation(prog, "uProjection");
    int locTex   = glGetUniformLocation(prog, "ourTexture");
    int locTint  = glGetUniformLocation(prog, "uTint");

    // Текстураны циклге дейін бір рет жүктейміз
    std::string texPath = std::string(ASSET_DIR) + "container.png";
    unsigned int texture = loadTexture(texPath.c_str());

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
        moveCamera(win, dt);
        if (!gPaused) clock += dt;

        glClearColor(0.07f, 0.08f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog);

        // Текстураны 0-бірлікке байлаймыз, sampler-ге сол бірліктің нөмірін береміз
        glUniform1i(locTex, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        float aspect = (gHeight > 0) ? (float)gWidth / (float)gHeight : 1.0f;

        glm::mat4 projection;
        if (gView.ortho) {
            projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f,
                                    gView.nearPlane, gView.farPlane);
        } else {
            projection = glm::perspective(glm::radians(gFov), aspect,
                                          gView.nearPlane, gView.farPlane);
        }

        glm::mat4 view = glm::lookAt(gCamera.pos,
                                     gCamera.pos + gCamera.front,
                                     gCamera.up);

        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(vao);

        if (!gView.zFightDemo) {
            glUniform3f(locTint, 1.0f, 1.0f, 1.0f);
            for (int i = 0; i < CUBE_COUNT; i++) {
                glm::mat4 model(1.0f);
                model = glm::translate(model, cubePos[i]);
                model = glm::rotate(model, clock * (0.5f + i * 0.3f),
                                    glm::vec3(0.3f, 1.0f, 0.0f));
                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, VERTEX_COUNT);
            }
        } else {
            // Текстура бірдей болғандықтан жыпылықтау көрінбей қалмас үшін екінші кубты қараңғылаймыз
            glm::mat4 modelA(1.0f);
            glUniform3f(locTint, 1.0f, 1.0f, 1.0f);
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelA));
            glDrawArrays(GL_TRIANGLES, 0, VERTEX_COUNT);

            glm::mat4 modelB(1.0f);
            modelB = glm::translate(modelB, glm::vec3(0.0f, 0.0f, 0.001f));
            modelB = glm::rotate(modelB, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glUniform3f(locTint, 0.4f, 0.4f, 0.4f);
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelB));
            glDrawArrays(GL_TRIANGLES, 0, VERTEX_COUNT);
        }

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteTextures(1, &texture);
    glDeleteProgram(prog);

    glfwTerminate();
    return 0;
}