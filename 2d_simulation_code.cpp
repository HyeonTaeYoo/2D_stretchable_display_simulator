#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <vector>
#include <QDebug>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>
#include <QtAlgorithms>
#include <QPair>

using namespace cv;
using namespace std;

int Block_size = 2;
int BlackWidth = Block_size;
int BrightWidth = Block_size;
int B = 2*Block_size; // B/2= Blocksize
double MAX_STRETCH_PERCENTAGE = 20;
int direction=0;
int Get_Back_flag=0;
int File_save_flag=0;
int howlong=2;
int fps=60;
int frames=120;
int GridColor=0;

double Primary[] = { 262.775,123.2375,98.4775,120.6349,344.1349,31.7649,0.1934,21.3641,514.1541 };
double InvPrimary[] = { 0.00452944, -0.00157421,-0.00077028,-0.00159373,0.00347093,0.00009081,0.00006452,-0.00014363,0.00194146 };
Matrix PC_Mx, IPC_Mx;
QPair<int, double> Rs[128] = {}; // EOTF_Value TXT파일에서 가져온 Rs 값을 저장할 배열
QPair<int, double> Gs[128] = {}; // EOTF_Value TXT파일에서 가져온 Gs 값을 저장할 배열
QPair<int, double> Bs[128] = {}; // EOTF_Value TXT파일에서 가져온 Bs 값을 저장할 배열

QString SaveFileName;
string avi=".avi";

struct cmp_XYZ
{
    bool operator()(const QPair<int,double> & a, const QPair<int,double> & b) const
    {
        return a.first < b.first;
    }
};

struct cmp_RGBs
{
    bool operator()(const QPair<int,double> & a, const QPair<int,double> & b) const
    {
        return (double)a.second < b.second;
    }
};


string video_name;


Mat src;
//Mat blockedimg(src.rows *B/2, src.cols *B/2, CV_8UC3, Vec3b(0, 0, 0)); //블록화된 이미지 저장 (타입 1,2 비교하기위함) 수정: 1/8 유현태 : 블록화 반만하고 블랙영역 삽입
Mat blockedimg(src.rows * B, src.cols * B, CV_8UC3, Vec3b(0, 0, 0)); //블록화된 이미지 저장
Mat imga;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::stretchableForming(int B, Mat src1, Mat& dst) {
    qDebug() << dst.rows;
    dst = src1;
    for (int y = 0; y < dst.rows; y++)
    {
        if ((y % B)>=BrightWidth) {
            for (int x = 0; x < dst.cols; x++) {
                dst.at<Vec3b>(y, x)[0] = GridColor;
                dst.at<Vec3b>(y, x)[1] = GridColor;
                dst.at<Vec3b>(y, x)[2] = GridColor;
            }
        }
        else {
            for (int x = 0; x < dst.cols; x++)
            {
                if ((x % B) >= BrightWidth) {
                    dst.at<Vec3b>(y, x)[0] = GridColor;
                    dst.at<Vec3b>(y, x)[1] = GridColor;
                    dst.at<Vec3b>(y, x)[2] = GridColor;
                }
                else
                {
                    dst.at<Vec3b>(y, x) = src1.at<Vec3b>(y, x);
                }
            }
        }
    }

    imwrite("StretchableFormed2.png", dst);
}

void MainWindow::makingBlock(int B, Mat src1, Mat& dst) {
   // Mat tmp(src1.rows * Block_size, src1.cols * Block_size, CV_8UC3, Vec3b(0, 0, 0));   // 변경 블록화 반만하기위함
    Mat tmp(src1.rows * B, src1.cols * B, CV_8UC3, Vec3b(0, 0, 0));
    for (int y = 0; y < tmp.rows; y++)
    {
        for (int x = 0; x < tmp.cols; x++)
        {
            tmp.at<Vec3b>(y, x) = src1.at<Vec3b>(y / B, x / B);
        }
    }
    dst = tmp;
    imwrite("Blocked.png",dst);
}


