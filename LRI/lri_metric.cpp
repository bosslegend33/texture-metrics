//c++ libraries
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <stdlib.h>
//opencv libraries
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/contrib/contrib.hpp"
#include "opencv2/core/core.hpp"

std::vector<cv::Mat> compute_diff_im(const cv::Mat& query_im, int no_directions, int k_size, float* thresh, std::vector< std::vector<float> > kernels)
{
	std::vector<cv::Mat> diff_im_vec;
		
	//+compute the threshold value fo the image
	cv::Scalar mean, stddev;
	cv::meanStdDev(query_im, mean, stddev);
	*thresh = stddev.val[0]/2;
	
	/*
	//===DEBUGGING===//
	//print image std/threshold
	std::cout << "mean: " << mean.val[0] << std::endl;
	std::cout << "std: " << stddev.val[0] << std::endl;
	//===//
	*/
	
	//+global parameters to convolve kernels with the image
	cv::Point anchor;
	double delta = 0.;
	cv::Ptr<cv::FilterEngine> im_filter;
	
	for(int c = 0; c < 2; c++) //this is just to stack the diff_im in more convenvient manner
	{
		for(int d = 0; d < no_directions; d++)
		{
			for(int i = 0; i < (k_size+1); i++)
			{
				//===DEBUGGING===//
				//std::cout << "* " << d << " * " << (d*(k_size+1)+i) << std::endl;
				//===//
				cv::Mat col_kernel = cv::Mat( kernels.at( (d*(k_size+1)+i) ) );
				cv::Mat diff_im = cv::Mat(query_im.size(),query_im.type());
				cv::Mat diff_im_comp = cv::Mat(query_im.size(),query_im.type());
				cv::Mat ker;
				cv::Mat tmp_query_im;
				switch(d)
				{
					//===NOTE: to convolve matrixes it is necessary to do zero padding
					//in the edges of the source image. The fcn im_filter only does
					//this in the right and bottom edges of the images. Thus, instead
					//of using fcn copyMakeBorder, images were flipped across y or x
					//then convolved and then flipped back ===//
					 
					case 0: //diff-im with horizontal kernels
						anchor = cv::Point(0,0);
						ker = col_kernel.reshape(0,1);
						if(c==0){
							im_filter = cv::createLinearFilter(query_im.type(), ker.type(), ker, anchor, 
								delta, cv::BORDER_CONSTANT, cv::BORDER_CONSTANT, cv::Scalar(0));
							im_filter->apply(query_im, diff_im);
							diff_im_vec.push_back(diff_im);
						}else{
							cv::flip(query_im, tmp_query_im, 1);
							im_filter = cv::createLinearFilter(query_im.type(), ker.type(), ker, anchor, 
								delta, cv::BORDER_CONSTANT, cv::BORDER_CONSTANT, cv::Scalar(0));
							im_filter->apply(tmp_query_im, diff_im_comp);
							cv::flip(diff_im_comp, diff_im_comp, 1);
							diff_im_vec.push_back(diff_im_comp);
						}
						break;
						
					case 1: //diff-im with vertical kernel
							anchor = cv::Point(0,0);
							ker = col_kernel;
							if(c==0){
							im_filter = cv::createLinearFilter(query_im.type(), ker.type(), ker, anchor, 
								delta, cv::BORDER_CONSTANT, cv::BORDER_CONSTANT, cv::Scalar(0));
							im_filter->apply(query_im, diff_im);
							diff_im_vec.push_back(diff_im);
						}else{						
							cv::flip(query_im, tmp_query_im, 0);
							im_filter = cv::createLinearFilter(query_im.type(), ker.type(), ker, anchor, 
								delta, cv::BORDER_CONSTANT, cv::BORDER_CONSTANT, cv::Scalar(0));
							im_filter->apply(tmp_query_im, diff_im_comp);
							cv::flip(diff_im_comp, diff_im_comp, 0);
							diff_im_vec.push_back(diff_im_comp);
						}
						break;
					
					case 2: //diff-im with 45° downwards kernel
						anchor = cv::Point(0,0);
						ker = col_kernel.reshape(0,i+2);
						if(c==0){
							im_filter = cv::createLinearFilter(query_im.type(), ker.type(), ker, anchor, 
								delta, cv::BORDER_CONSTANT, cv::BORDER_CONSTANT, cv::Scalar(0));
							im_filter->apply(query_im, diff_im);
							diff_im_vec.push_back(diff_im);
						}else{
							cv::flip(query_im, tmp_query_im, 1);
							im_filter = cv::createLinearFilter(query_im.type(), ker.type(), ker, anchor, 
								delta, cv::BORDER_CONSTANT, cv::BORDER_CONSTANT, cv::Scalar(0));
							im_filter->apply(tmp_query_im, diff_im_comp);
							cv::flip(diff_im_comp, diff_im_comp, 1);
							diff_im_vec.push_back(diff_im_comp);
						}
						break;
						
					case 3: //diff-im with +45 kernel
						anchor = cv::Point(i+1,i+1);
						ker = col_kernel.reshape(0,i+2);
						if(c==0){
							im_filter = cv::createLinearFilter(query_im.type(), ker.type(), ker, anchor, 
								delta, cv::BORDER_CONSTANT, cv::BORDER_CONSTANT, cv::Scalar(0));
							im_filter->apply(query_im, diff_im);
							diff_im_vec.push_back(diff_im);
						}else{
							cv::flip(query_im, tmp_query_im, 1);
							im_filter = cv::createLinearFilter(query_im.type(), ker.type(), ker, anchor, 
								delta, cv::BORDER_CONSTANT, cv::BORDER_CONSTANT, cv::Scalar(0));
							im_filter->apply(tmp_query_im, diff_im_comp);
							cv::flip(diff_im_comp, diff_im_comp, 1);
							diff_im_vec.push_back(diff_im_comp);
						}
						break;
						
					default:
						std::cout << "Invalid direction (2)" << std::endl;
				}
				~diff_im;
				~ker;
				~col_kernel;
				~tmp_query_im;
			}
		}
	}
	//std::cout << diff_im_vec.size() << std::endl;
	
	return diff_im_vec;
	
}

