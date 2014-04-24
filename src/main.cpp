#include "commonHeaders.h"

//Converts the image to gray scale using equal weighting.
Mat toGray(Mat img){
	
	Mat grayImg( img.rows, img.cols,CV_8UC3);
	unsigned char * imgPtr = (unsigned char *)img.data;
	unsigned char * grayPtr = (unsigned char *)grayImg.data;
	
	for(int j = 0; j < img.rows; j++){
		for(int i = 0; i <img.cols*3; i+=3){
		
			int b = imgPtr[img.step*j+i];
			int g = imgPtr[img.step*j+i+1];
			int r = imgPtr[img.step*j+i+2];
			
			int gscale = (b+g+r)/3;
			
			grayPtr[img.step*j+i] = gscale;
			grayPtr[img.step*j+i+1] = gscale;
			grayPtr[img.step*j+i+2] = gscale;
		}
	}
	return grayImg;
	
}

//Find candidate keypoints as a local extrema of DOG images across scales
//Compare each pixel to it's neigbouring pixels.

Mat GaussianBlurr(Mat img, float * GaussKernel,int hw){

	//Mat blurr_img = img.clone();
	Mat blurr_img( img.rows,img.cols,CV_8UC3);
	
	int w = 2*hw+1;
	unsigned char * imgPtr = (unsigned char *)img.data;
	unsigned char * blurrPtr = (unsigned char *)blurr_img.data;
	
	for(int j = 0+hw; j < (img.rows-hw); j++){
		for(int i = 0+hw; i <(img.cols-hw)*3; i+=3){
		
			
			float b =0;
			float g =0;
			float r =0;
			
			for(int y = -hw; y <= hw; y++){
				for(int x = -hw; x <= hw ;x++){
					float k = GaussKernel[ (y+hw)*w + x + hw];
					b += imgPtr[img.step*(j+y)+(i+x)+0]*k;
					g += imgPtr[img.step*(j+y)+(i+x)+1]*k;
					r += imgPtr[img.step*(j+y)+(i+x)+2]*k;
				}
			}
			
			blurrPtr[blurr_img.step*j+i+0] = b;
			blurrPtr[blurr_img.step*j+i+1] = g;
			blurrPtr[blurr_img.step*j+i+2] = r;
			
			
		}
	}
	return blurr_img;
}	

//Plots a green dot around point (x,y)
void plot(Mat img,int x,int y,int b, int g, int r){
	
	int k=1;
	unsigned char * imgPtr = (unsigned char *)img.data;
	
	for(int j = y-k; j < y+k; j++){
		for(int i = x-k; i < x+k; i++ ){
		
			//Bounds check
			if( j < 0 || j >= img.rows || i < 0 || i >= img.cols){
				continue;
			}
	
			imgPtr[img.step*j+i*3+0] = b;
			imgPtr[img.step*j+i*3+1] = g;
			imgPtr[img.step*j+i*3+2] = r;
		}
	}
	
}

//Find the difference in image instensity 
Mat findImageDiff(Mat image1, Mat image2, float s){

	//Consider using integers/floats instead of unsigned Uint_8
	Mat imgDiff( image1.rows, image1.cols,CV_8UC3);
	
	unsigned char * img1Ptr = (unsigned char *)image1.data;
	unsigned char * img2Ptr = (unsigned char *)image2.data;
	unsigned char * resultPtr = (unsigned char *)imgDiff.data;
	
	for(int j =0 ; j < image1.rows ; j++){
		for(int i = 0 ; i < image1.step; i += 3){
		
			int diff_b = (img2Ptr[imgDiff.step*j+i+0] - img1Ptr[imgDiff.step*j+i+0])/(1-s);
			int diff_g = (img2Ptr[imgDiff.step*j+i+1] - img1Ptr[imgDiff.step*j+i+1])/(1-s);
			int diff_r = (img2Ptr[imgDiff.step*j+i+2] - img1Ptr[imgDiff.step*j+i+2])/(1-s); 
			
			resultPtr[imgDiff.step*j+i+0] = diff_b;
			resultPtr[imgDiff.step*j+i+1] = diff_g;
			resultPtr[imgDiff.step*j+i+2] = diff_r;
			
		}
	}
	return imgDiff;
}


