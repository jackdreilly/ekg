#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "ceres/ceres.h"
#include "ceres/jet.h"
#include "glog/logging.h"
#include <gflags/gflags.h>
#include <iostream>
#include <fstream>	

DEFINE_string(output_image, "/tmp/output_image.jpg", "Output Image Path, optional");
DEFINE_string(input_jpg, "", "Input Image Path, required");
DEFINE_string(output_csv, "", "Output inference csv, optional, defaults to stdout.");
DEFINE_double(smoothness, 1, "Smoothness.");
DEFINE_double(brightness, 1, "Brightness.");


using namespace cv;
using namespace std;

struct NeighborFunctor {
   template <typename T>
   bool operator()(const T* const x, const T* const y, const T* const z, T* residual) const {
     residual[0] = T(FLAGS_smoothness) * ((y[0] - x[0]) - (z[0] - y[0]));
     return true;
   }
};

struct BlackFunctor {
   std::set<double> blacks;
   template <typename T>
   bool operator()(const T* const x, T* residual) const {
   	auto it = blacks.begin();
   	T v = ceres::abs(*it - x[0]);
   	it++;
   	for (;it != blacks.end(); it++) {
   		v = min(ceres::abs(T(*it) - x[0]), v);
   	}
	residual[0] = v;
     return true;
   }
};




int main( int argc, char** argv )
{
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	CHECK(!FLAGS_input_jpg.empty());

    Mat image;
    image = imread(FLAGS_input_jpg, CV_LOAD_IMAGE_COLOR);   // Read the file

    if(! image.data )                              // Check for invalid input
    {
        cout <<  "Could not open or find the image" << std::endl ;
        return -1;
    }
    vector<set<double>> blacks(image.cols);
    for (int row = 0; row < image.rows; ++row) {
    	for (int col = 0; col  < image.cols; ++col) {
    		auto pix = image.at<cv::Vec3b>(row, col);
    		int sum = pix[0] + pix[1] + pix[2];
    		if (sum < FLAGS_brightness) {
    			blacks[col].insert(row);
    		}
    	}
    }
    ceres::Problem problem;
    vector<double> sln(image.cols);
    for (int col = 0; col < image.cols - 2; ++col) {
    	problem.AddResidualBlock(new ceres::AutoDiffCostFunction<NeighborFunctor, 1,1, 1, 1>(new NeighborFunctor()), NULL, &sln[col], &sln[col+1], &sln[col+2]);
    }
    for (int col = 0; col < image.cols; ++col) {
    	auto s = blacks[col];
    	if (s.empty()) {
    		continue;
    	}
    	problem.AddResidualBlock(new ceres::AutoDiffCostFunction<BlackFunctor, 1, 1>(new BlackFunctor{s}), NULL, &sln[col]);
    }
    ceres::Solver::Options options;
	options.minimizer_progress_to_stdout = false;
  	ceres::Solver::Summary summary;
  	ceres::Solve(options, &problem, &summary);
  	if (FLAGS_output_csv.empty()) {
  		for (double x : sln) {
  			cout << x << endl;
  		}
  	} else {
  		ofstream myfile;
  		myfile.open (FLAGS_output_csv);
  		for (double x : sln) {
  			myfile << x << endl;
  		}
  		myfile.close();
  		LOG(ERROR) << "Csv output written to " << FLAGS_output_csv;
  	}

  	if (!FLAGS_output_image.empty()) {
  		for (int col = 0; col < image.cols; ++col) {
  			image.at<cv::Vec3b>(int(sln[col]-1), col) = cv::Vec3b(255,255,0);
  			image.at<cv::Vec3b>(int(sln[col]), col) = cv::Vec3b(255,255,0);
  			image.at<cv::Vec3b>(int(sln[col]+1), col) = cv::Vec3b(255,255,0);
  		}
  		cv::imwrite(FLAGS_output_image, image);
  		LOG(ERROR) << "Image output written to " << FLAGS_output_image;  		
  	}
    return 0;
}
