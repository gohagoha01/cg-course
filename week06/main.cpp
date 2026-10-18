#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <iostream>
#include <vector>

// ---------- терезе параметрлері ----------
static const int   WIN_W = 1280;
static const int   WIN_H = 720;
static float       gAspect = (float)WIN_W / (float)WIN_H;  // шар сопақ болмауы үшін
static bool        gPaused = false;                        // Space басқанда ауысады
static bool        gKeyHeld[GLFW_KEY_LAST + 1] = { false };

// ---------- сахна параметрлері (бір жерге жиналған) ----------
struct Scene {
    float groundTop    = -0.75f;  // жердің жоғарғы сызығы (NDC)
    float objSize      = 0.36f;   // шардың диаметрі = үшбұрыштың өлшемі
    float jumpHeight   = 1.20f;   // шардың көтерілу биіктігі
    float jumpPeriod   = 1.20f;   // бір секіру уақыты, сек
    int   passJumps    = 2;       // үшбұрыш бір шетке жеткенше шардың секіру саны
    float rollFactor   = 3.5f;    // x бірлігіне сәйкес айналу бұрышы (радиан)
    float roamLimit    = 3.2f;    // үшбұрыш қаншалықты алысқа кетеді (экраннан тыс)
};

// ---------- GPU-ға жүктелген геометрия ----------
struct GpuMesh {
    unsigned int vao = 0, vbo = 0, ebo = 0;
    int          count = 0;
};

// Бір рет басылғанда ғана true қайтарады
bool keyTapped(GLFWwindow* w, int key) {
    bool down  = glfwGetKey(w, key) == GLFW_PRESS;
    bool tap   = down && !gKeyHeld[key];
    gKeyHeld[key] = down;
    return tap;
}

void handleResize(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
    if (h > 0) gAspect = (float)w / (float)h;
}

void readInput(GLFWwindow* w) {
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(w, true);

    if (keyTapped(w, GLFW_KEY_SPACE)) {
        gPaused = !gPaused;
        std::cout << (gPaused ? "Pauza\n" : "Jalgasuda\n");
    }
}

// Шейдер компиляциясы кезіндегі қатені шығаратын көмекші
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

// Шейдерлер: әр вершинада позиция мен түс бар
const char* VS_CODE = R"(
#version 330 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

uniform mat4 uModel;
uniform mat4 uProj;

out vec3 passColor;

void main() {
    gl_Position = uProj * uModel * vec4(inPos, 1.0);
    passColor = inColor;
}
)";

const char* FS_CODE = R"(
#version 330 core
in vec3 passColor;
out vec4 outColor;
void main() { outColor = vec4(passColor, 1.0); }
)";

// VAO, VBO, EBO жасап, атрибуттарды (позиция + түс) баптайды
GpuMesh uploadMesh(const std::vector<float>& verts, const std::vector<unsigned int>& idx) {
    GpuMesh m;
    m.count = (int)idx.size();

    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);

    glBindVertexArray(m.vao);

    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    // EBO VAO-ға байланып тұрғанда ғана VAO ішінде сақталады
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);

    const GLsizei stride = 6 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return m;
}

void freeMesh(GpuMesh& m) {
    glDeleteVertexArrays(1, &m.vao);
    glDeleteBuffers(1, &m.vbo);
    glDeleteBuffers(1, &m.ebo);
}

