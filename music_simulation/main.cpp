

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/transform2.hpp>
#include <gtc/type_ptr.hpp>
#include <math.h>
#include <sstream>
#include <stdlib.h> 
#include "devil_cpp_wrapper.hpp"

using namespace std;

// global variable
/////////////////////////////////////////////////////////////////
// vao, vbo
unsigned int vao_line;
unsigned int vbo_line;
unsigned int vao_symbol;
unsigned int vbo_symbol;
unsigned int vao_symbol2;
unsigned int vbo_symbol2;



/////////////////////////////////////////////////////////////////
// shader program
unsigned int line_program;
unsigned int draw_program;
unsigned int draw_program2;
/////////////////////////////////////////////////////////////////
// data 
// line_vertices: the two vertices for a line
// symbol_data: the fixed vertices without dynamic change
// symbol_data2: the dynamic vertices
// output_data: the data we want to output
//////////////////////////////////////////////////////////////////
struct output_format
{
	bool beat;
	int texture_label;
	int beat_number;
	glm::vec2 position;
	float frequency;
};
////////////////////////////////////////////////////////////////////////

vector<output_format> output_data;
vector<float> line_vertices;
vector<glm::vec2> symbol_data;
vector<glm::vec2> symbol_data2;
/////////////////////////////////////////////////////////////////
// texture
// seperate textures into two category: with beats and without beats
vector<unsigned int> textures_wb;
vector<unsigned int> textures_nb;
/////////////////////////////////////////////////////////////////
// output
int number_screenshot = 0;
/////////////////////////////////////////////////////////////////
GLfloat vertices[] = { -1.0f,-1.0f,
1.0f,-1.0f,
-1.0f,1.0f,
1.0f,1.0f };

unsigned int vaoBrushPath;
unsigned int vboBrushPath;
unsigned int gaussianProgramID, brushProgramID;

int width, height;
const int Max_Points = 512;
std::vector<float> brushPath;
unsigned int brushTexture;


GLuint texture_load(const char * filename, unsigned int internalFormat)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	::ilLoadImage(filename);
	int imageWidth = ilGetInteger(IL_IMAGE_WIDTH);
	int imageHeight = ilGetInteger(IL_IMAGE_HEIGHT);
	int width = 2;
	for (; width < imageWidth; width *= 2) {}

	int height = 2;
	for (; height < imageHeight; height *= 2) {}

	if (width > height)
	{
		width = height;
	}
	else
	{
		height = width;
	}
	unsigned char* data = new unsigned char[4 * width*height];
	::iluScale(width, height, 1);
	// 1107
	//flip the image
	int origin = ilGetInteger(IL_IMAGE_ORIGIN);
	// flip the image
	/*
	if (origin == IL_ORIGIN_LOWER_LEFT) {
		iluFlipImage();
	};
	*/
	iluFlipImage();
	::ilCopyPixels(0, 0, 0, width, height, 1, IL_RGBA, IL_UNSIGNED_BYTE, data);
	gluBuild2DMipmaps(GL_TEXTURE_2D, internalFormat, width,
		height, GL_RGBA, GL_UNSIGNED_BYTE,
		data);
	glBindTexture(GL_TEXTURE_2D, 0);
	return textureID;
}

char* loadFile(const char *fname, GLint &fSize)
{
	ifstream::pos_type size;
	char * memblock;
	string text;

	// file read based on example in cplusplus.com tutorial
	ifstream file(fname, ios::in | ios::binary | ios::ate);
	if (file.is_open())
	{
		size = file.tellg();
		fSize = (GLuint)size;
		memblock = new char[size];
		file.seekg(0, ios::beg);
		file.read(memblock, size);
		file.close();
		cout << "file " << fname << " loaded" << endl;
		text.assign(memblock);
	}
	else
	{
		cout << "Unable to open file " << fname << endl;
		exit(1);
	}
	return memblock;
}