void MainWindow::stretch_cols_general(int B, double S, Mat src, Mat& dst) {
    double P = B * S; //연신 시 증가량
    double a = 0;
    double m = 0;
    int index = -1;
    int i_max = 0;
    double stretchper = 1 + S;

    Mat imgg(src.rows, src.cols * stretchper, CV_8UC3, Scalar(255, 255, 255));

    vector<Vec3b> v;

    for (int y = 0; y < src.rows; y++) {

        a = 0;
        m = 0;
        index = -1;
        i_max = 0;


        if (y % B == 0)
        {
            v.clear();
            for (int x = 0; x < src.cols; x++) {
                v.push_back(src.at<Vec3b>(y, x));
            }

            while (i_max < imgg.cols) {
                QPair<double, QPair<double, double> > adjacentXYZ1;
                QPair<double, QPair<double, double> > adjacentXYZ2;
                double percentage1, percentage2;
                double XYZ[3];
                if (0 <= (a - (int)a) * (a - (int)a) && (a - (int)a) * (a - (int)a) < 0.00000000001) {
                    //cout << "a정수" << endl;
                    m = 0;

                    index = index + B;
                    i_max = i_max + B;

                    if ((index >= imgg.cols - 1)||(v.size()<index)) {
                        if (imgg.cols > v.size()) {
                            int plus = imgg.cols - v.size();
                            Vec3b plusval = v[v.size() - 1];
                            v.insert(v.begin() + v.size() - 1, plus, plusval);
                        }

                        i_max = imgg.cols;
                        break;
                    }

                    if (((P - (int)P) * (P - (int)P) < 0.00000000001)) {
                        v.insert(v.begin() + index, (int)P, Vec3b(v.at(index)[0], v.at(index)[1], v.at(index)[2]));
                        //v.insert(v.begin() + index, (int)P, Vec3b(0, 0, 0));
                        index = index + (int)P;
                        i_max = i_max + (int)P;

                    }
                    else {
                        v.insert(v.begin() + index, ceil(P), Vec3b(v.at(index)[0], v.at(index)[1], v.at(index)[2]));
                        //v.insert(v.begin() + index, ceil(P), Vec3b(0, 0, 0));
                        index = index + ceil(P);
                        i_max = i_max + ceil(P);

                    }

                    if ((index >= imgg.cols - 1)||(v.size()<index)) {
                        if (imgg.cols > v.size()) {
                            int plus = imgg.cols - v.size();
                            Vec3b plusval = v[v.size() - 1];
                            v.insert(v.begin() + v.size() - 1, plus, plusval);
                        }
                        i_max = imgg.cols;
                        break;
                    }

                    if (P > 1) {

                        double percon = 1 - (P - (int)P);
                        adjacentXYZ1 = calXYZofAdj_Pixel(v[index + 1][2], v[index + 1][1], v[index + 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        adjacentXYZ2 = calXYZofAdj_Pixel(v[index - 1][2], v[index - 1][1], v[index - 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        //adjacentXYZ2 = calXYZofAdj_Pixel(0, 0, 0, Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        percentage1 = 1 - (P - (int)P);
                        percentage2 = 1 - percentage1;
                        if ((percon - 1) * (percon - 1) > 0.000000001 || (percon - 1) * (percon - 1) < -0.0000000001) {
                            Matrix RsGsBs_Mx = calRsGsBs2(adjacentXYZ1, adjacentXYZ2, XYZ, percentage1, percentage2);
                            QPair<int, QPair<int, int> > newRGB = calNewRGB(Rs, Gs, Bs, RsGsBs_Mx.GetValue(1, 1), RsGsBs_Mx.GetValue(2, 1), RsGsBs_Mx.GetValue(3, 1));

                            v[index][2] = newRGB.first;
                            v[index][1] = newRGB.second.first;
                            v[index][0] = newRGB.second.second;
                        }
                        m = P - (int)P;
                    }
                    else if (P < 1) {

                        adjacentXYZ1 = calXYZofAdj_Pixel(v[index + 1][2], v[index + 1][1], v[index + 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        adjacentXYZ2 = calXYZofAdj_Pixel(v[index - 1][2], v[index - 1][1], v[index - 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        //adjacentXYZ2 = calXYZofAdj_Pixel(0, 0, 0, Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        percentage1 = 1 - P;
                        percentage2 = 1 - percentage1;
                        Matrix RsGsBs_Mx = calRsGsBs2(adjacentXYZ1,adjacentXYZ2, XYZ, percentage1,percentage2);
                        QPair<int, QPair<int, int> > newRGB = calNewRGB(Rs, Gs, Bs, RsGsBs_Mx.GetValue(1, 1), RsGsBs_Mx.GetValue(2, 1), RsGsBs_Mx.GetValue(3, 1));

                        v[index][2] = newRGB.first;
                        v[index][1] = newRGB.second.first;
                        v[index][0] = newRGB.second.second;



                        m = P;
                    }
                    a = m;
                    /*v.erase(v.begin()+index-1);
                        index--;
                        i_max--;*/
                }
                else {
                    //cout << "a정수아님" << endl;
                    index = index + BrightWidth;
                    i_max = i_max + BrightWidth;

                    if ((index >= imgg.cols - 1)||(v.size()<index)) {
                        if (imgg.cols > v.size()) {
                            int plus = imgg.cols - v.size();
                            Vec3b plusval = v[v.size() - 1];
                            v.insert(v.begin() + v.size() - 1, plus, plusval);
                        }
                        i_max = imgg.cols;
                        break;
                    }


                    adjacentXYZ2 = calXYZofAdj_Pixel(v[index + 1][2], v[index + 1][1], v[index + 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                    //adjacentXYZ2 = calXYZofAdj_Pixel(0, 0, 0, Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                    adjacentXYZ1 = calXYZofAdj_Pixel(v[index][2], v[index][1], v[index][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                    percentage1 = m;
                    percentage2 = 1 - percentage1;
                    Matrix RsGsBs_Mx = calRsGsBs2(adjacentXYZ1,adjacentXYZ2, XYZ, percentage1,percentage2);
                    QPair<int, QPair<int, int> > newRGB = calNewRGB(Rs, Gs, Bs, RsGsBs_Mx.GetValue(1, 1), RsGsBs_Mx.GetValue(2, 1), RsGsBs_Mx.GetValue(3, 1));

                    v[index][2] = newRGB.first;
                    v[index][1] = newRGB.second.first;
                    v[index][0] = newRGB.second.second;

                    index = index + BlackWidth;
                    i_max = i_max + BlackWidth;

                    if ((index >= imgg.cols - 1)||(v.size()<index)) {
                        if (imgg.cols > v.size()) {
                            int plus = imgg.cols - v.size();
                            Vec3b plusval = v[v.size() - 1];
                            v.insert(v.begin() + v.size() - 1, plus, plusval);
                        }
                        i_max = imgg.cols;
                        break;
                    }

                    a = P - (1 - m);

                    if (((a - (int)a) * (a - (int)a)) < 0.0000000001) {
                        if (a > 1)
                            a = (int)a;
                        else if (a < 1)
                            a = 0;

                        v.insert(v.begin() + index, a, Vec3b(v.at(index)[0], v.at(index)[1], v.at(index)[2]));
                        //v.insert(v.begin() + index, a, Vec3b(0, 0, 0));
                        index = index + a;
                        i_max = i_max + a;
                    }

                    else {
                        v.insert(v.begin() + index, ceil(a), Vec3b(v.at(index)[0], v.at(index)[1], v.at(index)[2]));
                        //v.insert(v.begin() + index, ceil(a), Vec3b(0, 0, 0));
                        index = index + ceil(a);
                        i_max = i_max + ceil(a);
                    }

                    if ((index >= imgg.cols - 1)||(v.size()<index)) {
                        if (imgg.cols > v.size()) {
                            int plus = imgg.cols - v.size();
                            Vec3b plusval = v[v.size() - 1];
                            v.insert(v.begin() + v.size() - 1, plus, plusval);
                        }
                        i_max = imgg.cols;
                        break;
                    }

                    if ((a - (int)a) * (a - (int)a) < 0.000000001) continue;



                    else {
                        if (a > 1) {

                            adjacentXYZ1 = calXYZofAdj_Pixel(v[index + 1][2], v[index + 1][1], v[index + 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                            adjacentXYZ2 = calXYZofAdj_Pixel(v[index - 1][2], v[index - 1][1], v[index - 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                            //adjacentXYZ2 = calXYZofAdj_Pixel(0, 0, 0, Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                            percentage1 = 1 - (a - (int)a);
                            percentage2 = 1 - percentage1;
                            Matrix RsGsBs_Mx = calRsGsBs2(adjacentXYZ1,adjacentXYZ2, XYZ, percentage1, percentage2);
                            QPair<int, QPair<int, int> > newRGB = calNewRGB(Rs, Gs, Bs, RsGsBs_Mx.GetValue(1, 1), RsGsBs_Mx.GetValue(2, 1), RsGsBs_Mx.GetValue(3, 1));

                            v[index][2] = newRGB.first;
                            v[index][1] = newRGB.second.first;
                            v[index][0] = newRGB.second.second;



                            m = a - (int)a;
                        }

                        else if (0 < a && a < 1) {


                            adjacentXYZ1 = calXYZofAdj_Pixel(v[index + 1][2], v[index + 1][1], v[index + 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                            adjacentXYZ2 = calXYZofAdj_Pixel(v[index - 1][2], v[index - 1][1], v[index - 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                            //adjacentXYZ2 = calXYZofAdj_Pixel(0, 0, 0, Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                            percentage1 = 1 - a;
                            percentage2 = 1 - percentage1;
                            Matrix RsGsBs_Mx = calRsGsBs2(adjacentXYZ1, adjacentXYZ2, XYZ, percentage1, percentage2);
                            QPair<int, QPair<int, int> > newRGB = calNewRGB(Rs, Gs, Bs, RsGsBs_Mx.GetValue(1, 1), RsGsBs_Mx.GetValue(2, 1), RsGsBs_Mx.GetValue(3, 1));

                            v[index][2] = newRGB.first;
                            v[index][1] = newRGB.second.first;
                            v[index][0] = newRGB.second.second;


                            m = a;
                        }

                        else if (a < 0) {

                            adjacentXYZ1 = calXYZofAdj_Pixel(v[index + 1][2], v[index + 1][1], v[index + 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                            adjacentXYZ2 = calXYZofAdj_Pixel(v[index - 1][2], v[index - 1][1], v[index - 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                            //adjacentXYZ2 = calXYZofAdj_Pixel(0, 0, 0, Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                            percentage1 = -a;
                            percentage2 = 1 - percentage1;
                            Matrix RsGsBs_Mx = calRsGsBs2(adjacentXYZ1, adjacentXYZ2, XYZ, percentage1, percentage2);
                            QPair<int, QPair<int, int> > newRGB = calNewRGB(Rs, Gs, Bs, RsGsBs_Mx.GetValue(1, 1), RsGsBs_Mx.GetValue(2, 1), RsGsBs_Mx.GetValue(3, 1));

                            v[index][2] = newRGB.first;
                            v[index][1] = newRGB.second.first;
                            v[index][0] = newRGB.second.second;


                            m = a + 1;
                        }
                    }
                }
            }
        }

        if (y % B >= BlackWidth){
                v.clear();
                for(int x=0;x<imgg.cols;x++)
                    v.push_back(Vec3b(GridColor,GridColor,GridColor));
            }

        for (int x = 0; x < imgg.cols; x++) {
            imgg.at<Vec3b>(y, x) = v[x];
        }
    }
    dst = imgg;
}

void MainWindow::stretch_rows_general(int B, double S, Mat src, Mat& dst){
    double P = B * S; //연신 시 증가량
    double a = 0;
    double m = 0;
    int index = -1;
    int i_max = 0;
    double stretchper = 1 + S;


    Mat imgg(src.rows * stretchper, src.cols, CV_8UC3, Scalar(255, 255, 255));

    vector<Vec3b> v;

    for (int x = 0; x < src.cols; x++) {
        a = 0;
        m = 0;
        index = -1;
        i_max = 0;

        v.clear();
        for (int y = 0; y < src.rows; y++) {
            v.push_back(src.at<Vec3b>(y, x));
        }

        while (i_max < imgg.rows) {
            QPair<double, QPair<double, double> > adjacentXYZ1;
            QPair<double, QPair<double, double> > adjacentXYZ2;
            double percentage1, percentage2;
            double XYZ[3];
            if (0 <= (a - (int)a) * (a - (int)a) && (a - (int)a) * (a - (int)a) < 0.00000000001) {
                m = 0;

                index = index + B;
                i_max = i_max + B;

                if (index >= imgg.rows - 1) {
                    if (imgg.rows > v.size()) {
                        int plus = imgg.rows - v.size();
                        Vec3b plusval = v[v.size() - 1];
                        v.insert(v.begin() + v.size() - 1, plus, plusval);
                    }

                    i_max = imgg.rows;
                    break;
                }

                if (((P - (int)P) * (P - (int)P) < 0.00000000001)) {
                    v.insert(v.begin() + index, (int)P, Vec3b(v.at(index)[0], v.at(index)[1], v.at(index)[2]));
                    index = index + (int)P;
                    i_max = i_max + (int)P;
                }
                else {
                    v.insert(v.begin() + index, ceil(P), Vec3b(v.at(index)[0], v.at(index)[1], v.at(index)[2]));
                    index = index + ceil(P);
                    i_max = i_max + ceil(P);
                }

                if (index >= imgg.rows - 1) {
                    if (imgg.rows > v.size()) {
                        int plus = imgg.rows - v.size();
                        Vec3b plusval = v[v.size() - 1];
                        v.insert(v.begin() + v.size() - 1, plus, plusval);
                    }
                    i_max = imgg.rows;
                    break;
                }

                if (P > 1) {

                    double percon = 1 - (P - (int)P);
                    adjacentXYZ1 = calXYZofAdj_Pixel(v[index + 1][2], v[index + 1][1], v[index + 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                    adjacentXYZ2 = calXYZofAdj_Pixel(v[index - 1][2], v[index - 1][1], v[index - 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                    //adjacentXYZ2 = calXYZofAdj_Pixel(0, 0, 0, Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                    percentage1 = 1 - (P - (int)P);
                    percentage2 = 1 - percentage1;
                    if ((percon - 1) * (percon - 1) > 0.000000001 || (percon - 1) * (percon - 1) < -0.0000000001) {
                        Matrix RsGsBs_Mx = calRsGsBs2(adjacentXYZ1, adjacentXYZ2, XYZ, percentage1, percentage2);
                        QPair<int, QPair<int, int> > newRGB = calNewRGB(Rs, Gs, Bs, RsGsBs_Mx.GetValue(1, 1), RsGsBs_Mx.GetValue(2, 1), RsGsBs_Mx.GetValue(3, 1));

                        v[index][2] = newRGB.first;
                        v[index][1] = newRGB.second.first;
                        v[index][0] = newRGB.second.second;


                    }
                    m = P - (int)P;
                }
                else if (P < 1) {

                    adjacentXYZ1 = calXYZofAdj_Pixel(v[index + 1][2], v[index + 1][1], v[index + 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                    adjacentXYZ2 = calXYZofAdj_Pixel(v[index - 1][2], v[index - 1][1], v[index - 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                    //adjacentXYZ2 = calXYZofAdj_Pixel(0, 0, 0, Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                    percentage1 = 1 - P;
                    percentage2 = 1 - percentage1;
                    Matrix RsGsBs_Mx = calRsGsBs2(adjacentXYZ1,adjacentXYZ2, XYZ, percentage1,percentage2);
                    QPair<int, QPair<int, int> > newRGB = calNewRGB(Rs, Gs, Bs, RsGsBs_Mx.GetValue(1, 1), RsGsBs_Mx.GetValue(2, 1), RsGsBs_Mx.GetValue(3, 1));

                    v[index][2] = newRGB.first;
                    v[index][1] = newRGB.second.first;
                    v[index][0] = newRGB.second.second;



                    m = P;
                }
                a = m;
                /*v.erase(v.begin()+index-1);
                                            index--;
                                            i_max--;*/
            }
            else {
                //cout << "a정수아님" << endl;
                index = index + BrightWidth;
                i_max = i_max + BrightWidth;

                if ((index >= imgg.rows - 1)||(v.size()<index)) {
                    if (imgg.rows > v.size()) {
                        int plus = imgg.rows - v.size();
                        Vec3b plusval = v[v.size() - 1];
                        v.insert(v.begin() + v.size() - 1, plus, plusval);
                    }
                    i_max = imgg.rows;
                    break;
                }


                adjacentXYZ2 = calXYZofAdj_Pixel(v[index + 1][2], v[index + 1][1], v[index + 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                //adjacentXYZ2 = calXYZofAdj_Pixel(0, 0, 0, Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                adjacentXYZ1 = calXYZofAdj_Pixel(v[index][2], v[index][1], v[index][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                percentage1 = m;
                percentage2 = 1 - percentage1;
                Matrix RsGsBs_Mx = calRsGsBs2(adjacentXYZ1,adjacentXYZ2, XYZ, percentage1,percentage2);
                QPair<int, QPair<int, int> > newRGB = calNewRGB(Rs, Gs, Bs, RsGsBs_Mx.GetValue(1, 1), RsGsBs_Mx.GetValue(2, 1), RsGsBs_Mx.GetValue(3, 1));

                v[index][2] = newRGB.first;
                v[index][1] = newRGB.second.first;
                v[index][0] = newRGB.second.second;




                index = index + BlackWidth;
                i_max = i_max + BlackWidth;

                if ((index >= imgg.cols - 1)||(v.size()<index)) {
                    if (imgg.rows > v.size()) {
                        int plus = imgg.cols - v.size();
                        Vec3b plusval = v[v.size() - 1];
                        v.insert(v.begin() + v.size() - 1, plus, plusval);
                    }
                    i_max = imgg.rows;
                    break;
                }

                a = P - (1 - m);

                if (((a - (int)a) * (a - (int)a)) < 0.0000000001) {
                    if (a > 1)
                        a = (int)a;
                    else if (a < 1)
                        a = 0;

                    v.insert(v.begin() + index, a, Vec3b(v.at(index)[0], v.at(index)[1], v.at(index)[2]));
                    //v.insert(v.begin() + index, a, Vec3b(0, 0, 0));
                    index = index + a;
                    i_max = i_max + a;
                }

                else {
                    v.insert(v.begin() + index, ceil(a), Vec3b(v.at(index)[0], v.at(index)[1], v.at(index)[2]));
                    //v.insert(v.begin() + index, ceil(a), Vec3b(0, 0, 0));
                    index = index + ceil(a);
                    i_max = i_max + ceil(a);
                }

                if ((index >= imgg.rows - 1)||(v.size()<index)) {
                    if (imgg.rows > v.size()) {
                        int plus = imgg.rows - v.size();
                        Vec3b plusval = v[v.size() - 1];
                        v.insert(v.begin() + v.size() - 1, plus, plusval);
                    }
                    i_max = imgg.rows;
                    break;
                }

                if ((a - (int)a) * (a - (int)a) < 0.000000001) continue;



                else {
                    if (a > 1) {

                        adjacentXYZ1 = calXYZofAdj_Pixel(v[index + 1][2], v[index + 1][1], v[index + 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        adjacentXYZ2 = calXYZofAdj_Pixel(v[index - 1][2], v[index - 1][1], v[index - 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        //adjacentXYZ2 = calXYZofAdj_Pixel(0, 0, 0, Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        percentage1 = 1 - (a - (int)a);
                        percentage2 = 1 - percentage1;
                        Matrix RsGsBs_Mx = calRsGsBs2(adjacentXYZ1,adjacentXYZ2, XYZ, percentage1, percentage2);
                        QPair<int, QPair<int, int> > newRGB = calNewRGB(Rs, Gs, Bs, RsGsBs_Mx.GetValue(1, 1), RsGsBs_Mx.GetValue(2, 1), RsGsBs_Mx.GetValue(3, 1));

                        v[index][2] = newRGB.first;
                        v[index][1] = newRGB.second.first;
                        v[index][0] = newRGB.second.second;



                        m = a - (int)a;
                    }

                    else if (0 < a && a < 1) {


                        adjacentXYZ1 = calXYZofAdj_Pixel(v[index + 1][2], v[index + 1][1], v[index + 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        adjacentXYZ2 = calXYZofAdj_Pixel(v[index - 1][2], v[index - 1][1], v[index - 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        //adjacentXYZ2 = calXYZofAdj_Pixel(0, 0, 0, Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        percentage1 = 1 - a;
                        percentage2 = 1 - percentage1;
                        Matrix RsGsBs_Mx = calRsGsBs2(adjacentXYZ1, adjacentXYZ2, XYZ, percentage1, percentage2);
                        QPair<int, QPair<int, int> > newRGB = calNewRGB(Rs, Gs, Bs, RsGsBs_Mx.GetValue(1, 1), RsGsBs_Mx.GetValue(2, 1), RsGsBs_Mx.GetValue(3, 1));

                        v[index][2] = newRGB.first;
                        v[index][1] = newRGB.second.first;
                        v[index][0] = newRGB.second.second;


                        m = a;
                    }

                    else if (a < 0) {

                        adjacentXYZ1 = calXYZofAdj_Pixel(v[index + 1][2], v[index + 1][1], v[index + 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        adjacentXYZ2 = calXYZofAdj_Pixel(v[index - 1][2], v[index - 1][1], v[index - 1][0], Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        //adjacentXYZ2 = calXYZofAdj_Pixel(0, 0, 0, Rs, Gs, Bs); // 인접 픽셀의 XYZ 값 계산
                        percentage1 = -a;
                        percentage2 = 1 - percentage1;
                        Matrix RsGsBs_Mx = calRsGsBs2(adjacentXYZ1, adjacentXYZ2, XYZ, percentage1, percentage2);
                        QPair<int, QPair<int, int> > newRGB = calNewRGB(Rs, Gs, Bs, RsGsBs_Mx.GetValue(1, 1), RsGsBs_Mx.GetValue(2, 1), RsGsBs_Mx.GetValue(3, 1));

                        v[index][2] = newRGB.first;
                        v[index][1] = newRGB.second.first;
                        v[index][0] = newRGB.second.second;


                        m = a + 1;
                    }
                }
            }
        }

        /*if (x % B >= BlackWidth)
            {
                v.clear();
                for (int y = 0; y < imgg.rows; y++)
                {
                    v.push_back(Vec3b(0, 0, 0));
                }
            }*/

        for (int y = 0; y < imgg.rows; y++) {
            imgg.at<Vec3b>(y, x) = v[y];
        }
    }
    dst = imgg;
}
void MainWindow::stretch_diag_general(int B, double S, Mat src, Mat& dst){
    stretch_cols_general(B, S, src, dst);
    stretch_rows_general(B, S, dst, dst);
}

void MainWindow::on_pushButton_2_clicked()
{
    QString file = QFileDialog::getOpenFileName(this,"Choose File","C:\\","Files(*.*)");
    string str = file.toStdString();
    str.c_str();
    src = imread(str);
    if(!src.data)
    {
        cout<<"Image Loading error"<<endl;
        return;
    }
    ui->Directory->setText(file);
}

void MainWindow::makeVideo_cols(int B, int frames, int fps, Mat& img)
{
    Size size = Size(img.cols * (1 + (double)MAX_STRETCH_PERCENTAGE / 100) + 200, img.rows);
    VideoWriter writer;
    video_name = "../../C:\result_colsKsy.avi";
    if(File_save_flag==0){
        SaveFileName = QFileDialog::getSaveFileName(this,"Save File","",".avi");
        video_name = SaveFileName.toStdString();
        video_name += avi;
        video_name.c_str();
    }
    writer.open(video_name, VideoWriter::fourcc('M','J','P','G'), fps, size, true);
    Mat* frame;
    frame = new Mat[frames];

    string Frame_name;

    for (int frameNum = 0; frameNum < frames; frameNum++)
    {

        if (frameNum == 0) {
            copyMakeBorder(img, frame[frameNum], 0, 0, 0, (img.cols * (1 + (double)MAX_STRETCH_PERCENTAGE / 100) + 200 - img.cols), BORDER_CONSTANT, 0); // 영상의 가로를 800픽셀로 해주었으니, 연신된 이미지가 채우지 못한 부분은 검은색으로 표현
            //cvtColor(frame[frameNum],frame[frameNum],COLOR_RGB2BGR);
            writer.write(frame[frameNum]);
            Frame_name = "Frame."+to_string(frameNum)+".png";
            imwrite(Frame_name,img);
        }
        else
        {
            Mat temp(img.rows, img.cols * (1 + (double)MAX_STRETCH_PERCENTAGE / (frames - 1) * frameNum * 0.01), CV_8UC3, Vec3b(0, 0, 0));
            //int c = img.cols * (1 + (double)MAX_STRETCH_PERCENTAGE / (frames - 1) * frameNum * 0.01);
            //Size s = Size(c, img.rows);
            //cv::resize(img,temp,s,0,0,INTER_LINEAR);
            stretch_cols_general(B, (double)MAX_STRETCH_PERCENTAGE / (frames - 1) * frameNum * 0.01, img, temp);
            //GaussianBlur(temp,temp,Size(5,5),1.5);
            cout << (double)MAX_STRETCH_PERCENTAGE / (frames - 1) * frameNum << "%" << endl;
            copyMakeBorder(temp, frame[frameNum], 0, 0, 0, (img.cols * (1 + (double)MAX_STRETCH_PERCENTAGE / 100) + 200 - temp.cols), BORDER_CONSTANT, 0); // 영상의 가로를 800픽셀로 해주었으니, 연신된 이미지가 채우지 못한 부분은 검은색으로 표현
            //cvtColor(frame[frameNum],frame[frameNum],COLOR_RGB2BGR);
            writer.write(frame[frameNum]);
            Frame_name = "Frame."+to_string(frameNum)+".png";
            imwrite(Frame_name,temp);
        }
    }
    if(Get_Back_flag==0){
        for (int frameNum = frames - 1; frameNum >= 0; frameNum--)
            writer.write(frame[frameNum]);
    }
    cout << "done..." << endl;
    delete[] frame;
}



void MainWindow::makeVideo_rows(int B, int frames, int fps, Mat& img)
{
    Size size = Size(img.cols, img.rows * (1 + (double)MAX_STRETCH_PERCENTAGE / 100) + 200);
    VideoWriter writer;
    video_name = "../../C:\result_rows.avi";
    if(File_save_flag==0){
        SaveFileName = QFileDialog::getSaveFileName(this,"Save File","",".avi");
        video_name = SaveFileName.toStdString();
        video_name += avi;
        video_name.c_str();
    }
    writer.open(video_name, VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, size, true);
    Mat* frame;
    frame = new Mat[frames];
    for (int frameNum = 0; frameNum < frames; frameNum++)
    {
        if (frameNum == 0) {
            copyMakeBorder(img, frame[frameNum], 0, img.rows * (1 + (double)MAX_STRETCH_PERCENTAGE / 100) + 200 - img.rows, 0, 0, BORDER_CONSTANT, 0);
            writer.write(frame[frameNum]);
        }
        else
        {
            Mat temp(img.rows * (1 + (double)MAX_STRETCH_PERCENTAGE / (frames - 1) * frameNum * 0.01), img.cols, CV_8UC3, Vec3b(0, 0, 0));
            stretch_rows_general(B, (double)MAX_STRETCH_PERCENTAGE / (frames - 1) * frameNum * 0.01, img, temp);
            cout << (double)MAX_STRETCH_PERCENTAGE / (frames - 1) * frameNum << "%" << endl;
            copyMakeBorder(temp, frame[frameNum], 0, img.rows * (1 + (double)MAX_STRETCH_PERCENTAGE / 100) + 200 - temp.rows, 0, 0, BORDER_CONSTANT, 0);
            writer.write(frame[frameNum]);
        }
    }
    if(Get_Back_flag==0){
        for (int frameNum = frames - 1; frameNum >= 0; frameNum--)
            writer.write(frame[frameNum]);
    }
    cout << "done..." << endl;

    delete[] frame;
}

void MainWindow::makeVideo_diag(int B, int frames, int fps, Mat& img)
{
    Size size = Size(img.cols * (1 + (double)MAX_STRETCH_PERCENTAGE / 100) + 200, img.rows * (1 + (double)MAX_STRETCH_PERCENTAGE / 100) + 200);
    VideoWriter writer;
    video_name = "../../C:\result_diag.avi";
    if(File_save_flag==0){
        SaveFileName = QFileDialog::getSaveFileName(this,"Save File","",".avi");
        video_name = SaveFileName.toStdString();
        video_name += avi;
        video_name.c_str();
    }
    writer.open(video_name, VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, size, true);
    Mat* frame;
    frame = new Mat[frames];
    for (int frameNum = 0; frameNum < frames; frameNum++)
    {
        if (frameNum == 0) {
            copyMakeBorder(img, frame[frameNum], 0, img.rows * (1 + (double)MAX_STRETCH_PERCENTAGE / 100) + 200 - img.rows, 0, img.cols * (1 + (double)MAX_STRETCH_PERCENTAGE / 100) + 200 - img.cols, BORDER_CONSTANT, 0);
            writer.write(frame[frameNum]);
        }
        else
        {
            Mat temp(img.rows * (1 + (double)MAX_STRETCH_PERCENTAGE / (frames - 1) * frameNum * 0.01), img.cols * (1 + (double)MAX_STRETCH_PERCENTAGE / (frames - 1) * frameNum * 0.01), CV_8UC3, Vec3b(0, 0, 0));
            stretch_diag_general(B, (double)MAX_STRETCH_PERCENTAGE / (frames - 1) * frameNum * 0.01, img, temp);
            cout << (double)MAX_STRETCH_PERCENTAGE / (frames - 1) * frameNum << "%" << endl;
            copyMakeBorder(temp, frame[frameNum], 0, img.rows * (1 + (double)MAX_STRETCH_PERCENTAGE / 100) + 200 - temp.rows, 0, img.cols * (1 + (double)MAX_STRETCH_PERCENTAGE / 100) + 200 - temp.cols, BORDER_CONSTANT, 0);
            writer.write(frame[frameNum]);
        }
    }
    if(Get_Back_flag==0){
        for (int frameNum = frames - 1; frameNum >= 0; frameNum--)
            writer.write(frame[frameNum]);
    }
    cout << "done..." << endl;

    delete[] frame;
}

int MainWindow::readvideo() {
    cv::VideoCapture videoCapture(video_name);

    if (!videoCapture.isOpened()) {
        std::cout << "Can't open video !!!" << std::endl;
        return -1;
    }

    cv::Mat videoFrame;


    float videoFPS = videoCapture.get(cv::CAP_PROP_FPS);
    int frnum=0;
    while (true) {
        videoCapture >> videoFrame;
        if (videoFrame.empty()) {
            std::cout << "Video END" << std::endl;
            break;
        }
        string framename=to_string(frnum)+"frame_after.png";
        cv::imwrite(framename,videoFrame);
        frnum++;
        cv::imshow("./result_cols.mp4", videoFrame);

        if (cv::waitKey(1000 / videoFPS) == 27) {
            std::cout << "Stop Video" << std::endl;
            break;
        }
    }
    return 0;
}

void MainWindow::setMatrix()
{
    PC_Mx.SetValue(Primary, 3, 3);
    IPC_Mx.SetValue(InvPrimary, 3, 3);
}


void MainWindow::getRsGsBsfromTXT(QPair<int, double>* Rs, QPair<int, double>* Gs, QPair<int, double>* Bs)
{
    string in_line;
    ifstream in("RsGsBs_Table_DP1.txt");
    //ifstream in("RsGsBs_table.txt");

    int i = 0;
    while (getline(in, in_line))
    {
        int channel = 1;
        istringstream ss(in_line);
        string stringBuffer;
        while (getline(ss, stringBuffer, '\t'))
        {
            QPair<int, double> tmp_pair = qMakePair(254 - i * 2, stod(stringBuffer));
            switch (channel)
            {
            case 1:
                Rs[i] = tmp_pair;
                channel = 2;
                break;
            case 2:
                Gs[i] = tmp_pair;
                channel = 3;
                break;
            case 3:
                Bs[i] = tmp_pair;
                channel = 1;
                break;
            }
        }
        i++;
    }

    in.close();
}

QPair<double, QPair<double, double> > MainWindow::calXYZofAdj_Pixel(int adj_R, int adj_G, int adj_B, QPair<int, double>* Rs, QPair<int, double>* Gs, QPair<int, double>* Bs)
{
    QPair<int, double> tmp_Rs[128];
    QPair<int, double> tmp_Gs[128];
    QPair<int, double> tmp_Bs[128];
    copy(Rs, Rs + 128, tmp_Rs);
    copy(Gs, Gs + 128, tmp_Gs);
    copy(Bs, Bs + 128, tmp_Bs);
    for (int i = 0; i < 128; i++)
    {
        tmp_Rs[i].first = abs(tmp_Rs[i].first - adj_R);
        tmp_Gs[i].first = abs(tmp_Gs[i].first - adj_G);
        tmp_Bs[i].first = abs(tmp_Bs[i].first - adj_B);
    }
    qSort(tmp_Rs, tmp_Rs + 128, cmp_XYZ());
    qSort(tmp_Gs, tmp_Gs + 128, cmp_XYZ());
    qSort(tmp_Bs, tmp_Bs + 128, cmp_XYZ());

    double tmp_RGBs[] = { tmp_Rs[0].second, tmp_Gs[0].second, tmp_Bs[0].second };
    Matrix tmp_RGBs_Mx;
    tmp_RGBs_Mx.SetValue(tmp_RGBs, 3, 1);
    Matrix ans = PC_Mx * tmp_RGBs_Mx;

    return QPair<double, QPair<double, double> >(ans.GetValue(1, 1), QPair<double, double>(ans.GetValue(2, 1), ans.GetValue(3, 1)));
}

QPair<int, QPair<int, int> > MainWindow::calNewRGB(QPair<int, double>* Rs, QPair<int, double>* Gs, QPair<int, double>* Bs, double cal_value_Rs, double cal_value_Gs, double cal_value_Bs)
{
    // Rs, Gs, Bs는 변해서는 안되는데 이러면 변하는 오류 발생
    // 새로운 배열 필요함
    QPair<int, double> tmp_Rs[128];
    QPair<int, double> tmp_Gs[128];
    QPair<int, double> tmp_Bs[128];
    copy(Rs, Rs + 128, tmp_Rs);
    copy(Gs, Gs + 128, tmp_Gs);
    copy(Bs, Bs + 128, tmp_Bs);
    for (int i = 0; i < 128; i++)
    {
        tmp_Rs[i].second = abs(tmp_Rs[i].second - cal_value_Rs);
        tmp_Gs[i].second = abs(tmp_Gs[i].second - cal_value_Gs);
        tmp_Bs[i].second = abs(tmp_Bs[i].second - cal_value_Bs);
    }

    qSort(tmp_Rs, tmp_Rs + 128, cmp_RGBs());
    qSort(tmp_Gs, tmp_Gs + 128, cmp_RGBs());
    qSort(tmp_Bs, tmp_Bs + 128, cmp_RGBs());

    return QPair<int, QPair<int, int> >(tmp_Rs[0].first, QPair<int, int>(tmp_Gs[0].first, tmp_Bs[0].first));
}

Matrix MainWindow::calRsGsBs(QPair<double, QPair<double, double> > adjacentXYZ, double* XYZ, double percentage)
{
    XYZ[0] = percentage * adjacentXYZ.first;
    XYZ[1] = percentage * adjacentXYZ.second.first;
    XYZ[2] = percentage * adjacentXYZ.second.second;

    Matrix XYZ_Mx;
    XYZ_Mx.SetValue(XYZ, 3, 1);

    return IPC_Mx * XYZ_Mx;
}


Matrix MainWindow::calRsGsBs2(QPair<double, QPair<double, double> > adjacentXYZ1, QPair<double, QPair<double, double> > adjacentXYZ2, double* XYZ, double percentage1, double percentage2)
{
    XYZ[0] = adjacentXYZ1.first * percentage1+adjacentXYZ2.first*percentage2;
    XYZ[1] = adjacentXYZ1.second.first * percentage1 + adjacentXYZ2.second.first*percentage2;
    XYZ[2] = adjacentXYZ1.second.second * percentage1 + adjacentXYZ2.second.second * percentage2;

    Matrix XYZ_Mx;
    XYZ_Mx.SetValue(XYZ, 3, 1);

    return IPC_Mx * XYZ_Mx;
}


void MainWindow::on_lineEdit_36_textEdited(const QString &arg1)
{
    this->a = arg1;
    string parameter = a.toStdString();
    howlong = stoi(parameter);
    //frames = (howlong * fps) + 1;
    frames = (howlong * fps);
}

void MainWindow::on_txt_BlockSize_20_activated(int i)
{
    if(i == 0){
        direction=0;
    }
    else if(i==1){
        direction=0;
    }
    else if(i==2){
        direction=1;
    }
    else if(i==3){
        direction=1;
    }
    else if(i==4){
        direction=2;
    }
}

void MainWindow::on_lineEdit_37_textEdited(const QString &arg1)
{
    this->a = arg1;
    string parameter = a.toStdString();
    MAX_STRETCH_PERCENTAGE = stoi(parameter);
}


void MainWindow::on_txt_BlockSize_19_activated(int i)
{
    if(i == 0){
        Block_size = 2;
        BlackWidth = Block_size;
        BrightWidth = Block_size;
        B = 2*Block_size;
    }
    else if(i==1){
        Block_size = 3;
        BlackWidth = Block_size;
        BrightWidth = Block_size;
        B = 2*Block_size;
    }
    else if(i==2){
        Block_size = 4;
        BlackWidth = Block_size;
        BrightWidth = Block_size;
        B = 2*Block_size;
    }
    else if(i==3){
        Block_size = 5;
        BlackWidth = Block_size;
        BrightWidth = Block_size;
        B = 2*Block_size;
    }
    else if(i==4){
        Block_size = 6;
        BlackWidth = Block_size;
        BrightWidth = Block_size;
        B = 2*Block_size;
    }
    else if(i==5){
        Block_size = 7;
        BlackWidth = Block_size;
        BrightWidth = Block_size;
        B = 2*Block_size;
    }
    else if(i==6){
        Block_size = 8;
        BlackWidth = Block_size;
        BrightWidth = Block_size;
        B = 2*Block_size;
    }
    else if(i==7){
        Block_size = 9;
        BlackWidth = Block_size;
        BrightWidth = Block_size;
        B = 2*Block_size;
    }
    else if(i==8){
        Block_size = 10;
        BlackWidth = Block_size;
        BrightWidth = Block_size;
        B = 2*Block_size;
    }
}

void MainWindow::on_txt_BlockSize_22_activated(int flag)
{
    if(flag == 0){
        File_save_flag=0;
    }
    else if(flag == 1){
        File_save_flag=1;
    }
}

void MainWindow::on_Play_button_clicked() //Play Button 누르면 실행되는 함수 (Main함수)
{
    setMatrix();
    getRsGsBsfromTXT(Rs, Gs, Bs); // TXT 파일 파싱
    Mat img(src.rows * B, src.cols * B, CV_8UC3, Scalar(255, 255, 255)); // 스트레처블 구조 완성 이미지 저장
    makingBlock(B, src, blockedimg);
    //makingBlock(B/2, src, blockedimg); //블록화 변경하기 위함
    stretchableForming(B, blockedimg, img);

    imga = img;

    if(direction==0){
        makeVideo_cols(B,frames,fps,imga);
    }
    else if(direction==1){
        makeVideo_rows(B,frames,fps,imga);
    }
    else if(direction==2){
        makeVideo_diag(B,frames,fps,imga);
    }
    readvideo();
    waitKey();
}

void MainWindow::on_txt_BlockSize_21_activated(int flag)
{
    if(flag == 0){
        Get_Back_flag=0;
    }
    else if(flag == 1){
        Get_Back_flag=1;
    }
}
