#include <opencv2/opencv.hpp>

#include<iostream>
#include <fstream>
#include <string.h>
#include<string>
#include<sstream>
#include<vector>
#include<time.h>

#include <gl/glut.h>
#include <stdio.h>
#include <math.h>
#include <windows.h>
#include<gl/freeglut.h>

#define endl '\n'


using namespace std;
using namespace cv;

void draw(void);
void init(void);
void idle(void);
void resize(int, int);

void dodisplay_frame(int);
void read_csv_2(void);

void special_keyboard(int, int, int);
void mouse(int, int, int, int);
void mouseWheel(int, int, int, int);
void keyboard(unsigned char, int, int);
void draw_axis(void);

#define pi 3.1415926535897
double radian = pi / 180;
double radius = 2;

GLfloat rotate = 0.0f;
GLfloat xAngle, yAngle, zAngle;

double theta = 90 * radian, phi = 90 * radian;
double r = 1000.53;
double X = r * sin(theta) * cos(phi);
double Y = r * sin(theta) * sin(phi);
double Z = r * cos(theta);
bool axis_flag = false;
double frame_time;

int row = 512; //x 축으로의 점갯수
int column = 512; //y축으로의 점 갯수
int seconds = 5; //lenght of file
int  frame = 60*seconds; //총 프레임 수
int frame_num = 0; //몇번쨰 프레임 인지

double frame_n = 0; //몇번쨰 프레임 인지

vector<int> v0; //데이터 저장되는 벡터
vector<int> v1;
vector<int> v2;

vector<int> verify; //검증용 벡터

vector<vector<int>> v(frame); //총 프레인 수로 벡터를 저장함(f[0]~f[frame-1]까지)

int mode = 0;
double i = 0;

Mat src = imread("LGD.png"); // 원본사진 이미지를 불러온다.


int main(int argc, char** argv) {
    ios_base::sync_with_stdio(0);
    cin.tie(NULL);
    cout.tie(NULL);

    read_csv_2();

    /*window 초기화*/
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(5120 /3 , 2880/3);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("3D-Simulation");
    init(); //사용자 초기화 함수
    glutReshapeFunc(resize);
    /* Call back*/
    glutMouseFunc(mouse);
    glutMouseWheelFunc(mouseWheel);
    /* 키보드 콜백 함수 */
    glutSpecialFunc(special_keyboard);
    glutKeyboardFunc(keyboard);

    /* 마우스 콜백 함수 */
    glutIdleFunc(idle);
    glutDisplayFunc(draw); /* draw() : 실제 그리기 함수*/
   
    /* Looping 시작 */
    glutMainLoop();

    //read_csv(); //검증용

    return 0;

}


void read_csv_2(void) { //데이터 파일에서 데이터 읽어 오기 ! version 2
    ifstream readFile;
    //파일 열기
    //readFile.open("Cylinder_5s_v1.csv", ios::in); //파일 읽기
    readFile.open("Sphere_5s_v4.csv", ios::in); //파일 읽기
    string input;

    if (readFile.good()) //파일에 데이터 있으면 실행
    {
        int j = 0; //첫번째 행 스킵하기 위함
        int ccnt = 0;
        cout << "Loading..." << endl;
        
        while (getline(readFile, input)) // 한 행 읽기
        {
            double i;
            int sb;
            istringstream ss(input);
            string stringBuffer;
            int cnt = 0;
            while (getline(ss, stringBuffer, ',')) //한행에서 "," 단위로 데이터 읽기
            {
                if (j == 0)break; //첫번째 행 스킵하기 위함

                for (int k = 0; k < frame; k++)//프레임 0부터 ~frame-1번까지 각 frame 데이터 저장
                {
                    if (3 * k <= cnt && cnt < 3 * (k + 1)) {
                        /*실수 형으로 입력 받는 부분*/
                        std::stringstream ssdd(stringBuffer);
                        ssdd >> i;
                        if (i >= 127)i = i / 127;
                        v[k].push_back(i);

                        /*정수형으로 입력 받는 곳*/
                      /*std::stringstream ssint(stringBuffer);
                        ssint >> sb;
                        if (sb >= 127)sb = sb / 127;
                        v[k].push_back(sb);    */                    
                    }  
                }
                cnt++;
                
            }
            j++;

            float k = j / 512;
            int l = (int)k * 100 / 512;
            
            if (ccnt % 3071 == 0) {
                //cout << l << " %" << endl;
                cout<<"#";
            }
            if (l == 100)cout << "Complete...!" << endl;
            ccnt++;
        }
        readFile.close();    //파일 닫아줍니다.
    }
}