void printShaderInfoLog(GLint shader)
{
	int infoLogLen = 0;
	int charsWritten = 0;
	GLchar *infoLog;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

	if (infoLogLen > 0)
	{
		infoLog = new GLchar[infoLogLen];
		// error check for fail to allocate memory omitted
		glGetShaderInfoLog(shader, infoLogLen, &charsWritten, infoLog);
		cout << "InfoLog:" << endl << infoLog << endl;
		delete[] infoLog;
	}
}

void printProgramInfoLog(GLint program)
{
	int infoLogLen = 0;
	int charsWritten = 0;
	GLchar *infoLog;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);

	if (infoLogLen > 0)
	{
		infoLog = new GLchar[infoLogLen];
		// error check for fail to allocate memory omitted
		glGetProgramInfoLog(program, infoLogLen, &charsWritten, infoLog);
		cout << "InfoLog:" << endl << infoLog << endl;
		delete[] infoLog;
	}
}

unsigned int createShader(const char* vertexShader, const char* fragmentShader, const char* geometryShader = 0)
{
	GLuint f, g, v;

	char *vs, *gs, *fs;
	GLint compiled;

	unsigned int programID = glCreateProgram();
	// load shaders & get length of each
	GLint vlen;
	v = glCreateShader(GL_VERTEX_SHADER);
	vs = loadFile(vertexShader, vlen);
	const char * vv = vs;
	glShaderSource(v, 1, &vv, &vlen);
	glCompileShader(v);
	glGetShaderiv(v, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		cout << "Vertex shader (" << vertexShader << ") did not compiled." << endl;
		printShaderInfoLog(v);
		return 0;
	}
	glAttachShader(programID, v);

	if (geometryShader != 0)
	{
		GLint glen;
		g = glCreateShader(GL_GEOMETRY_SHADER);
		gs = loadFile(geometryShader, glen);
		const char * gg = gs;
		glShaderSource(g, 1, &gg, &glen);
		glCompileShader(g);
		glGetShaderiv(g, GL_COMPILE_STATUS, &compiled);
		if (!compiled)
		{
			cout << "Geometry shader  (" << geometryShader << ") did not compiled." << endl;
			printShaderInfoLog(g);
			return 0;
		}
		glAttachShader(programID, g);
	}

	GLint flen;
	f = glCreateShader(GL_FRAGMENT_SHADER);
	fs = loadFile(fragmentShader, flen);
	const char * ff = fs;
	glShaderSource(f, 1, &ff, &flen);
	glCompileShader(f);
	glGetShaderiv(f, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		cout << "Fragment shader  (" << fragmentShader << ") did not compiled." << endl;
		printShaderInfoLog(f);
	}
	glAttachShader(programID, f);


	glBindAttribLocation(programID, 0, "in_Position");
	glBindFragDataLocation(programID, 0, "out_Color");

	GLint linked;
	glLinkProgram(programID);
	glGetProgramiv(programID, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		cout << "Shader Program did not link." << endl;
		printShaderInfoLog(f);
	}

	delete[] vs; // dont forget to free allocated memory
	delete[] fs; // we allocated this in the loadFile function...
	if (geometryShader != 0)
		//delete[] gs; // we allocated this in the loadFile function...

	return programID;
}

void initOpenGlWindow(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB);
	glutInitWindowSize(600, 900);
	glutCreateWindow("Simulation_of_Music");
	glewInit();
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		cout << "glewInit failed, aborting." << endl;
		exit(1);
	}
	cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << endl;
	cout << "OpenGL version " << glGetString(GL_VERSION) << " supported" << endl;
}

