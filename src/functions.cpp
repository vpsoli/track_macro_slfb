#include "functions.h"

double average(vector<double>& vector){
	double sum = 0;
	for(uint i=0;i < vector.size(); i++){
		sum += vector[i];
	}
	return sum/((double)vector.size());
}
double standaddeviation(vector<double>& vector){
	double variable_average = average(vector);
	double delta, sum = 0;
	for(uint i=0;i < vector.size();i++){
		delta = pow(variable_average - vector[i],2);
		sum += delta;
	}
	return sqrt(sum)/((double)vector.size());
}
