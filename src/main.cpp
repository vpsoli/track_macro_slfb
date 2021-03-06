// Programa de processamento de imagem e detecção de plugs

#include "opencv2/opencv.hpp"
#include <string.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "functions.h"

using namespace std;
using namespace cv;

//só funciona nesta imagem para isolar a ROI
void getDiameter(Mat& thresholdMat, int d[3]){
    //int base_x0,base_xf,base_y; //diameter of the base
    int mat_rows  = thresholdMat.size[0],
        mat_cols  = thresholdMat.size[1],
		base_xf   = mat_cols/2,
    	base_x0   = mat_cols/2,
    	base_y    = 0;

    for(int j = mat_rows/2; j < mat_rows; j++){
		//navegue as colunas até encontrar uma que seja zero
		//a partir do meio da imagem
    	for(int i = 0; i < mat_cols/2; i++){
    		if ((int)thresholdMat.at<uchar>(j,i) > 100 && base_x0 > i) {
    			//cout << "\n posição: " << "x: " << j << "y: " << i;
    			base_x0 = i;
    		}
    	}
	}
    for(int j = mat_rows/2; j < mat_rows; j++) {
    	//navegue as colunas até encontrar uma que seja zero
    	//a partir do meio da imagem

    	for(int i = mat_cols; i > mat_cols/2; i--){
    	    if ((int)thresholdMat.at<uchar>(j,i) > 100 && i > base_xf) {
    	    	//cout << "\n posição: " << "x: " << j << "y: " << i;
    	    	base_xf = i;
    	    }
    	}
    }
    for (int i = mat_rows; i > mat_rows/2; i--) {
    	//navegue as colunas até encontrar uma que seja zero
    	//a partir do meio da imagem

    	if ((int)thresholdMat.at<uchar>(i, mat_cols/2) > 100) {
    		//cout << "\n posição: " << "x: " << j << "y: " << i;
    		base_y = i;
    		break;
    	}
    }
    rectangle(thresholdMat, Point(base_x0,base_y), Point(base_xf,mat_rows/2), Scalar(255,255,255), 3);
    namedWindow("win", WINDOW_FREERATIO);
    //imshow("win", thresholdMat);
    //waitKey(0);
	d[0] = base_xf - base_x0; //diametro externo
	d[1] = base_x0 + d[0]/2; //linha de centro
	d[2] = base_y; // altura da base

    //cout << d[0] << " " << d[1]<< " " << d[2] << "\n";
}

void hsvImProc(Mat& src,Mat& thh){
    Mat ihs, //matriz convertida em HSV
        blh, //matriz HSV em filtro médio
        mrph; //operador morfológico

    cvtColor(src, ihs, COLOR_RGB2HSV);
    blur(ihs,blh, Size(3,3),Point(-1,-1));
	inRange(blh,Scalar(0,0,100),Scalar(255,1,255),thh);

	for(int i=0;i<3;i++){
        mrph = getStructuringElement(MORPH_RECT,Size(2*i+1,2*i+1));
        morphologyEx(thh,thh,MORPH_CLOSE,mrph);
        morphologyEx(thh,thh,MORPH_OPEN,mrph);
    }
}

void grdImProc(Mat& src,Mat& thg){
    Mat grd, //matriz convertida em escala de cinza
        blg; //matriz em escala de cinza em filtro médio

    cvtColor(src, grd, COLOR_RGB2GRAY);
	blur(grd, blg, Size(5, 5), Point(3, 3));
	threshold(blg,thg,75,255,THRESH_BINARY);

}

float lineDensity(Mat& src,int line,int col_0, int col_f){
    float density = 0;
    for(int i = col_0;i <= col_f; i++){
        density += src.at<uchar>(line, i);
    }
    return density/(float)(col_f-col_0);
}