void init(void)
{
    printf("init func called\n");
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //set antialiasing   
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
    glEnable(GL_LINE_SMOOTH);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

bool up = true;
void idle(void)
{
    
    cout << frame_n << endl;

    if (up)frame_n += 1;
    else frame_n -= 1;
    //frame_n = 56;
    if (frame_n >= frame-3)up = !up;
    if (frame_n == 0)up = !up;
    glutPostRedisplay();
}


void dodisplay_frame(int frame) {
    for (int i = 0; i < v[frame].size() - 6 * row; i++) { //맨 윗줄은 실행안함
        if (i % 6 == 0 && !(i % (3 * column) >= (column - 2) * 3) &&!((i / (3 * row)) % 2 == 1)) { //x 좌표만을 이용해서 구현 2칸씩 띄워서 구현 ,마지막 2개 열의 점에서는 계산 x
           
            float blue, green, red;
            blue = (float)src.at<Vec3b>(src.rows  -1- (int)(i /(6 * column)), (i%(3*column)) /6)[0]; // 발광부가 어떠한 색을 가지는지 원본사진의 위치에서 따와서 처리를 해준다.
            green = (float)src.at<Vec3b>(src.rows  -1- (int)(i / (6 * column)), (i % (3 * column)) / 6)[1];// openCV를 이용한 픽셀 색 처리를 안할 경우 주석처리를 하면 된다.
            red = (float)src.at<Vec3b>(src.rows  -1 - (int)(i /(6 * column)), (i % (3 * column)) / 6)[2];
            
            //float sub = v[frame][i + 3 * 2 * column + 5]- v[frame][i + 3 * column + 2];
            //if (sub < 0) sub = sub * -1;
            //blue -= sub *10;
            //green -= sub *10 ;
            //red -= sub *10 ;
            if (blue <= 0)blue = 0;
            if (green <= 0)green = 0;
            if (red <= 0)red = 0;
            glColor3ub(red, green, blue);

            //glColor3ub(200, 200, 200);
            
             glBegin(GL_QUADS);
             glVertex3f(v[frame][i + 3 * column], v[frame][i + 3 * column + 1], v[frame][i + 3 * column + 2]); //0,1,0
             glVertex3f(v[frame][i + 3 * column + 3], v[frame][i + 3 * column + 4], v[frame][i + 3 * column + 5]); //1,1,0
             glVertex3f(v[frame][i + 3 * 2 * column + 3], v[frame][i + 3 * 2 * column + 4], v[frame][i + 3 * 2 * column + 5]); //1,2,0
             glVertex3f(v[frame][i + 3 * 2 * column], v[frame][i + 3 * 2 * column + 1], v[frame][i + 3 * 2 * column + 2]); //0,2,0
             glEnd();

             //glColor3ub(0, 0, 0);
             glBegin(GL_QUADS);
             glVertex3f(v[frame][i + 3 * column + 3], v[frame][i + 3 * column + 4], v[frame][i + 3 * column + 5]); 
             glVertex3f(v[frame][i + 3 * column + 6], v[frame][i + 3 * column + 7], v[frame][i + 3 * column + 8]); 
             glVertex3f(v[frame][i + 3 * 2 * column + 6], v[frame][i + 3 * 2 * column + 7], v[frame][i + 3 * 2 * column + 8]); 
             glVertex3f(v[frame][i + 3 * 2 * column + 3], v[frame][i + 3 * 2 * column + 4], v[frame][i + 3 * 2 * column + 5]); 
             glEnd();

            glBegin(GL_QUADS);
            glVertex3f(v[frame][i +3], v[frame][i + 4], v[frame][i + 5]);  //0,0,0
            glVertex3f(v[frame][i + 6], v[frame][i + 7], v[frame][i + 8]);   //0,1,0
            glVertex3f(v[frame][i + 3 * column + 6], v[frame][i + 3 * column + 7], v[frame][i + 3 * column + 8]); //1,1,0
            glVertex3f(v[frame][i + 3 * column + 3], v[frame][i + 3 * column + 4], v[frame][i + 3 * column + 5]); //0,1,0
            glEnd();


            glBegin(GL_QUADS);
            glVertex3f(v[frame][i], v[frame][i + 1], v[frame][i + 2]);  //0,0,0
            glVertex3f(v[frame][i + 3], v[frame][i + 4], v[frame][i + 5]);   //0,1,0
            glVertex3f(v[frame][i + 3 * column + 3], v[frame][i + 3 * column + 4], v[frame][i + 3 * column + 5]); //1,1,0
            glVertex3f(v[frame][i + 3 * column], v[frame][i + 3 * column + 1], v[frame][i + 3 * column + 2]); //0,1,0
            glEnd();
        }
    }
}






void draw(void) //display call back 함수
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (theta <= pi) gluLookAt(X, Z, Y, 0, 0, 0, 0, 1, 0);    //방향키로 시점 변경하되, 180도 넘어가면 카메라 Y값을 변경해야 부드럽게 넘어감
    else if (theta > pi) gluLookAt(X, Z, Y, 0, 0, 0, 0, -1, 0);
    
    int x = -row / 2 - 200;
    int y = -row / 2 - 100;
 
    glRotatef(xAngle, 1.0f, 0.0f, 0.0f);
    glRotatef(yAngle, 0.0f, 1.0f, 0.0f);
    glRotatef(zAngle, 0.0f, 0.0f, 1.0f);

    glTranslatef(x , y, 0);

    dodisplay_frame(frame_n);
    //dodisplay_frame(10);
    if (axis_flag == true)   draw_axis();

    //glFlush(); // 지금까지의 glut 함수를 밀어 넣어줌(실행시킴)
    glutSwapBuffers();
}