void compute_LRIa(int no_directions, int k_size, float thresh, std::vector<cv::Mat> diff_im_vec, cv::Mat& histograms)
{
	
	for(int d = 0; d < no_directions*2; d++)
	{
		cv::Mat hist(1,2*k_size+1, CV_32SC1, cv::Scalar(0)); //values range is from -K to K
		
		for(int pixr = (k_size+1); pixr < (diff_im_vec.at(0).rows-(k_size+1)); pixr++)
		{
			for(int pixc = (k_size+1); pixc < (diff_im_vec.at(1).cols-(k_size+1)); pixc++)
			{
				int pix_count = 0;
				bool big_or_small = true;
				bool pix_count_flag = true;
				//std::cout << "# " << std::endl;
				
				for(int sz = 0; sz < (k_size+1) && pix_count_flag == true; sz++)
				{
					//obtain pixel-dif value from the correspondant matrix
					cv::Mat diff_im = diff_im_vec.at((d*(k_size+1))+sz);
					float val = diff_im.at<float>(pixr,pixc);
					
					if(sz == 0) //for the adjacent pixel, set gradient
					{
						if(std::abs(val) < thresh)
						{   //std::cout << "+ " << k_size << std::endl;
							hist.at<int>(0,k_size) += 1 ; pix_count_flag = false;}
						else if (val < 0) //bigger
						{ big_or_small = true; pix_count++;}
						else              //smaller
						{ big_or_small = false; pix_count++;}
					}
					else //check further adjacent pixels
					{
						if(std::abs(val) < thresh) //stop condition regardless gradient
						{
							if(big_or_small)
							{   //std::cout << "+ " << k_size+std::min(pix_count,k_size) << std::endl;
								hist.at<int>(0,k_size+std::min(pix_count,k_size)) += 1;}
							else
							{   //std::cout << "+ " << k_size-std::min(pix_count,k_size) << std::endl;
								hist.at<int>(0,k_size-std::min(pix_count,k_size)) += 1;}
							
							pix_count_flag = false;

						}
						else
						{
							if( (val < 0 && big_or_small) || (val > 0 && !big_or_small))
								pix_count++;
							else
							{
								if(big_or_small)
								{   //std::cout << "+ " << k_size+std::min(pix_count,k_size) << std::endl;
								hist.at<int>(0,k_size+std::min(pix_count,k_size)) += 1;}
								else
								{   //std::cout << "+ " << k_size-std::min(pix_count,k_size) << std::endl;
								hist.at<int>(0,k_size-std::min(pix_count,k_size)) += 1;}
								
								pix_count_flag = false;
							}
							
						}
					}
				} //end of pixels comparison - go to compare the next sz-neighbor
				if(pix_count_flag) //if the size limit was reached
				{
					if(big_or_small)
					{   //std::cout << "!! " << k_size+std::min(pix_count,k_size) << std::endl;
						hist.at<int>(0,k_size+std::min(pix_count,k_size)) += 1;}
					else
					{   //std::cout << "!! " << k_size-std::min(pix_count,k_size) << std::endl;
						hist.at<int>(0,k_size-std::min(pix_count,k_size)) += 1;}
				}
				 			 
			}//-|change row and col indexes
		}//-----|then go to evaluate the next pixel
		
		//insert the resultant histogram that represents direction d
		//std::cout << hist << std::endl;
		histograms.push_back(hist);
	}
	/*
	//===DEBUGGING===//	
	//print obtained histograms
	for(int r = 0; r < histograms.rows; r++)
	{
		std::cout << histograms.row(r) << std::endl;
		std::cout << "***" << std::endl;
	}
	//===//
	*/
}