// function: CreateLineData
void CreateLineData()
{
	// line data
	float x_left = -0.8;
	float x_right = 0.8;
	float y = 0.7;
	int i = 0;
	float delta =0.03;
	while (i < 6) {
		// horizontal line
		float start = y;
		for (int j = 0; j < 5; j++)
		{
			line_vertices.push_back(x_left);
			line_vertices.push_back(start);
			line_vertices.push_back(x_right);
			line_vertices.push_back(start);
			start = start - delta;

		}
		start = start + delta;
		// vertical line
		line_vertices.push_back(x_left);
		line_vertices.push_back(y);
		line_vertices.push_back(x_left);
		line_vertices.push_back(start);
		line_vertices.push_back(x_right);
		line_vertices.push_back(y);
		line_vertices.push_back(x_right);
		line_vertices.push_back(start);
		i++;
		y = start - delta*5;
	}
}

void initModels(void)
{

	glGenVertexArrays(1, &vaoBrushPath);
	glBindVertexArray(vaoBrushPath);
	glGenBuffers(1, &vboBrushPath);
	glBindBuffer(GL_ARRAY_BUFFER, vboBrushPath);
	glBufferData(GL_ARRAY_BUFFER, Max_Points * sizeof(GLfloat), 0, GL_DYNAMIC_DRAW);
	glVertexAttribPointer((GLuint)0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	
	// CreateLineData
	CreateLineData();

	// symbol data
	float symbol_x = -0.75;
	float symbol_y = 0.64;
	for (int i = 0; i < 6; i++) {
		glm::vec2 tmp(symbol_x,symbol_y);
		symbol_data.push_back(tmp);
		symbol_y = symbol_y - 0.27;
		// for output data
		output_format tmp_format;
		tmp_format.beat = false;
		tmp_format.beat_number = 0;
		tmp_format.frequency = 0;
		tmp_format.position = tmp;
		tmp_format.texture_label = 14;
		output_data.push_back(tmp_format);
	}
	glGenVertexArrays(1, &vao_line);
	glBindVertexArray(vao_line);
	glGenBuffers(1, &vbo_line);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_line);
	glBufferData(GL_ARRAY_BUFFER, 1000* sizeof(GLfloat), 0, GL_DYNAMIC_DRAW);
	glVertexAttribPointer((GLuint)0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// symbol data
	glGenVertexArrays(1, &vao_symbol);
	glBindVertexArray(vao_symbol);
	glGenBuffers(1, &vbo_symbol);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_symbol);
	glBufferData(GL_ARRAY_BUFFER, 1000 * sizeof(glm::vec2), 0, GL_DYNAMIC_DRAW);
	glVertexAttribPointer((GLuint)0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);



	// symbol_data_2
	glGenVertexArrays(1, &vao_symbol2);
	glBindVertexArray(vao_symbol2);
	glGenBuffers(1, &vbo_symbol2);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_symbol2);
	glBufferData(GL_ARRAY_BUFFER, 1000 * sizeof(glm::vec2), 0, GL_DYNAMIC_DRAW);
	glVertexAttribPointer((GLuint)0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);


	// Reset 
	glBindVertexArray(0);
}

void initShaders()
{
	gaussianProgramID = createShader("../Media/shader/PassThruFarPlane.vert", "../Media/shader/Gaussian.frag", "../Media/shader/PassThru.geom");
	brushProgramID = createShader("../Media/shader/PassThruFarPlane.vert", "../Media/shader/Brush.frag", "../Media/shader/Brush.geom");
	line_program = createShader("../Media/shader/line.vert", "../Media/shader/line.frag");
	draw_program = createShader("../Media/shader/symbol.vert", "../Media/shader/symbol.frag", "../Media/shader/symbol.geom");
	draw_program2 = createShader("../Media/shader/symbol2.vert", "../Media/shader/symbol2.frag", "../Media/shader/symbol2.geom");
	
	unsigned int textureLoc = glGetUniformLocation(brushProgramID, "brushTexture");
	glUniform1i(textureLoc, 0);
	unsigned int texture1_Loc = glGetUniformLocation(draw_program, "brushTexture");
	glUniform1i(texture1_Loc, 0);
	unsigned int texture2_Loc = glGetUniformLocation(draw_program2, "brushTexture");
	glUniform1i(texture2_Loc, 0);

}

