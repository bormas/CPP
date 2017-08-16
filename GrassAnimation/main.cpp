#include <iostream>
#include <vector>
#include <ctime>
#include <SOIL.h>
#include "Utility.h"

using namespace std;

const uint GRASS_INSTANCES = 10000; // Количество травинок
const uint FLOWER_INSTANCES = 200;  // Количество цветков
const uint ROCK_INSTANCES = 100;    // Количество камней

GL::Camera camera;       // Мы предоставляем Вам реализацию камеры. В OpenGL камера - это просто 2 матрицы. Модельно-видовая матрица и матрица проекции. // ###
                         // Задача этого класса только в том чтобы обработать ввод с клавиатуры и правильно сформировать эти матрицы.
                         // Вы можете просто пользоваться этим классом для расчёта указанных матриц.

// Трава
GLuint grassShader;      // Шейдер, рисующий траву
GLuint grassVAO;         // VAO для травы
GLuint grassTBO;		 // Текстура травы
GLuint grassPointsCount; // Количество вершин у модели травинки
GLuint grassVariance;    // Смещения координат травинок
vector<VM::vec4> grassVarianceData(GRASS_INSTANCES); // Вектор со смещениями для координат травинок
GLfloat grassVelocity;

// Земля
GLuint groundShader;	 // Шейдер для земли
GLuint groundVAO;		 // VAO для земли
GLuint groundTBO;		 // Текстура земли

// Небо
GLuint skyShader;		 // Шейдер для неба
GLuint skyVAO;			 // VAO для неба
GLuint skyTBO;			 // Текстура неба

// Цветок
GLuint flowerShader;	  // Шейдер для цветка
GLuint flowerVAO;		  // VAO для цветка
GLuint flowerPointsCount; // Количество вершин у модели цветка
GLuint flowerVariance;    // Смещения координат цветка
vector<VM::vec4> flowerVarianceData(FLOWER_INSTANCES); // Вектор со смещениями для координат цветка
GLfloat flowerVelocity;

// Камень
GLuint rockShader;		  // Шейдер для камня
GLuint rockVAO;			  // VAO для камня
GLuint rockTBO;			  // Текстура камня
GLuint rockPointsCount;   // Количество вершин у модели камня
GLuint rockVariance;      // Смещения координат камня

// Размеры экрана
uint screenWidth = 1280;
uint screenHeight = 720;

// Для отрисовки нормалей и связей 
int key_pressed = 1;

// Ветер
bool windflag = false;			// Флаг ветра
float wind = 0;					// Ускорение переданное ветром

const float delta_t = 0.0009f;	// Временные промежутки
float a_grass;					// Ускорение травинки по осям
float a_flower;					// Ускорение цветка по осям
const float k_guk = 0.25f;		// Коэффициент силы Гука								
								// Определяет то, насколько отклонится цветок
const float k_resistance = 4;	// Коэффициент силы сопротивления
								// Определяет то, насколько быстро цветок вернется в исходное положение

// Мышь
bool captureMouse = false;		// Это для захвата мышки
bool pressedMouse = false;		// Это true если левая кнопка мыши нажата
int prevX, prevY;				// Предыдущие координаты мышки
int centerX = screenWidth / 2,	// Координаты центра окна
	centerY = screenHeight / 2;

// Таймер
uint speed = 6;	// Обновляем таймер каждые 'speed' миллисекунд

// Мультисэмплинг
bool MSflag = true;

// Обновление коэффициентов смещения травинок
void UpdateGrassVariance() {
	// Ускорение
	a_grass = wind - k_guk * grassVarianceData[0].x - k_resistance * grassVelocity;
	// Скорость
	grassVelocity += delta_t * a_grass;
}

// Обновление смещения травинок
void BindGrassVariance() {
	
	//UpdateGrassVariance();

	for (uint i = 0; i < GRASS_INSTANCES; ++i) {

		grassVarianceData[i].x += delta_t * delta_t * a_grass / 2 + grassVelocity; 
		grassVarianceData[i].y = -abs(grassVarianceData[i].x) / 3;
	}

    // Привязываем буфер, содержащий смещения
    glBindBuffer(GL_ARRAY_BUFFER, grassVariance);                                CHECK_GL_ERRORS
    // Загружаем данные в видеопамять
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * GRASS_INSTANCES, grassVarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
    // Отвязываем буфер
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
}

// Обновление коэффициентов смещения цветов
void UpdateFlowerVariance() {
	// Ускорение
	a_flower = wind - k_guk * flowerVarianceData[0].x - k_resistance * flowerVelocity;
	// Скорость
	flowerVelocity += delta_t * a_flower;
}

// Обновление смещения цветов
void BindFlowerVariance() {

	//UpdateFlowerVariance();

	for (uint i = 0; i < FLOWER_INSTANCES; ++i) {
		flowerVarianceData[i].x += delta_t * delta_t * a_flower / 2 + flowerVelocity;
		flowerVarianceData[i].y = -abs(flowerVarianceData[i].x) / 3;
	}

	// Привязываем буфер, содержащий смещения
	glBindBuffer(GL_ARRAY_BUFFER, flowerVariance);                                CHECK_GL_ERRORS
	// Загружаем данные в видеопамять
	glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * FLOWER_INSTANCES, flowerVarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
	// Отвязываем буфер
	glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
}

// Таймер
void Timer(int) {	
	if (windflag) 		
		wind = 0.5; // Задаем ускорение ветра
	else  
		wind = 0;
	
	UpdateGrassVariance();
	UpdateFlowerVariance();
	glutTimerFunc(speed, Timer, 0);
}

// Функция, рисующая землю
void DrawGround() {
    // Используем шейдер для земли
    glUseProgram(groundShader);                                                  CHECK_GL_ERRORS

    // Устанавливаем юниформ для шейдера. В данном случае передадим перспективную матрицу камеры
    // Находим локацию юниформа 'camera' в шейдере
    GLint cameraLocation = glGetUniformLocation(groundShader, "camera");         CHECK_GL_ERRORS
    // Устанавливаем юниформ (загружаем на GPU матрицу проекции?) 
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
	
    // Подключаем VAO, который содержит буферы, необходимые для отрисовки земли
    glBindVertexArray(groundVAO);                                                CHECK_GL_ERRORS

	GL::bindTexture(groundShader, "ground_texture", groundTBO, 0);

    // Рисуем землю: 2 треугольника (6 вершин)
    glDrawArrays(GL_TRIANGLES, 0, 6);                                            CHECK_GL_ERRORS
	
    // Отсоединяем VAO
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    // Отключаем шейдер
    glUseProgram(0);                                                             CHECK_GL_ERRORS
}

