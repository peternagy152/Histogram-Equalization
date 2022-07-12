#include <iostream>
#include <math.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <mpi.h>
#include <ctime>// include this header 
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;
#define fixedSize  100;

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//*********************************************************Read Image and save it to local arrayss*************************	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int* Red = new int[BM.Height * BM.Width];
	int* Green = new int[BM.Height * BM.Width];
	int* Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height * BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i * BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return input;
}

void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			//i * OriginalImageWidth + j
			if (image[i * width + j] < 0)
			{
				image[i * width + j] = 0;
			}
			if (image[i * width + j] > 255)
			{
				image[i * width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("E:/HPC_ProjectTemplate/Data/OutPut/outputrez" + index + ".png");
	cout << "result Image Saved " << index << endl;
}
int* split(int x, int n)
{


	static int splits[50];
	if (x % n == 0)
	{
		for (int i = 0;i < n;i++)
			splits[i] = (x / n);
	}
	else
	{

		int zp = n - (x % n);
		int pp = x / n;
		for (int i = 0;i < n;i++)
		{

			if (i >= zp)
				splits[i] = pp + 1;
			else
				splits[i] = pp;
		}
	}
	return splits;
}


int main()
{
	int ImageWidth = 4, ImageHeight = 4;
	int width = 4, height = 4;

	int start_s, stop_s, TotalTime = 0;

	System::String^ imagePath;
	std::string img;
	img = "E:/HPC_ProjectTemplate/Data/Input/2019_00003.png";

	imagePath = marshal_as<System::String^>(img);
	int* imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);
	int* modified = inputImage(&ImageWidth, &ImageHeight, imagePath);


	start_s = clock();
	MPI_Init(NULL, NULL);
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	// Code Start Here -------------------------------------

	int data[16] = { 3,2,4,5,7,7,8,2,3,1,2,3,5,4,6,7 };
	int localhistogram[256] = { 0 };
	int globalhistogram[256];
	float globalprob[256];
	float cumprob[256];
	int globalfloor[256];

	// Start here ============================

	//Get Correct Splits of Image Dims 



		 //int* splits;
		 //splits = split(ImageHeight * ImageWidth, size);
		 //int* displacement = new int[size];
		 //for (int i = 0; i < size; i++) // { 0 ,488 ,977 ,   }
		 //{
			// cout << "Split size is : " << splits[i] << endl;
			// if (i == 0)
			//	 displacement[i] = 0;
			// else if (i == 1)
			// {
			//	 displacement[i] = splits[i] - 1;
			// }
			// else
			// {
			//	 displacement[i] = displacement[i-1] + splits[i];
			// }
		 //}
		 //



	int imgSubdataLenght = (ImageHeight * ImageWidth) / size;  // Edit Here height and width 

	int* imgsubdata = new int[imgSubdataLenght];


	MPI_Scatter(imageData, imgSubdataLenght, MPI_INT, imgsubdata, imgSubdataLenght, MPI_INT, 0, MPI_COMM_WORLD);
	for (int i = 0; i < imgSubdataLenght;i++)
	{
		localhistogram[imgsubdata[i]] += 1;
	}
	MPI_Reduce(&localhistogram, &globalhistogram, 256, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

	MPI_Bcast(&globalhistogram, 256, MPI_INT, 0, MPI_COMM_WORLD);


	// Split GLobalHistogram into Normal  
	int histogramSplits = 256 / size;

	int* NormalGlobalHistogram = new int[histogramSplits * size];
	for (int i = 0; i < histogramSplits * size;i++)
		NormalGlobalHistogram[i] = globalhistogram[i];

	int DiscardedSize = 256 - (histogramSplits * size);



	if (size % 2 == 0)
	{
		int* localpixel = new int[histogramSplits];
		float* localprob = new float[histogramSplits];

		MPI_Scatter(&globalhistogram, histogramSplits, MPI_INT, localpixel, histogramSplits, MPI_INT, 0, MPI_COMM_WORLD);
		for (int i = 0; i < histogramSplits; i++)
			localprob[i] = (float)localpixel[i] / (ImageHeight * ImageWidth);

		MPI_Gather(localprob, histogramSplits, MPI_FLOAT, &globalprob, histogramSplits, MPI_FLOAT, 0, MPI_COMM_WORLD);
	}
	else
	{

		// ---------------------------- Normal Splits --------------------------------- 
		int* localpixel = new int[histogramSplits];
		float* localprob = new float[histogramSplits];


		MPI_Scatter(NormalGlobalHistogram, histogramSplits, MPI_INT, localpixel, histogramSplits, MPI_INT, 0, MPI_COMM_WORLD);
		for (int i = 0; i < histogramSplits; i++)
			localprob[i] = (float)localpixel[i] / (ImageHeight * ImageWidth);

		MPI_Gather(localprob, histogramSplits, MPI_FLOAT, &globalprob, histogramSplits, MPI_FLOAT, 0, MPI_COMM_WORLD);
		MPI_Bcast(&globalprob, 256, MPI_INT, 0, MPI_COMM_WORLD);

	}


	//calculate Cummulative Probability  And Handeling Odd Size issue 
	if (rank == 0)
	{
		if (size % 2 != 0)
		{
			for (int i = histogramSplits * size; i < 256; i++)
				globalprob[i] = (float)globalhistogram[i] / (ImageHeight * ImageWidth);
		}

		cout << "Size : " << size << endl;
		cout << "H : " << ImageHeight << "  W :  " << ImageWidth << endl;
		for (int i = 0; i < 256; i++)
		{
			if (i == 0)
				cumprob[i] = globalprob[i];
			else
				cumprob[i] = globalprob[i] + cumprob[i - 1];
		}


	}
	MPI_Bcast(&cumprob, 256, MPI_INT, 0, MPI_COMM_WORLD);

	// ----------------------------- Multiply CumProb by 255 and Get Floor -----------------------  
	if (size % 2 == 0)
	{
		// Multiply CumProb by 20 and Get Floor 
		float* localcumprob = new float[histogramSplits];
		float* localcumprob20 = new float[histogramSplits];
		int* floor = new int[histogramSplits];
		MPI_Scatter(&cumprob, histogramSplits, MPI_INT, localcumprob, histogramSplits, MPI_INT, 0, MPI_COMM_WORLD);
		for (int i = 0; i < histogramSplits; i++)
		{

			localcumprob[i] = localcumprob[i] * 255;
			floor[i] = localcumprob[i];
			//cout << localprob[i] << endl;
		}
		MPI_Gather(floor, histogramSplits, MPI_INT, &globalfloor, histogramSplits, MPI_INT, 0, MPI_COMM_WORLD);
	}
	else
	{
		// ---------------------------- Normal Splits --------------------------------- 
		float* localcumprob = new float[histogramSplits];
		float* localcumprob20 = new float[histogramSplits];
		int* floor = new int[histogramSplits];
		MPI_Scatter(&cumprob, histogramSplits, MPI_INT, localcumprob, histogramSplits, MPI_INT, 0, MPI_COMM_WORLD);
		for (int i = 0; i < histogramSplits; i++)
		{

			localcumprob[i] = localcumprob[i] * 255;
			floor[i] = localcumprob[i];
			//cout << localprob[i] << endl;
		}
		MPI_Gather(floor, histogramSplits, MPI_INT, &globalfloor, histogramSplits, MPI_INT, 0, MPI_COMM_WORLD);

	}

	// ----------------- --- ---  Handeling The Odd issue --- ---  --------------------------- 
	MPI_Bcast(&globalfloor, 256, MPI_INT, 0, MPI_COMM_WORLD);
	if (rank == 0)
	{
		if (size % 2 != 0)
		{
			for (int i = histogramSplits * size; i < 256; i++)
			{
				cumprob[i] = cumprob[i] * 255;
				globalfloor[i] = cumprob[i];
			}
		}
	}


	MPI_Bcast(&globalfloor, 256, MPI_INT, 0, MPI_COMM_WORLD);






	// ----------------------------------- Finial Array Of Image ----------------------------
	int* localresult = new int[imgSubdataLenght];
	MPI_Scatter(imageData, imgSubdataLenght, MPI_INT, localresult, imgSubdataLenght, MPI_INT, 0, MPI_COMM_WORLD);
	for (int i = 0; i < imgSubdataLenght;i++)
	{
		localresult[i] = globalfloor[localresult[i]];

	}
	MPI_Gather(localresult, imgSubdataLenght, MPI_INT, modified, imgSubdataLenght, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Bcast(modified, ImageHeight * ImageWidth, MPI_INT, 0, MPI_COMM_WORLD);
	if (rank == 0)
	{
		for (int i = 0; i < 20; i++)
			cout << modified[i] << endl;
	}

	MPI_Finalize();


	stop_s = clock();


	TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
	createImage(modified, ImageWidth, ImageHeight, 1);
	cout << "time: " << TotalTime << endl;

	free(imageData);
	return 0;

}