void initTexture()
{
	
	unsigned int texture;
	// load texture without beat
	texture = texture_load("../Media/texture/not_has_beat/digit_zero.png", GL_RGB);
	textures_nb.push_back(texture);
	texture = texture_load("../Media/texture/not_has_beat/digit_one.png", GL_RGB);
	textures_nb.push_back(texture);
	texture = texture_load("../Media/texture/not_has_beat/digit_two.png", GL_RGB);
	textures_nb.push_back(texture);
	texture = texture_load("../Media/texture/not_has_beat/digit_three.png", GL_RGB);
	textures_nb.push_back(texture);
	texture = texture_load("../Media/texture/not_has_beat/digit_four.png", GL_RGB);
	textures_nb.push_back(texture);
	texture = texture_load("../Media/texture/not_has_beat/digit_five.png", GL_RGB);
	textures_nb.push_back(texture);
	texture = texture_load("../Media/texture/not_has_beat/digit_six.png", GL_RGB);
	textures_nb.push_back(texture);
	texture = texture_load("../Media/texture/not_has_beat/digit_seven.png", GL_RGB);
	textures_nb.push_back(texture);
	texture = texture_load("../Media/texture/not_has_beat/digit_eight.png", GL_RGB);
	textures_nb.push_back(texture);
	texture = texture_load("../Media/texture/not_has_beat/digit_nine.png", GL_RGB);
	textures_nb.push_back(texture);
	texture = texture_load("../Media/texture/not_has_beat/music_flat_sign.png", GL_RGB);
	textures_nb.push_back(texture);
	texture = texture_load("../Media/texture/not_has_beat/music_natural_sign.png", GL_RGB);
	textures_nb.push_back(texture);
	texture = texture_load("../Media/texture/not_has_beat/music_sharp_sign.png", GL_RGB);
	textures_nb.push_back(texture);
	texture = texture_load("../Media/texture/not_has_beat/musical_symbol_final_barline.png", GL_RGB);
	textures_nb.push_back(texture);
	texture = texture_load("../Media/texture/not_has_beat/musical_symbol_g_clef.png", GL_RGB);
	textures_nb.push_back(texture);
	
	// load symbol with beat texture
	texture = texture_load("../Media/texture/has_beat/musical_symbol_half_note.png", GL_RGB);
	textures_wb.push_back(texture);
	texture = texture_load("../Media/texture/has_beat/musical_symbol_half_rest.png", GL_RGB);
	textures_wb.push_back(texture);
	texture = texture_load("../Media/texture/has_beat/musical_symbol_quarter_note.png", GL_RGB);
	textures_wb.push_back(texture);
	texture = texture_load("../Media/texture/has_beat/musical_symbol_quarter_rest.png", GL_RGB);
	textures_wb.push_back(texture);
	texture = texture_load("../Media/texture/has_beat/musical_symbol_eighth_note.png", GL_RGB);
	textures_wb.push_back(texture);
	texture = texture_load("../Media/texture/has_beat/musical_symbol_eighth_rest.png", GL_RGB);
	textures_wb.push_back(texture);
	texture = texture_load("../Media/texture/has_beat/musical_symbol_sixteenth_note.png", GL_RGB);
	textures_wb.push_back(texture);
	texture = texture_load("../Media/texture/has_beat/musical_symbol_sixteenth_rest.png", GL_RGB);
	textures_wb.push_back(texture);
	texture = texture_load("../Media/texture/has_beat/musical_symbol_thirty-second_note.png", GL_RGB);
	textures_wb.push_back(texture);
	texture = texture_load("../Media/texture/has_beat/musical_symbol_thirty-second_rest.png", GL_RGB);
	textures_wb.push_back(texture);
	texture = texture_load("../Media/texture/has_beat/musical_symbol_combining_augmentation_dot.png", GL_RGB);
	textures_wb.push_back(texture);
	//// for fun
	//texture = texture_load("../Media/WeChat Image_20180518103123.jpg", GL_RGB);
	//textures_wb.push_back(texture);

}