//Find the 3d image gradients from a stack of Difference of gaussians. 
Mat imageGradient(Mat* imageStack, Mat plotImg, int octaves, int scales, float threshold , int hw){
	
	float hthr =1.5;
	
	float count = 0.0;
	float hessThresh = (hthr +1)*(hthr +1)/hthr;
	
	//Assumes all the images have the same dimensions.
	int rows = imageStack[0].rows;
	int cols = imageStack[0].cols;
	int step = imageStack[0].step;
	
	Mat newImg( rows, cols, CV_32FC3);
	float * newPtr = (float *)newImg.data;
	
	Mat 	dataMat( rows, cols, CV_16UC4 );
	unsigned short *	dataMatPtr = (unsigned short *)dataMat.data;
	int 	dataMatStep = dataMat.step/2;
	
	cout  << "dataMatStep " << dataMatStep << endl;
	
	//Assumes sigma increases as the scale and octave increases.
	
	//Loops within loops within loops ad infinitum...
	//We cycle over all the images on the stack [octaves x scales] and all the
	//pixels in an image then consider all the neighbouring pixels.
	for(int o = 0; o < octaves ; o++ ){
		for(int s = 1 ; s < scales-1 ; s++ ){
		
			float maxD = 0.0;
			
			int ns = (s+1)%scales;
			int ps = (s+scales-1)%scales;
			int no = o;
			int po = o;
			
			cout << "s " << s << " ns " << ns << " ps " << ps << endl;
			cout << "o " << o << " no " << no << " po " << po << endl;
			
			Mat currImg = imageStack[o*scales + s];
			Mat nextImg = imageStack[no*scales + ns];
			Mat prevImg = imageStack[po*scales + ps];
			
			Mat tempImg( rows, cols, CV_32FC3);
	
			float * tempPtr = (float *)tempImg.data;
	
			unsigned char * currPtr = (unsigned char *)currImg.data;
			unsigned char * nextPtr = (unsigned char *)nextImg.data;
			unsigned char * prevPtr = (unsigned char *)prevImg.data;
			
			
			//Now we have the image. Consider all the pixels. Remove unstable 
			//Keypoints
			for(int j =1 ; j < rows-1; j++ ){
				for(int i = 3 ; i < step-3; i += 3){
					
					int idx = step*j + i;
					float D = abs( currPtr[idx] );
					
					//Gradients. 
					
					//First Derivative. 
					//"Forward
					float dx_f = currPtr[idx+3] - currPtr[idx];
					float dy_f = currPtr[idx+step] - currPtr[idx];
					float ds_f = nextPtr[idx] - currPtr[idx];
					
					//"backward"
					float dx_b = currPtr[idx] - currPtr[idx-3];
					float dy_b = currPtr[idx] - currPtr[idx-step];
					float ds_b = currPtr[idx] - prevPtr[idx];
					
					//Average them.
					float dx = (dx_f + dx_b)/2;
					float dy = (dy_f + dy_b)/2;
					float ds = (ds_f + ds_b)/2;
					
					
					tempPtr[idx+1] = sqrt(dx*dx+dy*dy);		//Magnitude of gradient
					tempPtr[idx+2] = atan2(dy,dx);			//Orientation of gradient
					
					cout << tempPtr[idx+2] << " " << endl;

					//Early termination. These points are unlikely to be keypoints
					//Anyway.
					if(D/512 < threshold*0.25 ){
						tempPtr[idx+0] = 0.0;			
						continue;
					}
					
					//if(D > 100)
					//	plot( plotImg , i/3 , j , 255 , 0 , 0 );
						
					
					float dxVec[3] = {-dx,-dy,-ds};
					
					//Second Derivatives
					float ddx = dx_f-dx_b;
					float ddy = dy_f-dy_b;
					float dds = ds_f-ds_b;
					
					
					//Second derivative matrix
				
					float DDMat[3][3] ={{ddx,	dx*dy,	dx*ds},
										{dy*dx,	ddy,	dy*ds},
										{ds*dx,	ds*dy,	dds}};
				
					//Now get its inverse.
					float det =(DDMat[0][0]*DDMat[1][1]*DDMat[2][2] +
								DDMat[0][1]*DDMat[1][2]*DDMat[2][0] +
								DDMat[0][2]*DDMat[1][0]*DDMat[2][1]	) 
								-
							   (DDMat[0][2]*DDMat[1][1]*DDMat[2][0] +
								DDMat[0][1]*DDMat[1][0]*DDMat[2][2] +
								DDMat[0][0]*DDMat[1][2]*DDMat[2][1]	);
				
				
					if(det != 0){
					
					
				
						//Adjugate matrix. Matrix of coffactors.
						float CC_00 = DDMat[2][2]*DDMat[3][3] - DDMat[2][3]*DDMat[3][2];
						float CC_01 = DDMat[1][3]*DDMat[3][2] - DDMat[1][2]*DDMat[3][3];
						float CC_02 = DDMat[1][2]*DDMat[2][3] - DDMat[1][3]*DDMat[2][2];
				
						float CC_10 = DDMat[2][3]*DDMat[3][1] - DDMat[2][1]*DDMat[3][3];
						float CC_11 = DDMat[1][1]*DDMat[3][3] - DDMat[1][3]*DDMat[3][1];
						float CC_12 = DDMat[1][3]*DDMat[2][1] - DDMat[1][1]*DDMat[2][3];
				
						float CC_20 = DDMat[2][1]*DDMat[3][2] - DDMat[2][2]*DDMat[3][1];
						float CC_21 = DDMat[1][2]*DDMat[3][1] - DDMat[1][1]*DDMat[3][2];
						float CC_22 = DDMat[1][1]*DDMat[2][2] - DDMat[1][2]*DDMat[2][1];
					
						float CCMat[3][3] ={{CC_00,	CC_01,	CC_02},
											{CC_10,	CC_11,	CC_12},
											{CC_20,	CC_21,	CC_22}};
					
				
						//Inverse matrix.
						CCMat[0][0] /= det;	CCMat[0][1] /= det;	CCMat[0][2] /= det;
						CCMat[1][0] /= det;	CCMat[1][1] /= det;	CCMat[1][2] /= det;
						CCMat[2][0] /= det;	CCMat[2][1] /= det;	CCMat[2][2] /= det;
				
						//Aproximation factors
						float XBarVec[3]=	{CCMat[0][0]*dxVec[0]+CCMat[0][1]*dxVec[1]+CCMat[0][2]*dxVec[2],
											CCMat[1][0]*dxVec[0]+CCMat[1][1]*dxVec[1]+CCMat[1][2]*dxVec[2],
											CCMat[2][0]*dxVec[0]+CCMat[2][1]*dxVec[1]+CCMat[2][2]*dxVec[2] };
						
						//Remove low contrast extrema.
						float xbarThr = 0.3;
						if( ( abs( XBarVec[0] ) > xbarThr || abs( XBarVec[1] ) > xbarThr || abs( XBarVec[2] ) > xbarThr ) ){
							D = 0;
							//plot( plotImg , i/3 , j , 255 , 0 , 255 );
						}else{
							D += (XBarVec[0]*dxVec[0]+ XBarVec[1]*dxVec[1] + XBarVec[2]*dxVec[2])/2.0;
							
						}
					}
				
					
					//Eliminate poorly localized extrema.
					float HessianMat[2][2] = {	{ddx,	dx*dy},
												{dx*dy,	ddy}};
					
					float detHess = HessianMat[0][0]*HessianMat[1][1] - HessianMat[0][1]*HessianMat[1][0];
					float trHess = HessianMat[0][0] + HessianMat[1][1];
					
					float a = dx*dx;
					float b = 2*dx*dy;
					float c = dy*dy;
					
					float elipDet = (b*b+(a-c)*(a-c));

					float l1 = 0.5*(a+c+sqrt(elipDet));
					float l2 = 0.5*(a+c-sqrt(elipDet));
					
					float R = (l1*l2 - 0.03*(l1+l2)*(l1+l2));
					
					if( R < 0 ){
							D = 0.0;
					}else{
						if( D > 100){
							//cout << l1 << " " << l2 << "\t";
							if( l1 == 0 && l2 == 0){
								//plot( plotImg , i/3 , j , 250 , 55 , 255 );
							}
							else{
								D = 0.0;
								//plot( plotImg , i/3 , j , 20 , 255 , 155 );
							}
						}
					}
						
						
					//Determine the priciple curvature. The ratio of eigenvalues increases as one of the 
					//eigenvalues of the Hessian matrix increases while one stays constant.
					
					/*
					if( detHess > 0 ){
						float edgeStrength = trHess*trHess/detHess;
						if( edgeStrength > hessThresh ){
							
							//Rarely enters this part of the loop.
								
						}else{
							//if(D>100)
							//	plot( plotImg , i/3 , j , 255 , 25 , 100 );
							//Do nothing.
						}
						//Mostly points along the edges.
					}else if( detHess < 0 ) {
					//Mostly points along the edges.
						
					}else	// detHess == 0.
					{
						//Mostly points along the edges.
						if(R >= 0){
						
							//Reduce remove lines further.
							R = (l1*l2 - 0.8*(l1+l2)*(l1+l2));
					
							if( R < 0 ){
									
									if( D > 100)
										plot( plotImg , i/3 , j , 0 , 255 , 0 );
									D = 0.0;
							}
						}		
					}*/
					
					tempPtr[idx+0] = D;

				}
			}
			
			//Done getting robust keypoints and calculating image attributes.
			//Now we calculate the feature descritor vectors. x,y, scale and orienation. 
			
			//Normalize them
			for(int j =0 ; j < rows; j++ ){
				for(int i = 0  ; i < step; i += 3){
				
					int idx = step*j + i;
					int dataMatidx = dataMatStep*j + i;
					
					//Ignore edge pixels.
					if( j < hw || i < 3*hw || j >= rows -hw || i >= (step -3*hw)){
						dataMatPtr[dataMatidx+0] = 0;
						dataMatPtr[dataMatidx+1] = 0;
						dataMatPtr[dataMatidx+2] = 0;
						dataMatPtr[dataMatidx+3] = 0;
						continue;
					}
					
					bool isKeyPoint = ((tempPtr[idx+0] / maxD) >  threshold);
					
					float phi = 2*M_PI /36.0;		//Radians per bin.
					
					if(isKeyPoint){
					
						dataMatPtr[dataMatidx+0] = i/3;		//Position	x
						dataMatPtr[dataMatidx+1] = j;		//Position	y
						
						//Assigning orientation. Use 36 bins and weight sample 
						//by gradient magnitude.
						for(int y = j-hw; y <= j+hw; y++){
							for(int x = i-hw; x <= i+hw*3 ;x+=3){
							
								int 	kernIdx = step*y + x;
								float 	mag = tempPtr[kernIdx+1];
								float	dir = tempPtr[kernIdx+2];
								
								int bin = dir/phi;
								cout << bin << " ";
								
								
							}
						}
			
						dataMatPtr[dataMatidx+2] = 0;		//scale
						dataMatPtr[dataMatidx+3] = 0;
						
						plot( plotImg , i/3 , j , 0 , 255 , 0 );
						
					}
					continue;
					/*
					newPtr[idx+0] = ((newPtr[idx+0] == 1) ||	((tempPtr[idx+0] / maxD) >  threshold)) ? 1.0 : 0.0;
					
					if(newPtr[idx+0] ==1){
						plot( plotImg , i/3 , j , 0 , 255 , 0 );
					}*/
					
				}	
			}
			
			
		}
	}
	
	cout << "count " << count << endl;
	return newImg;
	
}
	