// Бір объектіні модель матрицасымен салады
void drawMesh(const GpuMesh& m, int modelLoc, const glm::mat4& model) {
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(m.vao);
    glDrawElements(GL_TRIANGLES, m.count, GL_UNSIGNED_INT, 0);
}

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

    GLFWwindow* win = glfwCreateWindow(WIN_W, WIN_H, "2D sahna: shar men ushburysh", nullptr, nullptr);
    if (!win) {
        std::cerr << "Terezе ashylmady\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, handleResize);
    glfwSwapInterval(1);

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD qate\n";
        glfwTerminate();
        return -1;
    }

    int fw, fh;
    glfwGetFramebufferSize(win, &fw, &fh);
    if (fh > 0) gAspect = (float)fw / (float)fh;

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "Basqaru: Space - pauza, ESC - shygu\n";

    const float PI = 3.14159265f;
    Scene sc;

    // ---------- ШАР: ортасы + 48 нүкте, EBO арқылы ----------
    const int RING = 48;
    std::vector<float> cv;
    std::vector<unsigned int> ci;

    // 0-вершина: шеңбердің орталығы (ашық сары)
    cv.insert(cv.end(), { 0.0f, 0.0f, 0.0f,  1.0f, 0.85f, 0.35f });
    // 1..48 вершина: шеңбер бойындағы нүктелер (қызғылт-қызыл)
    for (int i = 0; i < RING; i++) {
        float ang = 2.0f * PI * (float)i / (float)RING;
        cv.insert(cv.end(), { std::cos(ang), std::sin(ang), 0.0f,  0.9f, 0.25f, 0.35f });
    }
    // әр сектор = орталық + екі көрші нүкте
    for (int i = 0; i < RING; i++) {
        ci.push_back(0);
        ci.push_back(1 + i);
        ci.push_back(1 + (i + 1) % RING);
    }
    GpuMesh ball = uploadMesh(cv, ci);

    // ---------- ҮШБҰРЫШ: 1x1 өлшемді, кейін scale-пен үлкейтеміз ----------
    std::vector<float> tv = {
        -0.5f, -0.5f, 0.0f,  0.1f, 0.9f, 0.5f,
         0.5f, -0.5f, 0.0f,  0.1f, 0.6f, 1.0f,
         0.0f,  0.5f, 0.0f,  1.0f, 0.8f, 0.2f
    };
    std::vector<unsigned int> ti = { 0, 1, 2 };
    GpuMesh tri = uploadMesh(tv, ti);

    // ---------- ЖЕР: 1x1 төртбұрыш (4 вершина, 6 индекс) ----------
    std::vector<float> gv = {
         0.5f,  0.5f, 0.0f,  0.20f, 0.35f, 0.25f,
         0.5f, -0.5f, 0.0f,  0.10f, 0.20f, 0.15f,
        -0.5f, -0.5f, 0.0f,  0.10f, 0.20f, 0.15f,
        -0.5f,  0.5f, 0.0f,  0.20f, 0.35f, 0.25f
    };
    std::vector<unsigned int> gi = { 0, 1, 3,  1, 2, 3 };
    GpuMesh floorMesh = uploadMesh(gv, gi);

    unsigned int prog = buildProgram(VS_CODE, FS_CODE);
    int uModel = glGetUniformLocation(prog, "uModel");
    int uProj  = glGetUniformLocation(prog, "uProj");

    // ---------- туынды шамалар ----------
    const float radius = sc.objSize * 0.5f;
    // Үшбұрыш айналғанда центрден ең алыс нүктесі ~0.707*өлшем. Соны жердің үстіне көтереміз
    const float triCenterY = sc.groundTop + 0.707f * sc.objSize;

    // Үшбұрыштың толық толқын периоды: екі жаққа бару-қайту.
    // Центрден өту әр (period/2) сайын болады, ол jumpPeriod-қа еселі,
    // сондықтан өту сәтін шардың шыңымен сәйкестендіруге болады.
    const float wavePeriod = 2.0f * sc.passJumps * sc.jumpPeriod;

    float clock = 0.0f;                          // ортақ ойын уақыты (пауза кезінде тоқтайды)
    float prevTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(win)) {

        // delta time
        float nowT = (float)glfwGetTime();
        float dt = nowT - prevTime;
        prevTime = nowT;

        readInput(win);
        if (!gPaused) clock += dt;               // екі объект те осы сағатпен жүреді

        // --- Шар: парабола бойымен секіру ---
        float phase = std::fmod(clock / sc.jumpPeriod, 1.0f);   // 0..1
        float lift  = 4.0f * phase * (1.0f - phase);            // 0 -> 1 -> 0
        float ballY = sc.groundTop + radius + sc.jumpHeight * lift;

        // --- Үшбұрыш: үшбұрышты толқын (fabs арқылы) ---
        // u=0.25 және u=0.75 кезінде x=0 (центрден өтеді).
        // Ығысу: clock = jumpPeriod/2 болғанда (шар шыңында) u дәл 0.25 болады.
        float shifted = clock - 0.5f * sc.jumpPeriod + 0.25f * wavePeriod + wavePeriod;
        float u = std::fmod(shifted, wavePeriod) / wavePeriod;  // 0..1
        float wave = 1.0f - 4.0f * std::fabs(u - 0.5f);         // -1..1
        float triX = sc.roamLimit * wave;
        // Бұрыш x-ке тәуелді: оңға жүрсе сағат тілімен, солға жүрсе кері домалайды
        float triAng = -triX * sc.rollFactor;

        // --- Сызу ---
        glClearColor(0.05f, 0.07f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);

        glm::mat4 proj = glm::ortho(-gAspect, gAspect, -1.0f, 1.0f);
        glUniformMatrix4fv(uProj, 1, GL_FALSE, glm::value_ptr(proj));

        // Жер
        glm::mat4 mFloor(1.0f);
        mFloor = glm::translate(mFloor, glm::vec3(0.0f, sc.groundTop - 0.15f, 0.0f));
        mFloor = glm::scale(mFloor, glm::vec3(2.0f * gAspect + 0.2f, 0.3f, 1.0f));
        drawMesh(floorMesh, uModel, mFloor);

        // Шар: translate * scale
        glm::mat4 mBall(1.0f);
        mBall = glm::translate(mBall, glm::vec3(0.0f, ballY, 0.0f));
        mBall = glm::scale(mBall, glm::vec3(radius, radius, 1.0f));
        drawMesh(ball, uModel, mBall);

        // Үшбұрыш: translate * rotate * scale
        glm::mat4 mTri(1.0f);
        mTri = glm::translate(mTri, glm::vec3(triX, triCenterY, 0.0f));
        mTri = glm::rotate(mTri, triAng, glm::vec3(0.0f, 0.0f, 1.0f));
        mTri = glm::scale(mTri, glm::vec3(sc.objSize, sc.objSize, 1.0f));
        drawMesh(tri, uModel, mTri);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    freeMesh(ball);
    freeMesh(tri);
    freeMesh(floorMesh);
    glDeleteProgram(prog);

    glfwTerminate();
    return 0;
}