// Функция, рисующая скайбокс
void DrawSky() {
    // Используем шейдер для неба
    glUseProgram(skyShader);													 CHECK_GL_ERRORS

    // Устанавливаем юниформ для шейдера. В данном случае передадим перспективную матрицу камеры
    // Находим локацию юниформа 'camera' в шейдере
    GLint cameraLocation = glGetUniformLocation(skyShader, "camera");			 CHECK_GL_ERRORS
    // Устанавливаем юниформ (загружаем на GPU матрицу проекции?) 
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
	
    // Подключаем VAO, который содержит буферы, необходимые для отрисовки земли
    glBindVertexArray(skyVAO);													 CHECK_GL_ERRORS

	GL::bindTexture(skyShader, "sky_texture", skyTBO, 3);

    // Рисуем небо: 6 квадратов (24 вершины)
    glDrawArrays(GL_TRIANGLES, 0, 36);						                         CHECK_GL_ERRORS
	
    // Отсоединяем VAO
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    // Отключаем шейдер
    glUseProgram(0);                                                             CHECK_GL_ERRORS
}

// Рисование травы
void DrawGrass() {
    // Тут то же самое, что и в рисовании земли
    glUseProgram(grassShader);                                                   CHECK_GL_ERRORS
    GLint cameraLocation = glGetUniformLocation(grassShader, "camera");          CHECK_GL_ERRORS
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS

    glBindVertexArray(grassVAO);                                                 CHECK_GL_ERRORS
    // Обновляем смещения для травы
	BindGrassVariance();
	// Привязываем текстуру
	GL::bindTexture(grassShader, "grass_texture", grassTBO, 1);	

	switch (key_pressed)
	{
	case 1:
		glDrawArraysInstanced(GL_TRIANGLES, 0, grassPointsCount, GRASS_INSTANCES);   CHECK_GL_ERRORS
		break;
	case 2:
		glPointSize(4.0f);
		glLineWidth(2.0f);
		glDrawArraysInstanced(GL_LINE_STRIP, 0, grassPointsCount, GRASS_INSTANCES);   CHECK_GL_ERRORS
		glDrawArraysInstanced(GL_POINTS, 0, grassPointsCount, GRASS_INSTANCES);   CHECK_GL_ERRORS
		break;
	}

    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glUseProgram(0);                                                             CHECK_GL_ERRORS
}

// Рисование камней
void DrawRock() {
    glUseProgram(rockShader);                                                    CHECK_GL_ERRORS
    GLint cameraLocation = glGetUniformLocation(rockShader, "camera");           CHECK_GL_ERRORS
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS

    glBindVertexArray(rockVAO);                                                  CHECK_GL_ERRORS

	// Привязываем текстуру
	GL::bindTexture(rockShader, "rock_texture", rockTBO, 4);

	
	glDrawArraysInstanced(GL_TRIANGLES, 0, rockPointsCount, ROCK_INSTANCES);     CHECK_GL_ERRORS

    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glUseProgram(0);                                                             CHECK_GL_ERRORS
}

// Рисование цветов
void DrawFlower() {
    // Тут то же самое, что и в рисовании земли
    glUseProgram(flowerShader);                                                     CHECK_GL_ERRORS
    GLint cameraLocation = glGetUniformLocation(flowerShader, "camera");            CHECK_GL_ERRORS
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS

    glBindVertexArray(flowerVAO);                                                   CHECK_GL_ERRORS
    // Обновляем смещения для травы
	BindFlowerVariance();
	// Привязываем текстуру
	//GL::bindTexture(flowerShader, "flower_texture", grassTBO, 1);

	glDrawArraysInstanced(GL_TRIANGLES, 0, flowerPointsCount, FLOWER_INSTANCES);    CHECK_GL_ERRORS

    glBindVertexArray(0);															CHECK_GL_ERRORS
    glUseProgram(0);																CHECK_GL_ERRORS
}

// Эта функция вызывается для обновления экрана
void RenderLayouts() {
    // Включение буфера глубины
    glEnable(GL_DEPTH_TEST); 
	// Включение мультисэмпинга
	//glEnable(GL_MULTISAMPLE);
	// Очистка буфера глубины и цветового буфера
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Рисуем меши
    DrawGrass();
    DrawGround();
	DrawSky();
	DrawFlower();
	DrawRock();
    glutSwapBuffers();
}

// Завершение программы
void FinishProgram() {
    glutDestroyWindow(glutGetWindow());
}

// Обработка события нажатия клавиши (специальные клавиши обрабатываются в функции SpecialButtons)
void KeyboardEvents(unsigned char key, int x, int y) {
	switch (key)
	{
	case 212:
	case 'A':
		MSflag = !MSflag;
		if (MSflag)
			glEnable(GL_MULTISAMPLE);
		else
			glDisable(GL_MULTISAMPLE);		
		break;
	case 27:
		FinishProgram();
		break;
	case 'w':
	case 246: // 'ц'
		camera.goForward();
		break;
	case 's': // 'ы'
	case 251:
		camera.goBack();
		break;
	case 'm':
	case 252: // 'ь'
		captureMouse = !captureMouse;
		if (captureMouse) {
			glutWarpPointer(screenWidth / 2, screenHeight / 2);
			glutSetCursor(GLUT_CURSOR_NONE);
		}
		else 
			glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);
		break;
	case '1':
		key_pressed = 1;
		break;
	case '2':
		key_pressed = 2;
		break;
	case 'W':
	case 214: //'Ц'
		windflag = !windflag;
		break;
	}
}

// Обработка события нажатия специальных клавиш
void SpecialButtons(int key, int x, int y) {
	switch (key)
	{
	case GLUT_KEY_RIGHT:
		camera.rotateY((const float)0.02);
		break;
	case GLUT_KEY_LEFT:
		camera.rotateY((const float)-0.02);
		break;
	case GLUT_KEY_UP:
		camera.rotateTop((const float)-0.02);
		break;
	case GLUT_KEY_DOWN:
		camera.rotateTop((const float)0.02);
		break;
	}
}

// Перерисовка экрана
void IdleFunc() {
    glutPostRedisplay();
}

