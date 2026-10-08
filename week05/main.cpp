#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <vector>

const int WIDTH  = 1280;
const int HEIGHT = 720;
bool isSpacePressed = false;

// [СЕМИНАР 4] 1-тапсырма: false қойсаң dt өшеді (кадрға тәуелді қозғалыс)
const bool USE_DT = true;

// [СЕМИНАР 5] 3-тапсырма: true қойсаң, төртбұрыш екі рет сызылады
const bool DRAW_TWICE = false;

// [СЕМИНАР 4] Анимация күйі
float angle    = 0.0f;   // шеңбердегі бұрыш (радиан)
float speed    = 1.5f;   // бұрыштық жылдамдық (рад/сек), W/S өзгертеді
float timeAcc  = 0.0f;   // жиналған уақыт (пульсация үшін)
float offsetX  = 0.0f, offsetY = 0.0f;
float scaleVal = 1.0f;
const float RADIUS = 0.4f; // [СЕМИНАР 5] орбита радиусы 0.15 -> 0.4

// ШЕЙДЕРЛЕР
const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;   // [СЕМИНАР 3] екінші атрибут: түс
uniform vec2 uOffset;                   // [СЕМИНАР 4] орын ауыстыру
uniform float uScale;                   // [СЕМИНАР 4] масштаб (пульсация)
out vec3 vColor;                        // [СЕМИНАР 3] fragment shader-ге жіберу
void main() { 
    gl_Position = vec4(aPos.xy * uScale + uOffset, aPos.z, 1.0); 
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

// [СЕМИНАР 4] update(): тек күйді жаңартады (сызбайды)
void update(GLFWwindow* window, float dt) {
    // W/S: жылдамдықты басқару (өзгеру жылдамдығы да dt-ға байланысты)
    float step = USE_DT ? dt : 0.016f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) speed += 2.0f * step;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) speed -= 2.0f * step;
    if (speed < 0.0f) speed = 0.0f;
    if (speed > 10.0f) speed = 10.0f;

    // Шеңбер бойымен қозғалу
    angle   += speed * step;
    timeAcc += step;

    offsetX = RADIUS * std::cos(angle);
    offsetY = RADIUS * std::sin(angle);

    // uScale пульсациясы
    scaleVal = 0.7f + 0.15f * std::sin(timeAcc * 3.0f);
}

// [СЕМИНАР 4] draw(): тек сызады (uniform-дарды жібереді)
// [СЕМИНАР 5] glDrawArrays орнына glDrawElements, indexCount параметрі қосылды
void draw(unsigned int shader, unsigned int vao, int indexCount,
          int locOffset, int locScale) {
    glUseProgram(shader);
    glUniform1f(locScale, scaleVal);
    glBindVertexArray(vao);

    // Бірінші төртбұрыш
    glUniform2f(locOffset, offsetX, offsetY);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

    // [СЕМИНАР 5] 3-тапсырма: екінші төртбұрыш орбитаның қарсы жағында
    if (DRAW_TWICE) {
        glUniform2f(locOffset, -offsetX, -offsetY);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
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

    // [СЕМИНАР 5] 1. Вершина деректері: төртбұрыш, 4 вершина x (позиция + түс)
    float vertices[] = {
        // позиция          // түс
         0.3f,  0.3f, 0.0f,  1.0f, 0.0f, 0.0f,   // 0 - оң жоғарғы
         0.3f, -0.3f, 0.0f,  0.0f, 1.0f, 0.0f,   // 1 - оң төменгі
        -0.3f, -0.3f, 0.0f,  0.0f, 0.0f, 1.0f,   // 2 - сол төменгі
        -0.3f,  0.3f, 0.0f,  1.0f, 1.0f, 0.0f    // 3 - сол жоғарғы
    };

    // [СЕМИНАР 5] 2. Индекс массиві: 2 үшбұрыш, барлығы сағат тіліне қарсы
    unsigned int indices[] = {
        0, 1, 3,    // бірінші үшбұрыш
        1, 2, 3     // екінші үшбұрыш
    };
    int indexCount = sizeof(indices) / sizeof(unsigned int); // = 6

    // [СЕМИНАР 5] 3. VAO + VBO + EBO
    unsigned int vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);                          // ЖАҢА

    glBindVertexArray(vao);                         // VAO бірінші

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);     // ЖАҢА: EBO VAO ішінде байланады
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    const int STRIDE = 6 * sizeof(float);

    // [СЕМИНАР 3] location = 0: позиция, stride 6 float, offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);

    // [СЕМИНАР 3] location = 1: түс, stride 6 float, offset 3 float
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    // [СЕМИНАР 5] ЕСКЕРТУ: glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) ЖАЗЫЛМАЙДЫ!
    // Ол EBO-ны VAO-дан үзіп тастайды да, экран қара болады.

    // Сплошная заливка полигонов
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // [СЕМИНАР 3] ШЕЙДЕРЛЕРДІ makeShader() арқылы жасау
    unsigned int shader = makeShader(vertexSrc, fragmentSrc);

    // [СЕМИНАР 4] Uniform орнын БІР РЕТ кэштеу (циклге дейін)
    int locOffset = glGetUniformLocation(shader, "uOffset");
    int locScale  = glGetUniformLocation(shader, "uScale");

    // Переменные для FPS (1-апта)
    double lastTime = glfwGetTime();
    int frameCount = 0;

    // [СЕМИНАР 4] dt үшін алдыңғы кадр уақыты
    double prevTime = glfwGetTime();

    // НЕГІЗГІ ЦИКЛ
    while (!glfwWindowShouldClose(window)) {
        // [СЕМИНАР 4] dt есептеу (үш жол)
        double now = glfwGetTime();
        float dt = (float)(now - prevTime);
        prevTime = now;

        // 1-АПТА: Вывод FPS в консоль
        double currentTime = glfwGetTime();
        frameCount++;
        if (currentTime - lastTime >= 1.0) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }

        processInput(window);

        // [СЕМИНАР 4] Күйді жаңарту
        update(window, dt);

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
        draw(shader, vao, indexCount, locOffset, locScale);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);      // [СЕМИНАР 5] ЖАҢА
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}