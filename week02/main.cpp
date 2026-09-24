#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <vector>

const int WIDTH  = 1280;
const int HEIGHT = 720;
bool isSpacePressed = false;

// ШЕЙДЕРЛЕР
const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
void main() { 
    gl_Position = vec4(aPos, 1.0); 
}
)";

const char* fragmentSrc = R"(
#version 330 core
out vec4 FragColor;
void main() { 
    // Оранжевый цвет фигуры как на фото
    FragColor = vec4(0.95f, 0.55f, 0.25f, 1.0f); 
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

            // Каждый блок состоит из оранжевого треугольника (как на фото)
            vertices.insert(vertices.end(), {
                x0, y0, 0.0f,
                x1, y0, 0.0f,
                x1, y1, 0.0f
            });
        }
    }

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Сплошная заливка полигонов
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // ШЕЙДЕРЛЕРДІ КОМПИЛЯЦИЯЛАУ
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexSrc, nullptr);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentSrc, nullptr);
    glCompileShader(fs);

    unsigned int shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs);
    glDeleteShader(fs);

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
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}