// Обработка события движения мыши
void MouseMove(int x, int y) {

    if (captureMouse || pressedMouse) {
        
		if (!pressedMouse && captureMouse) {
			//cout << "delta x = " << x - centerX << endl;
			//cout << "delta y = " << x - centerX << endl;
			//cout << endl;

			if (x != centerX || y != centerY) {				
				camera.rotateY((float)(x - centerX) / 1000.0f);
				camera.rotateTop((float)(y - centerY) / 1000.0f);
				glutWarpPointer(centerX, centerY);				
			}
		} else {
			if (x != prevX || y != prevY) {
				camera.rotateY((float)(prevX - x) / 1000.0f);
				camera.rotateTop((float)(prevY - y) / 1000.0f);
			}
			prevX = x;
			prevY = y;
        }
    }
}

// Обработка нажатия кнопки мыши
void MouseClick(int button, int state, int x, int y) {
	prevX = x;
	prevY = y;
	pressedMouse = button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && !captureMouse;
}

// Событие изменение размера окна
void ReshapeFunc(GLint newWidth, GLint newHeight) {
    /*glViewport(0, 0, newWidth, newHeight);
    screenWidth = newWidth;
    screenHeight = newHeight;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    camera.screenRatio = (GLfloat)screenWidth / (GLfloat)screenHeight;
	gluOrtho2D(0, (GLdouble)screenWidth, 0, (GLdouble)screenHeight);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();*/
	glViewport(0, 0, newWidth, newHeight);
	screenWidth = newWidth;
	screenHeight = newHeight;
	camera.screenRatio = (GLfloat)screenWidth / (GLfloat)screenHeight;
}

// Инициализация окна
void InitializeGLUT(int argc, char **argv) {

    glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_MULTISAMPLE);	
    glutInitContextVersion(3, 3);
    glutInitWindowPosition(120, 30);
    glutInitWindowSize(screenWidth, screenHeight);
    glutCreateWindow("Computer Graphics 3");
    glutWarpPointer(centerX, centerY);

    glutDisplayFunc(RenderLayouts);
    glutKeyboardFunc(KeyboardEvents);
    glutSpecialFunc(SpecialButtons);	
    glutIdleFunc(IdleFunc);
    glutPassiveMotionFunc(MouseMove);
	glutMotionFunc(MouseMove);
    glutMouseFunc(MouseClick);
    glutReshapeFunc(ReshapeFunc);
	glutTimerFunc(speed, Timer, 0);
}

// Генератор рандомых чисел в указанном диапазоне
float random(int a, int b)
{
	if (a > 0) return (float)(a + rand() % (b - a));
	else return (float)(a + rand() % (abs(a) + b));
}
// Генерация позиций травинок

vector<VM::vec2> GeneratePositions(uint n) {
    vector<VM::vec2> grassPositions(n);

    for (uint i = 0; i < n; ++i)
		grassPositions[i] = VM::vec2((float)rand() / RAND_MAX + 0.02f, (float)rand() / RAND_MAX + 0.02f);
    
    return grassPositions;
}

// Рандомный размер для каждой травинки
vector<float> GenSize(uint n) {

	vector<float> sizes(n);
	for (uint i = 0; i < n; i++)
		sizes[i] = random(70, 100) / 100;	

	return sizes;
}

// Рандомный угол для каждой травинки
vector<float> GenAngle(uint n) {

	vector<float> angles(n);
	for (uint i = 0; i < n; i++)
		angles[i] = random(0, 360) / 180 * M_PI;

	return angles;
}

// 1 - травинка желтая, 0 - иначе
vector<float> GenYellow(uint n) {
	vector<float> angles(n);

	for (uint i = 0; i < n; i++)
		angles[i] = random(0, 100) / 100;

	return angles;
}

// Меш травы
vector<VM::vec4> GenMeshGrass() {
	VM::vec4 p1 = VM::vec4(0     , 0      , 0    , 1);
	VM::vec4 p2 = VM::vec4(10.f/10, 0      , 0    , 1);
	VM::vec4 p3 = VM::vec4(2.f/ 10, 6.f/20  , 0.001f, 1);
	VM::vec4 p4 = VM::vec4(8.f/ 10, 6.f/20  , 0.001f, 1);
	VM::vec4 p5 = VM::vec4(3.f/ 10, 12.f/20 , 0.004f, 1);
	VM::vec4 p6 = VM::vec4(7.f/ 10, 12.f/20 , 0.004f, 1);
	VM::vec4 p7 = VM::vec4(5.f/ 10, 20.f/20 , 0.012f, 1);
	
	return 
	{	p3, p1, p2,
		p3, p2, p4,
		p3, p5, p4,
		p5, p4, p6,
		p7, p5, p6
	};
}

// Координаты текстуры травы
vector<VM::vec2> GenTexGrass() {
	VM::vec2 p1 = VM::vec2(0, 0);
	VM::vec2 p2 = VM::vec2(10.f / 10, 0);
	VM::vec2 p3 = VM::vec2(2.f / 10, 3.f / 10);
	VM::vec2 p4 = VM::vec2(8.f / 10, 3.f / 10);
	VM::vec2 p5 = VM::vec2(3.f / 10, 6.f / 10);
	VM::vec2 p6 = VM::vec2(9.f / 10, 6.f / 10);
	VM::vec2 p7 = VM::vec2(5.f / 10, 10.f / 10);

	return
	{	p3, p1, p2,
		p3, p2, p4,
		p3, p5, p4,
		p5, p4, p6,
		p7, p5, p6
	};
}

