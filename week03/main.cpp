#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <vector>

const int WIDTH  = 1280;
const int HEIGHT = 720;
bool isSpacePressed = false;

// [СЕМИНАР 3] 2-тапсырма: true қойсаң, үш төбеге де бірдей түс беріледі
const bool SAME_COLOR = false;

// ШЕЙДЕРЛЕР
const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;   // [СЕМИНАР 3] екінші атрибут: түс
out vec3 vColor;                        // [СЕМИНАР 3] fragment shader-ге жіберу
void main() { 
    gl_Position = vec4(aPos, 1.0); 
    vColor = aColor;
}
)";

const char* fragmentSrc = R"(
#version 330 core
in vec3 vColor;                         // [СЕМИНАР 3] vertex shader-ден келген түс
out vec4 FragColor;
void main() { 
    FragColor = vec4(vColor, 1.0);
    // 1-тапсырма (түстерді төңкеру): жоғарыдағы жолды өшіріп, төмендегіні қос
    // FragColor = vec4(1.0 - vColor, 1.0);
}
)";

void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        isSpacePressed = true;
    } else {
        isSpacePressed = false;
    } 
}

// [СЕМИНАР 3] makeShader(): компиляция + линк + қатені тексеру
unsigned int makeShader(const char* vsSrc, const char* fsSrc) {
    int success;
    char log[1024];

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSrc, nullptr);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 1024, nullptr, log);
        std::cerr << "VERTEX SHADER ERROR:\n" << log << std::endl;
    }

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 1024, nullptr, log);
        std::cerr << "FRAGMENT SHADER ERROR:\n" << log << std::endl;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "LINK ERROR:\n" << log << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Компьютерлік графика", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(0); // VSync өшіру (1-апта)

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        glfwTerminate();
        return -1;
    }

    // === ГЕНЕРАЦИЯ СЕТКИ ПРЯМОУГОЛЬНИКОВ И ТРЕУГОЛЬНИКОВ (2-АПТА) ===
    std::vector<float> vertices;
    int rows = 6;
    int cols = 4;

    float startX = -0.85f, endX = 0.85f;
    float startY = -0.85f, endY = 0.85f;

    float dx = (endX - startX) / cols;
    float dy = (endY - startY) / rows;

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            float x0 = startX + j * dx;
            float y0 = startY + i * dy;
            float x1 = x0 + dx * 0.92f; // небольшие зазоры между блоками
            float y1 = y0 + dy * 0.92f;

            // [СЕМИНАР 3] Әр төбе: x y z  r g b  (stride = 6 float)
            if (SAME_COLOR) {
                vertices.insert(vertices.end(), {
                    x0, y0, 0.0f,  0.95f, 0.55f, 0.25f,
                    x1, y0, 0.0f,  0.95f, 0.55f, 0.25f,
                    x1, y1, 0.0f,  0.95f, 0.55f, 0.25f
                });
            } else {
                vertices.insert(vertices.end(), {
                    x0, y0, 0.0f,  1.0f, 0.0f, 0.0f,   // қызыл
                    x1, y0, 0.0f,  0.0f, 1.0f, 0.0f,   // жасыл
                    x1, y1, 0.0f,  0.0f, 0.0f, 1.0f    // көк
                });
            }
        }
    }

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // [СЕМИНАР 3] location = 0: позиция, stride 6 float, offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // [СЕМИНАР 3] location = 1: түс, stride 6 float, offset 3 float
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Сплошная заливка полигонов
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // [СЕМИНАР 3] ШЕЙДЕРЛЕРДІ makeShader() арқылы жасау
    unsigned int shader = makeShader(vertexSrc, fragmentSrc);

    // Переменные для FPS (1-апта)
    double lastTime = glfwGetTime();
    int frameCount = 0;

    // НЕГІЗГІ ЦИКЛ
    while (!glfwWindowShouldClose(window)) {
        // 1-АПТА: Вывод FPS в консоль
        double currentTime = glfwGetTime();
        frameCount++;
        if (currentTime - lastTime >= 1.0) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }

        processInput(window);

        // 1-АПТА: Динамический фон + Белый фон по нажатию пробела
        if (isSpacePressed) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // Белый фон при Пробеле
        } else {
            float t = (float)glfwGetTime();
            float r = (std::sin(t * 2.0f) + 1.0f) * 0.5f * 0.25f;
            float g = (std::sin(t * 1.5f) + 1.0f) * 0.5f * 0.25f;
            glClearColor(r, g, 0.25f, 1.0f);      // Плавно меняющийся фон
        }
        glClear(GL_COLOR_BUFFER_BIT);

        // СЫЗУ КӨРІНІСІ
        glUseProgram(shader);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 6); // [СЕМИНАР 3] stride 6

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}