void compute_norm_feat_vec(const cv::Mat& mul_feat, cv::Mat& feat_vec)
{
	feat_vec = mul_feat.reshape(0,1);
	feat_vec.convertTo(feat_vec, CV_32FC1);
	normalize(feat_vec, feat_vec, 1, 0, cv::NORM_L1, -1, cv::Mat() );
	
	//===DEBUGGING===//	
	//print feature vectors
	//std::cout << feat_vec << std::endl;
	//===//	
}

//====================================================================================================================================
//*** MAIN METHOD ***//
int main ( int argc, char *argv[] )
{
	//receive query and ground-truth image
	//===NOTE: specify type of image in imread, otherwise opencv creates matrix with 3 channels===
	cv::Mat query_im = cv::imread("/home/arturokkboss33/DataSets/Gustaf/floor1/floor1-a-p004.png", CV_LOAD_IMAGE_GRAYSCALE);
	cv::Mat baseline_im = cv::imread("/home/arturokkboss33/DataSets/Gustaf/floor1/floor1-a-p003.png", CV_LOAD_IMAGE_GRAYSCALE);
	query_im.convertTo(query_im, CV_32FC1);
	baseline_im.convertTo(baseline_im, CV_32FC1);
	
	//+++INITIAL PARAMETERS+++
	int k_size = 4; //size limit of the area to analyze
	float thresh = 0.; //value that determines if a pixel is an edge
	
	//+++CREATION OF DIFFERENCE IMAGES+++
	//++create a kernel for each of the 4 directions around the pixel
	//===NOTE: the fixed order is horizontal, vertical, \ 45° downwards, \ 45° upwards
	//horizontal and vertical kernels are consider the same because one
	//can be transposed from the other ===
	std::vector< std::vector<float> > kernels;
	int no_directions = 4;
	for(int d = 0; d < no_directions; d++)
	{
		//+traverse through the different kernel sizes
		for(int i = 1; i <= (k_size+1); i++)
		{
			std::vector<float> data;
			int kernel_size = (int)(std::pow( (double)(i+1), 2. ));

			switch(d)
			{
				case 0: //horizontal kernel				
					data.push_back(1.);
					for(int j = 0; j < (i-1); j++)
						data.push_back(0.);
					data.push_back(-1.);
					break;	
				
				case 1: //vertical kernel
					data.push_back(1.);
					for(int j = 0; j < (i-1); j++)
						data.push_back(0.);
					data.push_back(-1.);
					break;	
					
				case 2: // 45 downwards 
					data.push_back(1.);
					for(int j = 0; j < (kernel_size-2); j++)
						data.push_back(0.);
					data.push_back(-1.);
					break;
				
				case 3: // 45 upwards
					data.push_back(-1.);
					for(int j = 0; j < (kernel_size-2); j++)
						data.push_back(0.);
					data.push_back(1.);
					break;
				default:
					std::cout << "Invalid direction" << std::endl;
								
			}	
			kernels.push_back(data);
		}
	}
	
	/*
	//===DEBUGGING===//
	//print the computed kernels
	for(std::vector< std::vector<float> >::iterator it = kernels.begin(); it != kernels.end(); ++it)
	{
		std::vector<float> tmp = *it;
		for(std::vector<float>::iterator it2 = tmp.begin(); it2 != tmp.end(); ++it2)
		{
			std::cout << *it2 << " ";
		}
		std::cout << std::endl;
	}
	//===//
	*/

	//++with the kernels, compute the difference images
	//cv::Rect roi = cv::Rect(0,0,10,10);
	//cv::Mat test_im = query_im(roi);
	//test_im.convertTo(test_im, CV_32FC1);
	
	//+++DIFFERENCE IMAGE AND HISTOGRAM COMPUTATION+++
	cv::Mat histograms_query(0,2*k_size+1, CV_32SC1);
	cv::Mat histograms_base(0,2*k_size+1, CV_32SC1);
	std::vector<cv::Mat> difference_images_query( compute_diff_im(query_im, no_directions, k_size, &thresh, kernels) );
	compute_LRIa(no_directions, k_size, thresh, difference_images_query, histograms_query);
	std::vector<cv::Mat> difference_images_base( compute_diff_im(baseline_im, no_directions, k_size, &thresh, kernels) );
	compute_LRIa(no_directions, k_size, thresh, difference_images_base, histograms_base);
	
	//+++HISTOGRAM COMPARISON+++
	cv::Mat feature_query, feature_base;
	compute_norm_feat_vec(histograms_query, feature_query);
	compute_norm_feat_vec(histograms_base, feature_base);
	
	//===NOTE: The similiraity score using BHATACHARYYA ranges from 0 to 1
	//where 0 indicates that two images are the same ===//
	double score = compareHist(feature_query, feature_base, CV_COMP_BHATTACHARYYA);
	std::cout << "Similarity score: " << score << std::endl;
	
	/*
	//===DEBUGGING===//	
	//print the difference images
	std::cout << test_im << std::endl;
	std::cout << "***" << std::endl;
	for(std::vector< cv::Mat >::iterator it = difference_images.begin(); it != difference_images.end(); ++it)
	{
		cv::Mat tmp = *it;
		//std::cout << test_im << std::endl;
		//std::cout << "***" << std::endl;
		std::cout << tmp << std::endl;
		std::cout << "***" << std::endl;
	}
	//===//
	*/
	
		
	return 0;
}


//test code for filter creation
	/*
	//cv::Point anchor( -1 ,-1 );
	//double delta = 0;
	
	//float data[3][5] = {{11,11,11,11,11},{22,22,22,22,22}, {44,44,44,44,44}};
	float data[15] = {11,11,11,11,11,22,22,22,22,22,44,44,44,44,44};
	//float kernel[3][3] = {{1,0,0},{0,0,0},{0,0,-1}};
	float kernel[1][3] = {1,0,-1};

	cv::Mat src = cv::Mat(3, 5, CV_32FC1, &data);
	cv::Mat ker = cv::Mat(1, 3, CV_32FC1, &kernel);
	cv::Mat dst = cv::Mat(src.size(), src.type());

	cv::Ptr<cv::FilterEngine> fe =  cv::createLinearFilter(src.type(), ker.type(), ker, anchor,
			delta, cv::BORDER_CONSTANT, cv::BORDER_CONSTANT, cv::Scalar(0));
	fe->apply(src, dst);
	std::cout << src << std::endl;
	std::cout << dst << std::endl;
	*/