// Создание травы
void CreateGrass() {
	//*************ПОДГОТОВКА**************
	// Создаём позиции для травинок
    vector<VM::vec2> grassPositions = GeneratePositions(GRASS_INSTANCES);
	
	// Создаём размер и форму для травинок
	vector<float> grassSize = GenSize(GRASS_INSTANCES);

	// Создаём углы поворота для травинок
	vector<float> grassAngle = GenAngle(GRASS_INSTANCES);

	// Создаём углы поворота для травинок
	vector<float> grassYellow = GenYellow(GRASS_INSTANCES);
	
	// Инициализация смещений для травинок
    for (uint i = 0; i < GRASS_INSTANCES; ++i)  
        grassVarianceData[i] = VM::vec4(0, 0, 0, 0); 

	grassVelocity = 0;

    // Создаём меш
    vector<VM::vec4> grassPoints = GenMeshGrass();

	// Создаём координаты для текстуры
	vector<VM::vec2> grassTexture = GenTexGrass();

    // Сохраняем количество вершин в меше травы
    grassPointsCount = grassPoints.size();

	grassShader = GL::CompileShaderProgram("grass");

	GL::loadTexture(grassTBO, "Texture/grass.jpg");	

	//***********ВЕСЕЛУХА************
	/* Компилируем шейдеры
    Эта функция принимает на вход название шейдера 'shaderName',
    читает файлы shaders/{shaderName}.vert - вершинный шейдер
    и shaders/{shaderName}.frag - фрагментный шейдер,
    компилирует их и линкует*/
	

	// Создание и привязка VAO
    glGenVertexArrays(1, &grassVAO);													CHECK_GL_ERRORS
    glBindVertexArray(grassVAO);														CHECK_GL_ERRORS

	// Здесь создаём буфер
    GLuint pointsBuffer[2];
	glGenBuffers(2, pointsBuffer);														CHECK_GL_ERRORS	

		/*************МЕШ*************/
	// Привязываем сгенерированный буфер
	glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer[0]);										CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * grassPoints.size(), grassPoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	// Получение локации параметра 'point' в шейдере
    GLuint pointsID = glGetAttribLocation(grassShader, "point");						CHECK_GL_ERRORS
    // Подключаем массив атрибутов к данной локации
    glEnableVertexAttribArray(pointsID);												CHECK_GL_ERRORS
    // Устанавливаем параметры для получения данных из массива (по 4 значение типа float на одну вершину)
    glVertexAttribPointer(pointsID, 4, GL_FLOAT, GL_FALSE, 0, 0);						CHECK_GL_ERRORS

		/***********ТЕКСТУРА*************/
	// Заносим текстуру в буфер
	glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer[1]);										CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * grassTexture.size(), grassTexture.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
	
	// Даем инструкции переменной в шейдере 'in_texture_coord' как читать текстуру
	GLuint textureID = glGetAttribLocation(grassShader, "texture_coord");				CHECK_GL_ERRORS
	glEnableVertexAttribArray(textureID);												CHECK_GL_ERRORS
	glVertexAttribPointer(textureID, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);		CHECK_GL_ERRORS

		/***********СМЕЩЕНИЯ*************/
    glGenBuffers(1, &grassVariance);													CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, grassVariance);										CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * GRASS_INSTANCES, grassVarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	GLuint varianceLocation = glGetAttribLocation(grassShader, "variance");				CHECK_GL_ERRORS
    glEnableVertexAttribArray(varianceLocation);										CHECK_GL_ERRORS
    glVertexAttribPointer(varianceLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);				CHECK_GL_ERRORS
	glVertexAttribDivisorARB(varianceLocation, 1);											CHECK_GL_ERRORS


		/***********ПОЗИЦИИ*************/    
    GLuint positionID;
    glGenBuffers(1, &positionID);														CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, positionID);											CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * GRASS_INSTANCES, grassPositions.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	GLuint positionLocation = glGetAttribLocation(grassShader, "position");				CHECK_GL_ERRORS
    glEnableVertexAttribArray(positionLocation);										CHECK_GL_ERRORS
    glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);				CHECK_GL_ERRORS
	glVertexAttribDivisorARB(positionLocation, 1);											CHECK_GL_ERRORS

		/*************РАЗМЕР*************/
	GLuint sizeID;
	glGenBuffers(1, &sizeID);															CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, sizeID);												CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * GRASS_INSTANCES, grassSize.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	GLuint sizeLocation = glGetAttribLocation(grassShader, "size");						CHECK_GL_ERRORS
    glEnableVertexAttribArray(sizeLocation);											CHECK_GL_ERRORS
    glVertexAttribPointer(sizeLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);					CHECK_GL_ERRORS
	glVertexAttribDivisorARB(sizeLocation, 1);												CHECK_GL_ERRORS

		/*************ПОВОРОТ*************/
	GLuint angleID;	
	glGenBuffers(1, &angleID);															CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, angleID);												CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * GRASS_INSTANCES, grassAngle.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	GLuint angleLocation = glGetAttribLocation(grassShader, "angle");					CHECK_GL_ERRORS
    glEnableVertexAttribArray(angleLocation);											CHECK_GL_ERRORS
    glVertexAttribPointer(angleLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);					CHECK_GL_ERRORS
	glVertexAttribDivisorARB(angleLocation, 1);											CHECK_GL_ERRORS

		/*************ПОЖЕЛТЕНИЕ*************/
	GLuint yellowID;	
	glGenBuffers(1, &yellowID);															CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, yellowID);											CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * GRASS_INSTANCES, grassYellow.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	GLuint yellowLocation = glGetAttribLocation(grassShader, "yellow");					CHECK_GL_ERRORS
    glEnableVertexAttribArray(yellowLocation);											CHECK_GL_ERRORS
    glVertexAttribPointer(yellowLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);					CHECK_GL_ERRORS
	glVertexAttribDivisorARB(yellowLocation, 1);										CHECK_GL_ERRORS
	
    // Отвязываем VAO
    glBindVertexArray(0);																CHECK_GL_ERRORS
    // Отвязываем буфер
    glBindBuffer(GL_ARRAY_BUFFER, 0);													CHECK_GL_ERRORS
}

// Меш травы
vector<VM::vec4> GenMeshRock() {
	VM::vec4 p1  = VM::vec4(3 , 0, 0 , 1) / 10;
	VM::vec4 p2  = VM::vec4(1 , 0, 1 , 1) / 10;
	VM::vec4 p3  = VM::vec4(5 , 0, 1 , 1) / 10;
	VM::vec4 p4  = VM::vec4(2 , 3, 2 , 1) / 10;
	VM::vec4 p5  = VM::vec4(4 , 3, 2 , 1) / 10;
	VM::vec4 p6  = VM::vec4(0 , 0, 3 , 1) / 10;
	VM::vec4 p7  = VM::vec4(4 , 4, 3 , 1) / 10;
	VM::vec4 p8  = VM::vec4(2 , 3, 4 , 1) / 10;
	VM::vec4 p9  = VM::vec4(3 , 4, 4 , 1) / 10;
	VM::vec4 p10 = VM::vec4(1 , 0, 5 , 1) / 10;
	VM::vec4 p11 = VM::vec4(9 , 0, 5 , 1) / 10;
	VM::vec4 p12 = VM::vec4(7 , 4, 6 , 1) / 10;
	VM::vec4 p13 = VM::vec4(8 , 3, 6 , 1) / 10;
	VM::vec4 p14 = VM::vec4(6 , 4, 7 , 1) / 10;
	VM::vec4 p15 = VM::vec4(10, 0, 7 , 1) / 10;
	VM::vec4 p16 = VM::vec4(6 , 3, 8 , 1) / 10;
	VM::vec4 p17 = VM::vec4(8 , 3, 8 , 1) / 10;
	VM::vec4 p18 = VM::vec4(5 , 0, 9 , 1) / 10;
	VM::vec4 p19 = VM::vec4(9 , 0, 9 , 1) / 10;
	VM::vec4 p20 = VM::vec4(7 , 0, 10, 1) / 10;
	
	return 
	{	p1, p2, p4,
		p2, p6, p4,
		p6, p8, p4,
		p1, p5, p4,
		p6, p10, p8,
		p1, p5, p3,
		p4, p8, p9,
		p4, p9, p7,
		p4, p5, p7,
		p10, p8, p18,
		p8, p18, p16,
		p8, p9, p16,
		p9, p16, p14,
		p9, p7, p14,
		p7, p14, p12,
		p7, p5, p12,
		p5, p12, p13,
		p5, p3, p13,
		p3, p13, p11,
		p18, p20, p16,
		p16, p20, p17,
		p16, p14, p17,
		p14, p17, p12,
		p12, p13, p17,
		p13, p17, p15,
		p11, p13, p15,
		p17, p20, p19,
		p17, p19, p15,
	};
}