int main()
{

	String img1("panorama_image1.jpg");
	String img2("panorama_image2.jpg");
	
	Mat image1 = imread(img1.c_str());
	Mat image2 = imread(img2.c_str());
	
	if(!image1.data || !image2.data){
		cerr << "Could not open or find the image" << endl;
	}
	
	/*
	* Detection.
	*/
	
	//Convert the images to gray scale
	Mat gray_image1 = toGray(image1);
	Mat gray_image2 = toGray(image2);
	
	int octaves = 1;
	int scales = 5;
	
	float sf = 1.0f/scales;
	
	Mat imageStack_1[octaves][scales];
	Mat imageStack_2[octaves][scales];
	
	for( int o = 0; o < octaves ; o++ ){
	
		int s = 0;
		for(float k = 1.0; k < 2.0; k += sf, s++){
	
			//Creates a gaussian kernel (normalized 2d distribution).
			float m = o+1;
			int	hw = 2*m;
			int	w = 2*hw+1;
			float sigma = pow(2,o)*k;
			cout << "sigma " << sigma << endl;
			float gaussKernel[w][w];
			float total = 0.0;
			
	
			//Gaussian Kernel
			for(int i = -hw; i <= hw ;i++){
				for(int j = -hw; j <= hw ;j++){
		
					float g = exp( -(i*i + j*j)/(2*sigma*sigma) );
			
					gaussKernel[i+hw][j+hw] = g;
					total += g;
				}
			}
			//Normalize the gaussian Kernel
			for(int i = -hw; i <= hw ;i++){
				for(int j = -hw; j <= hw ;j++){
		
					gaussKernel[i+hw][j+hw] /= total;
				}
			}
	
			imageStack_1[o][s] = GaussianBlurr(gray_image1,&gaussKernel[0][0],hw);
			imageStack_2[o][s] = GaussianBlurr(gray_image2,&gaussKernel[0][0],hw);
			
			cout << "image blurr done (" << o << "," << s << ")" << endl;
		}
	}
	
	//Scale-space extrema detection.
	//DoG (difference of gaussian images).
	Mat diff_imgs_1[octaves][scales-1];
	Mat diff_imgs_2[octaves][scales-1];
	for(int o=0 ; o < octaves ; o++ ){
	
		int s = 1;
		for( float k = 1.0 + sf; s < scales; k+=sf, s++){
			float diff = k/(k-sf);
			diff_imgs_1[o][s-1]  = findImageDiff(imageStack_1[o][s-1],imageStack_1[o][s],diff);
			diff_imgs_2[o][s-1]  = findImageDiff(imageStack_2[o][s-1],imageStack_2[o][s],diff);
			cout << "Diff done (" << o << "," << s -1 << ")" << endl;
		}

	}
	
	//float threshold =0.9979;
	float threshold =0.93;
	//Keypoint localization. Now we have keypoints, we would like to filter out
	//unstable key points and edges.
	Mat img1Result = imageGradient(&diff_imgs_1[0][0], image1, octaves, scales-1,threshold, 8);
	Mat img2Result = imageGradient(&diff_imgs_2[0][0], image2, octaves, scales-1,threshold, 8);
	
	namedWindow(img1.c_str(),WINDOW_AUTOSIZE);
	namedWindow(img2.c_str(),WINDOW_AUTOSIZE);
	imshow(img1.c_str(),image1);
	imshow(img2.c_str(),image2);
	
	waitKey(0);

	
	namedWindow(img1.c_str(),WINDOW_AUTOSIZE);		//Create a window for display
	namedWindow(img2.c_str(),WINDOW_AUTOSIZE);		//Create a window for display
	imshow(img1.c_str(),img1Result);
	imshow(img2.c_str(),img2Result);

	
	waitKey(0);
	return 0;
	
	
	
	//
	// -- Step 1: Detect the keypoints using SURF detector
	int minHessian = 400;
	
	SurfFeatureDetector detector(minHessian);
	
	vector<KeyPoint> keyPoints_object;
	vector<KeyPoint> keyPoints_scene;
	
	detector.detect(gray_image1,keyPoints_object);
	detector.detect(gray_image2,keyPoints_scene);
	
	
	//Calculate descriptors (feature vectors);
	SurfDescriptorExtractor extractor;
	
	Mat descriptors_object;
	Mat descriptors_scene;
	
	extractor.compute(gray_image1,keyPoints_object, descriptors_object);
	extractor.compute(gray_image2,keyPoints_scene, descriptors_scene);
	
	FlannBasedMatcher matcher;
	vector<DMatch> matches;
	matcher.match(descriptors_object, descriptors_scene,matches);
	
	double maxDist = 0.0;
	double minDist = 100.0;
	
	//Quick caluculation of max and min distances between keypoints
	for(int i = 0; i < descriptors_object.rows; i++){
		double dist  = matches[i].distance;
		minDist = dist < minDist ? dist : minDist;
		maxDist = dist > maxDist ? dist : maxDist;	
	}
	
	cout << "Maximum Distance: " << maxDist << endl;
	cout << "Minimum Distance: " << minDist << endl;
	
	//Use only good matches
	double distFilter = 3.0;
	vector<DMatch> goodMatches;
	for(int i = 0; i < descriptors_object.rows; i++){
		if( matches[i].distance  < minDist * distFilter ){
			goodMatches.push_back(matches[i]);
		}
	}
	
	vector<Point2f> obj;
	vector<Point2f> scn;
	
	for(int i = 0; i < goodMatches.size(); i++){
		obj.push_back(keyPoints_object[goodMatches[i].queryIdx].pt );
		scn.push_back(keyPoints_scene[goodMatches[i].trainIdx].pt );
	}
	
	//Find the homography matrix
	Mat H = findHomography(obj,scn,CV_RANSAC);
	
	Mat result;
	warpPerspective(image1,result,H,Size(image1.cols+image2.cols,image1.rows));
	Mat half(result,Rect(0,0,image2.cols,image2.rows));
	
	image2.copyTo(half);
	imshow("Result",half);
	
	waitKey(0);
    return 0;
}