void initBrush()
{
	// Create the brush texture
	const int size = 256;
	glGenTextures(1, &brushTexture);
	glBindTexture(GL_TEXTURE_2D, brushTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_FLOAT, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Create a FBO
	unsigned int fboName;
	glGenFramebuffers(1, &fboName);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboName);
	glViewport(0, 0, size, size);

	// Attach the Texture to the Framebuffer.
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brushTexture, 0);
	// Check for FBO completeness.
	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER))
		throw "Problem creating the FBO";
	// Render a quad into the FBO.
	glUseProgram(gaussianProgramID);

	// Allocate Vertex Buffer Object (get a handle or ID)
	// No VAO, since we are only doing this once.
	unsigned int vertexBufferID;
	glGenBuffers(1, &vertexBufferID);

	// VBO for vertex data
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
	// Set the model data into the VBO.
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), vertices, GL_DYNAMIC_DRAW);
	// Define the layout of the vertex data.
	glVertexAttribPointer((GLuint)0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// Draw! But draw into the FBO.
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Clean up
	// Unbind and delete the buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vertexBufferID);

	//Unbind the Program (could have created it and deleted it here as well).
	glUseProgram(0);
	// Un-bind the FBO and delete it. Note: the texture still remains.
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fboName);

	// Generate the mip-maps for the texture.
	glBindTexture(GL_TEXTURE_2D, brushTexture);
	glGenerateMipmap(GL_TEXTURE_2D);
	// Alternatively, could tell the system to not use mipmaps with the call below.
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Reset Graphics state.
	glBindTexture(GL_TEXTURE_2D, 0);
	glViewport(0, 0, width, height);
}

void initBlending()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void initScene(void)
{
	//glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	initShaders();
	initTexture();
	initBrush();
	initModels();
	initBlending();
}

void takeScreenshot(const char* screenshotFile)
{
	ILuint imageID = ilGenImage();
	ilBindImage(imageID);
	ilutGLScreen();
	ilEnable(IL_FILE_OVERWRITE);
	ilSaveImage(screenshotFile);
	ilDeleteImage(imageID);
	printf("Screenshot saved to: %s\n", screenshotFile);
}


void draw_symbol(int texture_label, glm::vec2 tmp)
{
	glUseProgram(draw_program2);
	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_2D, textures_wb[texture_label]);
	glBindVertexArray(vao_symbol2);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_symbol2);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2), &tmp);
	glDrawArrays(GL_POINTS, 0, 1);
}