// Координаты текстуры травы
vector<VM::vec2> GenTexRock() {

	VM::vec2 p1  = VM::vec2(3 ,  0 ) / 10;
	VM::vec2 p2  = VM::vec2(1 ,  1 ) / 10;
	VM::vec2 p3  = VM::vec2(5 ,  1 ) / 10;
	VM::vec2 p4  = VM::vec2(2 ,  2 ) / 10;
	VM::vec2 p5  = VM::vec2(4 ,  2 ) / 10;
	VM::vec2 p6  = VM::vec2(0 ,  3 ) / 10;
	VM::vec2 p7  = VM::vec2(4 ,  3 ) / 10;
	VM::vec2 p8  = VM::vec2(2 ,  4 ) / 10;
	VM::vec2 p9  = VM::vec2(3 ,  4 ) / 10;
	VM::vec2 p10 = VM::vec2(1 ,  5 ) / 10;
	VM::vec2 p11 = VM::vec2(9 ,  5 ) / 10;
	VM::vec2 p12 = VM::vec2(7 ,  6 ) / 10;
	VM::vec2 p13 = VM::vec2(8 ,  6 ) / 10;
	VM::vec2 p14 = VM::vec2(6 ,  7 ) / 10;
	VM::vec2 p15 = VM::vec2(10,  7 ) / 10;
	VM::vec2 p16 = VM::vec2(6 ,  8 ) / 10;
	VM::vec2 p17 = VM::vec2(8 ,  8 ) / 10;
	VM::vec2 p18 = VM::vec2(5 ,  9 ) / 10;
	VM::vec2 p19 = VM::vec2(9 ,  9 ) / 10;
	VM::vec2 p20 = VM::vec2(7 ,  10) / 10;
	
	return 
	{	p1, p2, p4,
		p2, p6, p4,
		p6, p8, p4,
		p1, p5, p4,
		p6, p10, p8,
		p1, p5, p3,
		p4, p8, p9,
		p4, p9, p7,
		p4, p5, p7,
		p10, p8, p18,
		p8, p18, p16,
		p8, p9, p16,
		p9, p16, p14,
		p9, p7, p14,
		p7, p14, p12,
		p7, p5, p12,
		p5, p12, p13,
		p5, p3, p13,
		p3, p13, p11,
		p18, p20, p16,
		p16, p20, p17,
		p16, p14, p17,
		p14, p17, p12,
		p12, p13, p17,
		p13, p17, p15,
		p11, p13, p15,
		p17, p20, p19,
		p17, p19, p15,
	};
}

// Создание травы
void CreateRock() {
	//*************ПОДГОТОВКА**************
	// Создаём позиции для травинок
    vector<VM::vec2> rockPositions = GeneratePositions(ROCK_INSTANCES);
	
	// Создаём размер и форму для травинок
	vector<float> rockSize = GenSize(ROCK_INSTANCES);

	// Создаём углы поворота для травинок
	vector<float> rockAngle = GenAngle(ROCK_INSTANCES);

    // Создаём меш
    vector<VM::vec4> rockPoints = GenMeshRock();

	// Создаём координаты для текстуры
	vector<VM::vec2> rockTexture = GenTexRock();

    // Сохраняем количество вершин в меше травы
    rockPointsCount = rockPoints.size();

	rockShader = GL::CompileShaderProgram("rock");

	GL::loadTexture(rockTBO, "Texture/rock.jpg");	

	//***********ВЕСЕЛУХА************
	/* Компилируем шейдеры
    Эта функция принимает на вход название шейдера 'shaderName',
    читает файлы shaders/{shaderName}.vert - вершинный шейдер
    и shaders/{shaderName}.frag - фрагментный шейдер,
    компилирует их и линкует*/
	

	// Создание и привязка VAO
    glGenVertexArrays(1, &rockVAO);													CHECK_GL_ERRORS
    glBindVertexArray(rockVAO);														CHECK_GL_ERRORS

	// Здесь создаём буфер
    GLuint pointsBuffer[2];
	glGenBuffers(2, pointsBuffer);														CHECK_GL_ERRORS	

		/*************МЕШ*************/
	// Привязываем сгенерированный буфер
	glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer[0]);										CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * rockPoints.size(), rockPoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	// Получение локации параметра 'point' в шейдере
    GLuint pointsID = glGetAttribLocation(rockShader, "point");						CHECK_GL_ERRORS
    // Подключаем массив атрибутов к данной локации
    glEnableVertexAttribArray(pointsID);												CHECK_GL_ERRORS
    // Устанавливаем параметры для получения данных из массива (по 4 значение типа float на одну вершину)
    glVertexAttribPointer(pointsID, 4, GL_FLOAT, GL_FALSE, 0, 0);						CHECK_GL_ERRORS

		/***********ТЕКСТУРА*************/
	// Заносим текстуру в буфер
	glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer[1]);										CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * rockTexture.size(), rockTexture.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
	
	// Даем инструкции переменной в шейдере 'in_texture_coord' как читать текстуру
	GLuint textureID = glGetAttribLocation(rockShader, "texture_coord");				CHECK_GL_ERRORS
	glEnableVertexAttribArray(textureID);												CHECK_GL_ERRORS
	glVertexAttribPointer(textureID, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);		CHECK_GL_ERRORS
		
		/***********ПОЗИЦИИ*************/    
    GLuint positionID;
    glGenBuffers(1, &positionID);														CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, positionID);											CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * ROCK_INSTANCES, rockPositions.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	GLuint positionLocation = glGetAttribLocation(rockShader, "position");				CHECK_GL_ERRORS
    glEnableVertexAttribArray(positionLocation);										CHECK_GL_ERRORS
    glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);				CHECK_GL_ERRORS
	glVertexAttribDivisorARB(positionLocation, 1);											CHECK_GL_ERRORS

		/*************РАЗМЕР*************/
	GLuint sizeID;
	glGenBuffers(1, &sizeID);															CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, sizeID);												CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * ROCK_INSTANCES, rockSize.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	GLuint sizeLocation = glGetAttribLocation(rockShader, "size");						CHECK_GL_ERRORS
    glEnableVertexAttribArray(sizeLocation);											CHECK_GL_ERRORS
    glVertexAttribPointer(sizeLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);					CHECK_GL_ERRORS
	glVertexAttribDivisorARB(sizeLocation, 1);												CHECK_GL_ERRORS

		/*************ПОВОРОТ*************/
	GLuint angleID;	
	glGenBuffers(1, &angleID);															CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, angleID);												CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * ROCK_INSTANCES, rockAngle.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	GLuint angleLocation = glGetAttribLocation(rockShader, "angle");					CHECK_GL_ERRORS
    glEnableVertexAttribArray(angleLocation);											CHECK_GL_ERRORS
    glVertexAttribPointer(angleLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);					CHECK_GL_ERRORS
	glVertexAttribDivisorARB(angleLocation, 1);											CHECK_GL_ERRORS
	
    // Отвязываем VAO
    glBindVertexArray(0);																CHECK_GL_ERRORS
    // Отвязываем буфер
    glBindBuffer(GL_ARRAY_BUFFER, 0);													CHECK_GL_ERRORS
}