void resize(int width, int height)
{
    printf("WM_SIZE change!\n");


    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, (float)width / (float)height, 0, 15000);
    glMatrixMode(GL_MODELVIEW);
    printf("높이: %d, 폭: %d \n", height, width);
}
void special_keyboard(int key, int x, int y)
{
    switch (key) {
    case GLUT_KEY_LEFT:
        phi -= 5 * radian;
        if (phi < 0) {
            phi += 2 * pi;
        }
        X = r * sin(theta) * cos(phi);
        Y = r * sin(theta) * sin(phi);
        Z = r * cos(theta);
        break;

    case GLUT_KEY_RIGHT:
        phi += 5 * radian;
        if (phi > 2 * pi) {
            phi -= 2 * pi;
        }
        X = r * sin(theta) * cos(phi);
        Y = r * sin(theta) * sin(phi);
        Z = r * cos(theta);
        break;
    case GLUT_KEY_UP:
        theta += 5 * radian;
        if (theta > 2 * pi) {
            theta -= 2 * pi;
        }
        X = r * sin(theta) * cos(phi);
        Y = r * sin(theta) * sin(phi);
        Z = r * cos(theta);
        break;
    case GLUT_KEY_DOWN:
        theta -= 5 * radian;
        if (theta < 0) {
            theta += 2 * pi;
        }
        X = r * sin(theta) * cos(phi);
        Y = r * sin(theta) * sin(phi);
        Z = r * cos(theta);

        break;
    case GLUT_KEY_PAGE_UP:
        r -= radius;
        X = r * sin(theta) * cos(phi);
        Y = r * sin(theta) * sin(phi);
        Z = r * cos(theta);

        break;
    case GLUT_KEY_PAGE_DOWN:
        r += radius;
        X = r * sin(theta) * cos(phi);
        Y = r * sin(theta) * sin(phi);
        Z = r * cos(theta);

        break;

    }
    glutPostRedisplay();
}
void mouseWheel(int button, int dir, int x, int y) {
    if (dir > 0) {  //줌인 마우스 휠
        if (r > 3) {
            r -= radius;
            X = r * sin(theta) * cos(phi);
            Y = r * sin(theta) * sin(phi);
            Z = r * cos(theta);
            printf("Zoom in!, radius : %f\n", r);
        }
    }
    else {      //줌아웃 마우스 휠
        if (r < 3430.53) {
            r += radius;
            X = r * sin(theta) * cos(phi);
            Y = r * sin(theta) * sin(phi);
            Z = r * cos(theta);
            printf("Zoom out!, radius : %f\n", r);
        }
    }
    glutPostRedisplay();
}


/*마우스 콜백 함수*/
void mouse(int button, int state, int x, int y)
{
    /*인자들을 해석해서 원하는 기능을 구현*/
    printf("Mouse button is clicked! (%d, %d, %d, %d)\n", button, state, x, y);
}

/*키보드 입력 콜백 함수*/
void keyboard(unsigned char key, int x, int y)
{
    printf("You pressed %c\n", key);
    switch (key) {
    case 'a':yAngle += 2; break;
    case 'd':yAngle -= 2; break;
    case 'w':xAngle += 2; break;
    case 's':xAngle -= 2; break;
    case 'q':zAngle += 2; break;
    case 'e':zAngle -= 2; break;
    case 'z':xAngle = yAngle = zAngle = 0.0; break;
    case 'i':axis_flag = !axis_flag; break;

    }
    glutPostRedisplay();
}
void draw_axis(void)
{
    glLineWidth(3); // 좌표축의 두께
    glBegin(GL_LINES);
    glColor3f(1, 0, 0); // X축은 red
    glVertex3f(0, 0, 0);
    glVertex3f(14, 0, 0);

    glColor3f(0, 1, 0); // Y축은 green
    glVertex3f(0, 0, 0);
    glVertex3f(0, 14, 0);

    glColor3f(0, 0, 1); // Z축은 blue
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, 14);
    glEnd();
    glLineWidth(1); // 두께 다시 환원
}