int main(int argc, char* argv[])
{

	Mat src,thg,thh,crp1,crp2, trot, wrt;
	ofstream arquivo, plugInfo;
	string ensaio = "",
           path = "",
           aux = "",
		   buffer = "";

    int dims[3] = {0,0,0};
    int d_ext = 0, x_celine = 0, base_height = 0,d_in = 0;
    int p_u[50] = {0,0,0,0,0,0,0,0,0,0};
	int p_d[50] = { 0,0,0,0,0,0,0,0,0,0};
    int bed_height = 0, it = 0, plc=0;

    fill_n(p_u,50,0);
    fill_n(p_d,50,0);

    path = "input/image0000.jpg";

    path = samples::findFile(path);
	src = imread(path, IMREAD_COLOR);

    grdImProc(src,thg);
	getDiameter(thg,dims);

	d_ext = dims[0]; x_celine = dims[1]; base_height = dims[2]; d_in=d_ext;

    //cout << dims[0] << " " << dims[1] << dims[2] << endl;

	aux = "output/bed_height.txt";
	arquivo.open(aux,ios::out);
	arquivo << "#Arquivo de dados do tamamanho do leito\n"
			<< "#1ª coluna - tempo [s]; 2ª coluna - altura do leito [mm]\n\n";

	aux = "output/plug_info.txt";
	plugInfo.open(aux,ios::out);
	plugInfo << "#Arquivo de informação dos plugs\n"
			 << "1ª coluna -> tempo [s]; 2ª coluna -> posição do topo do plug [mm]\n"
			 << "3ª coluna -> tempo [s]; 4ª coluna -> posição da base do plug [mm]\n\n";

	//alterar para o número de imagens desejado.

	ostringstream setbuffer;
	vector<double> lambda, height_bed;

	for(int c = 1;c <= 900 ;c++){

		setbuffer << setw(4) << setfill('0') << c;
		path = "input/image" + setbuffer.str() +".jpg";
		path = samples::findFile(path);
		src = imread(path, IMREAD_COLOR);

		//cout << path <<"\n";

		if(src.empty()){
			return -1;
		}

		hsvImProc(src,thh);

		it = base_height;

		while(it > 5){
			if(lineDensity(thh,it,x_celine-d_in/2,x_celine+d_in/2)/255 > 0.60){
				p_d[plc] = it;
				while((lineDensity(thh,it,x_celine-d_in/2,x_celine+d_in/2)/255 > 0.50)&& (it > 0)){
					it -= 1;
					//cout << lineDensity(thh,it,x_celine-d_in/2,x_celine+d_in/2)/255 << " - in\n";
				}
				p_u[plc] = it;
				plc +=1;
			}else{
				while((lineDensity(thh,it,x_celine-d_in/2,x_celine+d_in/2)/255 <= 0.60) && (it > 0)){
					it -= 1;
					//cout << lineDensity(thh,it,x_celine-d_in/2,x_celine+d_in/2)/255 << "\n";
				}
			}
		}
		bed_height = p_u[plc-1];

		//Remove estruturas que não são plugs
		for(int j = 0; j<plc; j++){
			if(p_d[j]-p_u[j] < (d_in+5)){
				for(int k = j; k < plc;k++){
					p_d[k] = p_d[k+1];
					p_u[k] = p_u[k+1];
				}
				plc -= 1;
				j = j-1;
			}
		}

		src.copyTo(wrt);
		for( int j = 0; j < plc; j++){
			rectangle(wrt, Point(x_celine-d_in/2,p_u[j]),
					Point(x_celine+d_in/2,p_d[j]),
					Scalar(255,255,0),3);
		}
        imwrite("frames/image" + setbuffer.str() + ".jpg",wrt);

        for(int j=0;j<plc;j++){
        	lambda.push_back(0.0254*((double)(p_d[j]-p_u[j]))/(double)d_in);
        	plugInfo << setw(10) << setprecision(4) << ((float)(c-1))/60.0 << " "
                     << setw(10) << setprecision(4) << 0.0254*((float)(base_height-p_d[j]))/(float)d_in << " "
                     << setw(10) << setprecision(4) << ((float)(c-1))/60.0 << " "
                     << setw(10) << setprecision(4) << 0.0254*((float)(base_height-p_u[j]))/(float)d_in
                     << setw(10) << setprecision(4) << ((float)(c-1))/60.0 << " "
                     << setw(10) << setprecision(4) << 0.0254*((float)(p_d[j]-p_u[j]))/(float)d_in  << "\n";
         }
         arquivo << setw(10) << setprecision(4) << ((float)(c-1))/60.0 << " "
                 << setw(10) << setprecision(4) << 0.0254*((float)(base_height-bed_height)/(float)d_in) << "\n";

         height_bed.push_back(0.0254*((double)(base_height-bed_height)/(double)d_in));
         //Dados do Estado do Leito
         //cout << "Número de plugs: " << plc <<"\n";
         //cout << "Altura do plug " << c << " - d: " <<25.4*(float)((p_d[c]-p_u[c])/(float)d_in) <<"\n";
         /*
          * for(int j = 0; j <= plc; j++){
          * rectangle(src, Rect(Point(x_celine-d_in/2,p_d[j]), Point(x_celine+d_in/2,p_u[j])), Scalar(255,255,0), 5);
          * }
          */

         //cout << x_celine << " "<< d_in << " " << base_height << "\n";


         src.release();
         setbuffer.str("");
         setbuffer.clear();
         path = "";
         aux = "";
         fill_n(p_u,30,0);
         fill_n(p_d,30,0);
         bed_height = 0, plc = 0;
	}

	double data_height, data_lambda, data_lambda_deviation;

	data_height = average(height_bed);
	data_lambda = average(lambda);
	data_lambda_deviation = standaddeviation(lambda);

	cout << "hmf:         " << data_height << "\n";
	cout << "lambda:      " << data_lambda << "\n";
	cout << "dev:         " << data_lambda_deviation << "\n";
	cout << "lambda/norm: " << data_lambda/0.0254 << "\n";
	cout << "dev/norm:    " << data_lambda_deviation/0.0254 << "\n";

	arquivo.close();
	plugInfo.close();

	return 0;
}