// Меш цветка
vector<VM::vec4> GenMeshFlower() {
	VM::vec4 p1  = VM::vec4(0 , 21.5f,  0, 20) / 20;
	VM::vec4 p2  = VM::vec4(20, 21.5f,  0, 20) / 20;

	VM::vec4 p3  = VM::vec4(7 , 21,  5, 20) / 20;
	VM::vec4 p4  = VM::vec4(13, 21,  5, 20) / 20;

	VM::vec4 p5  = VM::vec4(5 , 21,  7, 20) / 20;
	VM::vec4 p6  = VM::vec4(10, 20,  8, 20) / 20;
	VM::vec4 p7  = VM::vec4(15, 21,  7, 20) / 20;

	VM::vec4 p8  = VM::vec4(8 , 20, 10, 20) / 20;
	VM::vec4 p9  = VM::vec4(12, 20, 10, 20) / 20;

	VM::vec4 p10 = VM::vec4(5 , 21, 13, 20) / 20;
	VM::vec4 p11 = VM::vec4(10, 20, 12, 20) / 20;
	VM::vec4 p12 = VM::vec4(15, 21, 13, 20) / 20;

	VM::vec4 p13 = VM::vec4(7 , 21, 15, 20) / 20;
	VM::vec4 p14 = VM::vec4(13, 21, 15, 20) / 20;

	VM::vec4 p15 = VM::vec4(0 , 21.5f, 20, 20) / 20;
	VM::vec4 p16 = VM::vec4(20, 21.5f, 20, 20) / 20;

	VM::vec4 p17 = VM::vec4(9.5 , 17, 10  , 20) / 20;//8
	VM::vec4 p18 = VM::vec4(10  , 17, 9.5 , 20) / 20;//6
	VM::vec4 p19 = VM::vec4(10.5, 17, 10  , 20) / 20;//9
	VM::vec4 p20 = VM::vec4(10  , 17, 10.5, 20) / 20;//11

	VM::vec4 p21 = VM::vec4(9.5  , 0, 10, 20) / 20;//8
	VM::vec4 p22 = VM::vec4(10, 0, 9.5  , 20) / 20;//6
	VM::vec4 p23 = VM::vec4(10.5 , 0, 10, 20) / 20;//9
	VM::vec4 p24 = VM::vec4(10, 0, 10.5 , 20) / 20;//11

	return
	{	//лепестки
		p1, p3, p5,
		p5, p3, p8,
		p3, p6, p8,

		p2, p7, p4,
		p4, p7, p6,
		p6, p9, p7,

		p16, p14, p12,
		p12, p14, p9,
		p11, p14, p9,

		p15, p10, p13,
		p10, p13, p11,
		p10, p11, p8,

		// сердцевина
		p8, p11, p9,
		p9, p8, p6,

		// стебель
		p17, p8, p6,
		p17, p18, p6,

		p6, p18, p9,
		p9, p18, p19,

		p9, p19, p11,
		p19, p11, p20,

		p11, p20, p8,
		p8, p20, p17,

		// стебель 2
		p17, p21, p22,
		p17, p18, p22,

		p22, p18, p23,
		p23, p18, p19,

		p23, p19, p24,
		p19, p24, p20,

		p24, p20, p21,
		p21, p20, p17,
	};
}