void display(void)
{
	/*
	if (brushPath.size() <= 0)
		return;
	glUseProgram(draw_program);

	//glUseProgram(line_program);
	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_2D, textures_wb[11]);
	glBindVertexArray(vaoBrushPath);
	glBindBuffer(GL_ARRAY_BUFFER, vboBrushPath);
	glBufferSubData(GL_ARRAY_BUFFER, 0, brushPath.size() * sizeof(GLfloat), &brushPath[0]);
	glDrawArrays(GL_POINTS, 0, brushPath.size() / 2);
	brushPath.clear();
	*/
	//glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	// create and draw symbol2
	float symbol2_y = 0.64;
	for (int i = 0; i < 6; i++) {
		float sum_beats = 4.0;
		float symbol2_x = -0.6;
		while (sum_beats != 0.0)
		{
			int random_number = rand() % 10; // do not consider . first

			if (random_number == 0 || random_number == 1)
			{
				if (sum_beats >= 2.0) {
					sum_beats = sum_beats - 2.0;
					glm::vec2 tmp(symbol2_x, symbol2_y);
					symbol_data2.push_back(tmp);
					symbol2_x = symbol2_x + 0.1;
					draw_symbol(random_number, tmp);
				}
			}
			if (random_number == 2 || random_number == 3)
			{
				if (sum_beats >= 1.0) {
					sum_beats = sum_beats - 1.0;
					glm::vec2 tmp(symbol2_x, symbol2_y);
					symbol_data2.push_back(tmp);
					symbol2_x = symbol2_x + 0.1;
					draw_symbol(random_number, tmp);
				}
			}
			if (random_number == 4 || random_number == 5)
			{
				if (sum_beats >= 0.5) {
					sum_beats = sum_beats - 0.5;
					glm::vec2 tmp(symbol2_x, symbol2_y);
					symbol_data2.push_back(tmp);
					symbol2_x = symbol2_x + 0.1;
					draw_symbol(random_number, tmp);
				}
			}
			if (random_number == 6 || random_number == 7)
			{
				if (sum_beats >= 0.25) {
					sum_beats = sum_beats - 0.25;
					glm::vec2 tmp(symbol2_x, symbol2_y);
					symbol_data2.push_back(tmp);
					symbol2_x = symbol2_x + 0.1;
					draw_symbol(random_number, tmp);
				}
			}
			if (random_number == 8 || random_number == 9)
			{
				if (sum_beats >= 0.125) {
					sum_beats = sum_beats - 0.125;
					glm::vec2 tmp(symbol2_x, symbol2_y);
					symbol_data2.push_back(tmp);
					symbol2_x = symbol2_x + 0.1;
					draw_symbol(random_number, tmp);
				}
			}
		}
		symbol2_y = symbol2_y - 0.27;
	}


	// draw symbol
	glUseProgram(draw_program);
	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_2D, textures_nb[14]);
	glBindVertexArray(vao_symbol);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_symbol);
	glBufferSubData(GL_ARRAY_BUFFER, 0, symbol_data.size() * sizeof(glm::vec2), &symbol_data[0]);
	glDrawArrays(GL_POINTS, 0, symbol_data.size());
	

	// draw line
	glColor3f(0.1, 0.1, 0.1);
	glUseProgram(line_program);
	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_2D, brushTexture);
	glBindVertexArray(vao_line);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_line);
	glBufferSubData(GL_ARRAY_BUFFER, 0, line_vertices.size() * sizeof(GLfloat), &line_vertices[0]);
	for (int i = 0; i < line_vertices.size(); i += 2) {
		glDrawArrays(GL_LINES, i, 2);
	}
	// reset things back to the default.
	glBindVertexArray(0);
	glUseProgram(0);

	glFinish();
	
	
	string output = "../Output/picture/screenshot";
	string medium = std::to_string(number_screenshot);
	string last = ".png";
	output = output + medium + last;
	takeScreenshot(output.c_str());
	number_screenshot++;
	
	//takeScreenshot("../Output/picture/screenshot.png");

}

void reshape(int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	// For debugging and a cleaner look
	glClear(GL_COLOR_BUFFER_BIT);
	glFinish();
	glutPostRedisplay();
}
bool painting = false;
void mouseEvent(int button, int state, int ix, int iy)
{
	// Could require left mouse down only, but ...
	if (state == GLUT_DOWN)
	{
		painting = true;
	}
	else
	{
		painting = false;
	}
}

void mouseMove(int ix, int iy)
{
	if (painting)
	{
		if (brushPath.size() > Max_Points)
			brushPath.clear();
		float x = 2.0f* ix / (float)(width - 1) - 1.0f;
		float y = 2.0f* (height - 1 - iy) / (float)(height - 1) - 1.0f;
		brushPath.push_back(x);
		brushPath.push_back(y);
		glutPostRedisplay();
	}
}

int main(int argc, char* argv[])
{

	initOpenGlWindow(argc, argv);
	initScene();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouseEvent);
	glutMotionFunc(mouseMove);
	glutMainLoop();
	return 0;
}