// Создание цветка
void CreateFlower() {
	//*************ПОДГОТОВКА**************
	// Создаём позиции для цветов
    vector<VM::vec2> flowerPositions = GeneratePositions(FLOWER_INSTANCES);

	// Инициализация смещений для цветов
    for (uint i = 0; i < FLOWER_INSTANCES; ++i)
        flowerVarianceData[i] = VM::vec4(0, 0, 0, 0);  

	flowerVelocity = 0;

    // Создаём меш
    vector<VM::vec4> flowerPoints = GenMeshFlower();

	// Создаём координаты для текстуры
	//vector<VM::vec2> flowerTexture = GenTexflower();

	// Создаём размер и форму для цветов
	vector<float> flowerSize = GenSize(FLOWER_INSTANCES);

	// Создаём углы поворота для цветов
	vector<float> flowerAngle = GenAngle(FLOWER_INSTANCES);

    // Сохраняем количество вершин в меше травы
    flowerPointsCount = flowerPoints.size();

	flowerShader = GL::CompileShaderProgram("flower");

	//GL::loadTexture(flowerTBO, "Texture/flower.jpg");	

	//***********ВЕСЕЛУХА************
	/* Компилируем шейдеры
    Эта функция принимает на вход название шейдера 'shaderName',
    читает файлы shaders/{shaderName}.vert - вершинный шейдер
    и shaders/{shaderName}.frag - фрагментный шейдер,
    компилирует их и линкует*/
	

	// Создание и привязка VAO
    glGenVertexArrays(1, &flowerVAO);													CHECK_GL_ERRORS
    glBindVertexArray(flowerVAO);														CHECK_GL_ERRORS

	// Здесь создаём буфер
    GLuint pointsBuffer[2];
	glGenBuffers(2, pointsBuffer);														CHECK_GL_ERRORS	

		/*************МЕШ*************/
	// Привязываем сгенерированный буфер
	glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer[0]);										CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * flowerPoints.size(), flowerPoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	// Получение локации параметра 'point' в шейдере
    GLuint pointsID = glGetAttribLocation(flowerShader, "point");						CHECK_GL_ERRORS
    // Подключаем массив атрибутов к данной локации
    glEnableVertexAttribArray(pointsID);												CHECK_GL_ERRORS
    // Устанавливаем параметры для получения данных из массива (по 4 значение типа float на одну вершину)
    glVertexAttribPointer(pointsID, 4, GL_FLOAT, GL_FALSE, 0, 0);						CHECK_GL_ERRORS

		/***********ТЕКСТУРА*************/
	// Заносим текстуру в буфер
	/*glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer[1]);										CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * flowerTexture.size(), flowerTexture.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
	
	// Даем инструкции переменной в шейдере 'in_texture_coord' как читать текстуру
	GLuint textureID = glGetAttribLocation(flowerShader, "texture_coord");				CHECK_GL_ERRORS
	glEnableVertexAttribArray(textureID);												CHECK_GL_ERRORS
	glVertexAttribPointer(textureID, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);		CHECK_GL_ERRORS*/

		/***********СМЕЩЕНИЯ*************/
    glGenBuffers(1, &flowerVariance);													CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, flowerVariance);										CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * FLOWER_INSTANCES, flowerVarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	GLuint varianceLocation = glGetAttribLocation(flowerShader, "variance");			CHECK_GL_ERRORS
    glEnableVertexAttribArray(varianceLocation);										CHECK_GL_ERRORS
    glVertexAttribPointer(varianceLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);				CHECK_GL_ERRORS
	glVertexAttribDivisorARB(varianceLocation, 1);										CHECK_GL_ERRORS


		/***********ПОЗИЦИИ*************/    
    GLuint positionID;
    glGenBuffers(1, &positionID);														CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, positionID);											CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * FLOWER_INSTANCES, flowerPositions.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	GLuint positionLocation = glGetAttribLocation(flowerShader, "position");			CHECK_GL_ERRORS
    glEnableVertexAttribArray(positionLocation);										CHECK_GL_ERRORS
    glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);				CHECK_GL_ERRORS
	glVertexAttribDivisorARB(positionLocation, 1);										CHECK_GL_ERRORS

		/*************РАЗМЕР*************/
	GLuint sizeID;
	glGenBuffers(1, &sizeID);															CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, sizeID);												CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * FLOWER_INSTANCES, flowerSize.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	GLuint sizeLocation = glGetAttribLocation(flowerShader, "size");					CHECK_GL_ERRORS
    glEnableVertexAttribArray(sizeLocation);											CHECK_GL_ERRORS
    glVertexAttribPointer(sizeLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);					CHECK_GL_ERRORS
	glVertexAttribDivisorARB(sizeLocation, 1);											CHECK_GL_ERRORS

		/*************ПОВОРОТ*************/
	GLuint angleID;	
	glGenBuffers(1, &angleID);															CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, angleID);												CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * FLOWER_INSTANCES, flowerAngle.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	GLuint angleLocation = glGetAttribLocation(flowerShader, "angle");					CHECK_GL_ERRORS
    glEnableVertexAttribArray(angleLocation);											CHECK_GL_ERRORS
    glVertexAttribPointer(angleLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);					CHECK_GL_ERRORS
	glVertexAttribDivisorARB(angleLocation, 1);											CHECK_GL_ERRORS
	
    // Отвязываем VAO
    glBindVertexArray(0);																CHECK_GL_ERRORS
    // Отвязываем буфер
    glBindBuffer(GL_ARRAY_BUFFER, 0);													CHECK_GL_ERRORS
}

// Создаём замлю
void CreateGround() {
	// Земля состоит из двух треугольников 
	vector<VM::vec4> mesh = {
		VM::vec4(0, 0, 0, 1),
		VM::vec4(1, 0, 0, 1),
		VM::vec4(1, 0, 1, 1),

		VM::vec4(0, 0, 0, 1),
		VM::vec4(1, 0, 1, 1),
		VM::vec4(0, 0, 1, 1),
	};
	
	// Координаты в текстуре
	vector<VM::vec2> texture = {
		VM::vec2(0, 0),
		VM::vec2(1, 0),
		VM::vec2(1, 1),

		VM::vec2(0, 0),
		VM::vec2(1, 1),
		VM::vec2(0, 1),
	};

	// Шейдер отрисовки земли
	groundShader = GL::CompileShaderProgram("ground");

	GL::loadTexture(groundTBO, "Texture/ground.jpg");


	// Генерируем 2 буфера
	GLuint pointsBuffer[2];
	glGenBuffers(2, pointsBuffer);														CHECK_GL_ERRORS

	// Создание и генерация VAO
	glGenVertexArrays(1, &groundVAO);													CHECK_GL_ERRORS
	glBindVertexArray(groundVAO);														CHECK_GL_ERRORS

		/***********ТОЧКИ_МЕША*************/
	// Заполняем первый буфер точками меша
	glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer[0]);										CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * mesh.size(), mesh.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	// Даем инструкции переменной в шейдере 'point' как читать точки меша
	GLuint pointID = glGetAttribLocation(groundShader, "point");						CHECK_GL_ERRORS		
	glEnableVertexAttribArray(pointID);													CHECK_GL_ERRORS
	glVertexAttribPointer(pointID, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);		CHECK_GL_ERRORS

		/***********ТОЧКИ_ТЕКСТУРЫ*************/
	// Заносим координаты текстуры в буфер
	glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer[1]);										CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * texture.size(), texture.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	// Даем инструкции переменной в шейдере 'in_texture_coord' как читать текстуру
	GLuint textureID = glGetAttribLocation(groundShader, "in_texture_ground_coord");	CHECK_GL_ERRORS		
	glEnableVertexAttribArray(textureID);												CHECK_GL_ERRORS
	glVertexAttribPointer(textureID, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);		CHECK_GL_ERRORS

	glBindVertexArray(0);																CHECK_GL_ERRORS
	glBindBuffer(GL_ARRAY_BUFFER, 0);													CHECK_GL_ERRORS
}

// Меш скайбокса
vector<VM::vec4> GenSky() {
	float size = 2;
	VM::vec4 p1 = VM::vec4( size, size,  size, size);
	VM::vec4 p2 = VM::vec4(-size, size,  size, size);
	VM::vec4 p3 = VM::vec4(-size, size, -size, size);
	VM::vec4 p4 = VM::vec4( size, size, -size, size);

	VM::vec4 p5 = VM::vec4( size,  -size,  size, size);
	VM::vec4 p6 = VM::vec4(-size,  -size,  size, size);
	VM::vec4 p7 = VM::vec4(-size,  -size, -size, size);
	VM::vec4 p8 = VM::vec4( size,  -size, -size, size);

	return
	{   p1, p2, p3,
		p1, p4, p3,
		
		p2, p6, p7,
		p2, p3, p7,

		p3, p7, p8,
		p3, p4, p8,

		p4, p1, p5,
		p4, p8, p5,

		p1, p2, p6,
		p1, p5, p6,

		p5, p6, p7,
		p5, p8, p7,
	};
}

// Координаты текстуры скайбокса
vector<VM::vec2> GenTexSky() {
	const float shift = 0.00025f;
	VM::vec2 p1  = VM::vec2(1.0f / 4 + shift,        0 + shift);
	VM::vec2 p2  = VM::vec2(2.0f / 4 - shift,        0 + shift);

	VM::vec2 p3  = VM::vec2(0 + shift		, 1.0f / 3 + shift);
	VM::vec2 p4  = VM::vec2(1.0f / 4 + shift, 1.0f / 3 + shift);
	VM::vec2 p5  = VM::vec2(2.0f / 4 - shift, 1.0f / 3 + shift);
	VM::vec2 p6  = VM::vec2(3.0f / 4		, 1.0f / 3 + shift);
	VM::vec2 p7  = VM::vec2(1 - shift		, 1.0f / 3 + shift);

	VM::vec2 p8  = VM::vec2(0 + shift		, 2.0f / 3 - shift);
	VM::vec2 p9  = VM::vec2(1.0f / 4 + shift, 2.0f / 3 - shift);
	VM::vec2 p10 = VM::vec2(2.0f / 4 - shift, 2.0f / 3 - shift);
	VM::vec2 p11 = VM::vec2(3.0f / 4		, 2.0f / 3 - shift);
	VM::vec2 p12 = VM::vec2(1 - shift		, 2.0f / 3 - shift);

	VM::vec2 p13 = VM::vec2(1.0f / 4 + shift,        1 - shift);
	VM::vec2 p14 = VM::vec2(2.0f / 4 - shift,        1 - shift);

	return
	{   p1, p2, p5,
		p1, p4, p5,

		p6, p11, p10,
		p6, p5, p10,

		p5, p10, p9,
		p5, p4 , p9,

		p4, p3, p8,
		p4, p9, p8,

		p7, p6 , p11,
		p7, p12, p11,

		p13, p14, p10,
		p13, p9 , p10,
	};
}

// Создаём скайбокс
void CreateSky() {
	// Небо состоит из 6 квадратов 
	vector<VM::vec4> mesh = GenSky();
	
	// Координаты в текстуре
	vector<VM::vec2> texture = GenTexSky();

	// Шейдер отрисовки земли
	skyShader = GL::CompileShaderProgram("sky");

	GL::loadTexture(skyTBO, "Texture/skybox.jpg");

	// Генерируем 2 буфера
	GLuint pointsBuffer[2];
	glGenBuffers(2, pointsBuffer);														CHECK_GL_ERRORS

	// Создание и генерация VAO
	glGenVertexArrays(1, &skyVAO);														CHECK_GL_ERRORS
	glBindVertexArray(skyVAO);															CHECK_GL_ERRORS

		/***********ТОЧКИ_МЕША*************/
	// Заполняем первый буфер точками меша
	glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer[0]);										CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * mesh.size(), mesh.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	// Даем инструкции переменной в шейдере 'point' как читать точки меша
	GLuint pointID = glGetAttribLocation(skyShader, "point");							CHECK_GL_ERRORS		
	glEnableVertexAttribArray(pointID);													CHECK_GL_ERRORS
	glVertexAttribPointer(pointID, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);		CHECK_GL_ERRORS
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		/***********ТОЧКИ_ТЕКСТУРЫ*************/
	// Заносим координаты текстуры в буфер
	glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer[1]);										CHECK_GL_ERRORS
	glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * texture.size(), texture.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

	// Даем инструкции переменной в шейдере 'in_texture_coord' как читать текстуру
	GLuint textureID = glGetAttribLocation(skyShader, "in_texture_sky_coord");			CHECK_GL_ERRORS
	glEnableVertexAttribArray(textureID);												CHECK_GL_ERRORS
	glVertexAttribPointer(textureID, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);		CHECK_GL_ERRORS

	glBindVertexArray(0);																CHECK_GL_ERRORS
	glBindBuffer(GL_ARRAY_BUFFER, 0);													CHECK_GL_ERRORS
}

// Создаём камеру
void CreateCamera() {
    camera.angle = 45.0f / 180.0f * M_PI;
    camera.direction = VM::vec3(0, 0.3f, -1);
    camera.position = VM::vec3(0.5f, 0.4f, -0.5f);
    camera.screenRatio = (float)screenWidth / (float)screenHeight;
    camera.up = VM::vec3(0, 1, 0);
    camera.zfar = 50.0f;
    camera.znear = 0.05f;
}

int main(int argc, char **argv)
{
    try {
        cout << "Start" << endl;
        InitializeGLUT(argc, argv);
        cout << "GLUT inited" << endl;
        glewInit();
        cout << "glew inited" << endl;
        CreateCamera();
        cout << "Camera created" << endl;
        CreateGrass();
        cout << "Grass created" << endl;
        CreateGround();
        cout << "Ground created" << endl;
		CreateSky();
		cout << "Sky created" << endl;
		CreateFlower();
		cout << "Flower created" << endl;
		CreateRock();
		cout << "Rocks created" << endl;
        glutMainLoop();
    } catch (string s) {
        cout << s << endl;
    }
}