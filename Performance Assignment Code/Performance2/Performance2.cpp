// Performance2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Performance2.h"

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Timer - used to established precise timings for code.
class TIMER
{
	LARGE_INTEGER t_;

	__int64 current_time_;

public:
	TIMER()	// Default constructor. Initialises this timer with the current value of the hi-res CPU timer.
	{
		QueryPerformanceCounter(&t_);
		current_time_ = t_.QuadPart;
	}

	TIMER(const TIMER &ct)	// Copy constructor.
	{
		current_time_ = ct.current_time_;
	}

	TIMER& operator=(const TIMER &ct)	// Copy assignment.
	{
		current_time_ = ct.current_time_;
		return *this;
	}

	TIMER& operator=(const __int64 &n)	// Overloaded copy assignment.
	{
		current_time_ = n;
		return *this;
	}

	~TIMER() {}		// Destructor.

	static __int64 get_frequency()
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		return frequency.QuadPart;
	}

	__int64 get_time() const
	{
		return current_time_;
	}

	void get_current_time()
	{
		QueryPerformanceCounter(&t_);
		current_time_ = t_.QuadPart;
	}

	inline bool operator==(const TIMER &ct) const
	{
		return current_time_ == ct.current_time_;
	}

	inline bool operator!=(const TIMER &ct) const
	{
		return current_time_ != ct.current_time_;
	}

	__int64 operator-(const TIMER &ct) const		// Subtract a TIMER from this one - return the result.
	{
		return current_time_ - ct.current_time_;
	}

	inline bool operator>(const TIMER &ct) const
	{
		return current_time_ > ct.current_time_;
	}

	inline bool operator<(const TIMER &ct) const
	{
		return current_time_ < ct.current_time_;
	}

	inline bool operator<=(const TIMER &ct) const
	{
		return current_time_ <= ct.current_time_;
	}

	inline bool operator>=(const TIMER &ct) const
	{
		return current_time_ >= ct.current_time_;
	}
};

CWinApp theApp;  // The one and only application object

using namespace std;

CImage* Copy(CImage *source)
{
	CImage *dest = new CImage;
	dest->Create(source->GetWidth(), source->GetHeight(), source->GetBPP());
	source->Draw(dest->GetDC(), 0, 0, dest->GetWidth(), dest->GetHeight(), 0, 0, source->GetWidth(), source->GetHeight());
	dest->ReleaseDC();
	return dest;
}

void Brighten(CImage *img)
{
	//Getting the width and height of the image using CImages native functions
	int width = img->GetWidth();
	int height = img->GetHeight();
	//Declaring 3 integers that will store the red green blue values of the chosen pixel
	int R = 0, G = 0, B = 0;

	//We're going to work directly on the image as the image does not change in size or orientation in this function.
	//the bptr is a pointer to the first byte of the image
	BYTE* bptr = (BYTE*)img->GetBits();
	//returns the pitch of the image. the pitch indicates whether the BMP data starts from the bottom left or top left of the image
	int pitch = img->GetPitch();

	//we now loop through every pixel on the image. we start with x = 0, and then loop from y = 0, meaning the image will go down in columns of the image rather than across the rows
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			//This calculation is performed to avoid using the getPixel method of a CImage.
			//The algorithm returns a pointer to the pixel colour of choice. 
			//the first part of the algorithm, bptr, is the starting point location of the image. 
			//the next bit is the 'pitch * the y row', where y is number the loop is currently on. remember, pitch can be negative and therefore this could produce a negative result.
			//then, (3 * x) is added to the current value. 3*x is just finding the x location value of the image. 
			//the final bit of the algorithm, the +0, +1 and +2 specifies whether the pointer points to the red, green or blue location
			R = *(bptr + pitch*y + 3 * x);
			G = *(bptr + pitch*y + 3 * x + 1);
			B = *(bptr + pitch*y + 3 * x + 2);

			//here, we are performing a check to see if the rgb values are less than 255. this is because 255 is the maximum value a colour value can hold (https://en.wikipedia.org/wiki/RGB_color_model). going over this value would cause problems
			//after checking that the current r g and b values wont go over 255, we add 10 to them. this increases the intensity of the colour, increasing the brightness of the colours
			//if we kept increasing the brightness, we would eventually reach plain white, which is RGB (255,255,255)
			if ((R + 10) > 255) R = 255; else R += 10;
			if ((G + 10) > 255) G = 255; else G += 10;
			if ((B + 10) > 255) B = 255; else B += 10;

			//now, just like how we found each pixel's colour, we set the colours of the pixel by directly by using their pointers. the R G and B may or may not have been modified.
			*(bptr + pitch*y + 3 * x) = R;
			*(bptr + pitch*y + 3 * x + 1) = G;
			*(bptr + pitch*y + 3 * x + 2) = B;
		}
	}
	//As we did not require the image to change size or scale, we do not need to return anything as we have directly modified the image using its pointer
}

CImage* Rotate(CImage *img)
{
	//again, initialising the rgb values and getting the dimensions of the image
	int R = 0, G = 0, B = 0;
	int width = img->GetWidth();
	int height = img->GetHeight();

	//this time we will need to change the orientation and dimensions of the image so we have to crate a new image object to modify, rather than modifying the original image
	CImage *res = new CImage;
	res->Create(height, width, img->GetBPP());

	//same as brighten function, getting starting pointers and image info for the image we have just created which will be drawn onto.
	BYTE* bptr = (BYTE*)res->GetBits();
	int pitch = res->GetPitch();

	//get the pointer info for the original image, from which we will be using to pull rgb values from, however we will not be modifying values using these pointers
	BYTE* iptr = (BYTE*)img->GetBits();
	int iPitch = img->GetPitch();

	//here we initialise X and Y values for the resulting image. these location values will keep track of what location we will be applying pixel colours to, as the x and y will not be the same as the x and y of the source image
	//resX is initialised as the height -1, because the image will be rotated making the height the width, and the width the height
	int resX = height - 1;
	int resY = 0;

	//again, these 2 loops are for looping through each images pixel	
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			//like before, we use the bptr and pitch to identify the memory locations of pixel colour values.
			//note that we are setting the pixel values using resX and resY, and getting the values using x and y.
			//this is because we loop through each of the ORIGINAL images pixels, and then apply them to the NEW image's relational pixels. X and Y and resX and resY will look very different looping through this code,
			//as the original image is being scanned VERTICALLY, whereas the new image is being drawn HORIZONTALLY
			*(bptr + pitch*resY + 3 * resX) = *(iptr + iPitch*y + 3 * x);
			*(bptr + pitch*resY + 3 * resX + 1) = *(iptr + iPitch*y + 3 * x + 1);
			*(bptr + pitch*resY + 3 * resX + 2) = *(iptr + iPitch*y + 3 * x + 2);
			//As resX started as the image height - 1, we take 1 away to move 1 pixel right to left on the resulting image
			resX--;
		}
		//This is 'y less than width' because the image is now rotated, so height and width are flipped. although it seems strange its correct :)
		if (resY < width - 1)
			resY++;
		//everytime we reach this stage, this means that the y loop has reached its max value. so, we reset the resX as the x is now the y on the new image. resX is reset by setting it equal to height - 1
		resX = height - 1;
	}
	//now we return the image (well, pointer to the image) so that the other image operations can continue
	return res;
}

void Greyscale(CImage *img)
{
	//declaring rgb values and initialising
	int R = 0, G = 0, B = 0;

	//finding starting pointer and pitch of image
	BYTE* bptr = (BYTE*)img->GetBits();
	int pitch = img->GetPitch();

	//finding width and height of the image, as this will be used to loop through each pixel
	int width = img->GetWidth();
	int height = img->GetHeight();

	//2 loops to loop through every pixel
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			//now to greyscale, i use a method used by GIMP (popular free image editor) to perform the 'average' greyscale algorithm, found at: http://docs.gimp.org/2.6/en/gimp-tool-desaturate.html
			//this is where you take the rgb values of a pixel, add them all together and divide by 3. you then apply this value to the R G and B value of the pixel, and it produces a grey colour.

			//below, we get the r g b values of the pixel on the image (x,y)
			R = *(bptr + pitch*y + 3 * x);
			G = *(bptr + pitch*y + 3 * x + 1);
			B = *(bptr + pitch*y + 3 * x + 2);

			//add them all together and divide by 3 (the number will NEVER be over 255 so no need to check)
			BYTE gs = (R + G + B) / 3;

			//then put the values back all as one value, the value produced from the above algorithm.
			*(bptr + pitch*y + 3 * x) = gs;
			*(bptr + pitch*y + 3 * x + 1) = gs;
			*(bptr + pitch*y + 3 * x + 2) = gs;
		}
	}
}

COLORREF getPixelInterpolated(BYTE *iptr, int ipitch, int xPos, int yPos, int width, int height)
{
	//first, we create a counter that checks how many pixels have been included in the average colour alrogithm.
	int pixelCounter = 0;
	//initalise pixel rgb values to 0
	int R = 0, G = 0, B = 0;

	//before we get the colour info of the surrounding pixels, we check to ensure that we are not at the edges of the image, which will result in a code break as the function will try to access a memory locations that may not exist.
	//if successful, the r g and b values are added to the original rgb values we initalised. this will be used to average out the colours at the end

	//we find the surrounding pixels by adding and taking away '1' from the x and y coordinates of the original pixel. this can be seen in all of the code below, eg. (yPos - 1) and (xPos + 1)

	//first, we get the pixel ABOVE by one, only if it exists (checks to make sure pointer doesnt point out of boundary)
	if (yPos > 0)
	{
		//rgb values added to count
		R += *(iptr + ipitch*(yPos - 1) + 3 * xPos);
		G += *(iptr + ipitch*(yPos - 1) + 3 * xPos + 1);
		B += *(iptr + ipitch*(yPos - 1) + 3 * xPos + 2);
		pixelCounter++;
	}

	//Pixel 2 - the pixel BELOW the original pixel
	if (yPos < height - 1)
	{
		//rgb values added to count
		R += *(iptr + ipitch*(yPos + 1) + 3 * xPos);
		G += *(iptr + ipitch*(yPos + 1) + 3 * xPos + 1);
		B += *(iptr + ipitch*(yPos + 1) + 3 * xPos + 2);
		pixelCounter++;
	}

	//Pixel 3 - pixel to the LEFT of the original pixel
	if (xPos > 0)
	{
		//rgb values added to count
		R += *(iptr + ipitch*(yPos)+3 * (xPos - 1));
		G += *(iptr + ipitch*(yPos)+3 * (xPos - 1) + 1);
		B += *(iptr + ipitch*(yPos)+3 * (xPos - 1) + 2);
		pixelCounter++;
	}

	//Pixel 4 - pixel to the RIGHT of the orignal pixel
	if (xPos < width - 1)
	{
		//rgb values added to count
		R += *(iptr + ipitch*(yPos)+3 * (xPos + 1));
		G += *(iptr + ipitch*(yPos)+3 * (xPos + 1) + 1);
		B += *(iptr + ipitch*(yPos)+3 * (xPos + 1) + 2);
		pixelCounter++;
	}

	//now we get an average value from all the pixel info we collected in the code above. this average is calculated by dividing the resulting colour values by the number of times we found a pixel and added the colour values
	R = R / pixelCounter;
	G = G / pixelCounter;
	B = B / pixelCounter;

	//now we return a variable in the form of a COLORREF by creating a pixel using the RGB(R,G,B) function
	return (RGB(R, G, B));
}

CImage* Scale(CImage *img)
{
	//first we get the width and height of the image
	int width = img->GetWidth();
	int height = img->GetHeight();

	//then, we create a new image. we can not modify directly to the image sent as the resulting image from this function will have different dimensions.
	CImage *res = new CImage;
	//We create the image using CImage's native create function. the width and height of the image are set to HALF that of which the original image is. this is because the bilinear filtering algorithm will scale down the image for us by half
	//This line of code creates a new blank image thta is half the size of the original image, and will be drawn to.
	res->Create(width / 2, height / 2, img->GetBPP());

	//now we get the ptr location of the beginning of the new image we will be modifying, and the pitch of the image too.
	BYTE* bptr = (BYTE*)res->GetBits();
	int pitch = res->GetPitch();

	//we also get the pointer location of the image we will be extracting rgb values from, as well as the pitch of that image.
	BYTE* iptr = (BYTE*)img->GetBits();
	int ipitch = img->GetPitch();

	//resX and resY were used to keep track of the x and y pixel location of the images we will be editing. the x and y of the original image will not be the same as the x and y of the resulting image because it will be scaled by half.
	int resX = 0,
		resY = 0;

	//we again are looping through the image, however this time we are only interested in every other pixel of the original image as this is how w will scale down by half
	//to do this, x and y is incremented by 2 every time in the loop to 'skip' over pixels that were not going to be transferred to the new image anyway. this reduces the size by half.
	for (int x = 0; x < width; x += 2)
	{
		for (int y = 0; y < height; y += 2)
		{
			//we create a new pixel called pixelInfo that will store the info for the image. 
			//getPixelInterpolated is a function i created to get the average colour of the 4 colours surrounding the pixel and apply it to the resulting images pixel location
			/*
			Here, X represents pixels we wont use, O represents the pixels we take colour info from and P represents the original pixel
			XXXXXXXX
			XXXXOXXX
			XXXOPOXX
			XXXXOXXX
			XXXXXXXX
			*/
			COLORREF pixelInfo = getPixelInterpolated(iptr, ipitch, x, y, width, height);

			//now we set each RGB value into the new image using resX and resY to denote the location of the pixel, using the 'GetXValue' function of the COLORREF var
			*(bptr + pitch*resY + 3 * resX) = GetRValue(pixelInfo);
			*(bptr + pitch*resY + 3 * resX + 1) = GetGValue(pixelInfo);
			*(bptr + pitch*resY + 3 * resX + 2) = GetBValue(pixelInfo);

			//before we increment the Y pixel locationn of the resulting image we check to ensure it remains within the images bounds
			if (resY < (height / 2) - 1)
				resY++;
		}
		//before we increment the X pixel locationn of the resulting image we check to ensure it remains within the images bounds
		if (resX < width / 2)
			resX++;
		//we also reset the y pixel location of the resulting image to begin drawing again from the top of the image
		resY = 0;
	}
	//and returning the resulting new image because it has changed sizes.
	return res;
}

//This function processes only 1 image at a time. it is called multiple times whilst multithreading. it takes 2 arguments, 1 is a constant pointer to a string which is used to load the image, and another constant pointer to string used to save the image
void imageFunctions(LPCTSTR image, LPCTSTR dest)
{
	//firstly we create a new image and load the image in according to the pointer to string sent in the arguments of this function
	CImage img;
	img.Load(image);
	//then a new img is created which will be the image all the changes are applied to
	CImage *res = new CImage;
	//we scale the image first because this reduces the size of the image by half. this is done first because all the other functions will only have half the amount of work to do after this
	res = Scale(&img);
	//then rotate the image. again, res is set to whatever the rotate function returns because the image dimensions actually change 
	res = Rotate(res);
	//now the next 2 functions do not require the image to be set to what they return as they modify the image directly. this is possible because the dimensions and orientation of the image will not change in these images
	Brighten(res);
	Greyscale(res);
	//finally we save the image ot whatever location is specified, this info is sent in the arguments of this function
	res->Save(dest);
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize Microsoft Foundation Classes, and print an error if failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
	}
	else
	{
		// Application starts here...

		// Time the application's execution time.
		TIMER start;	// DO NOT CHANGE THIS LINE

		//--------------------------------------------------------------------------------------
		// Process the images...   // Put your code here...

		//Declaring the long constant pointers to strings, these are necessary to specify what the images will be saved as, and what the images will be loaded as.
		//they are hard coded because from testing I found that dynamically generating these variables took longer on average than just hard coding them
		LPCTSTR img1U = L"IMG_1-U.PNG";
		LPCTSTR img2U = L"IMG_2-U.PNG";
		LPCTSTR img3U = L"IMG_3-U.PNG";
		LPCTSTR img4U = L"IMG_4-U.PNG";
		LPCTSTR img5U = L"IMG_5-U.PNG";
		LPCTSTR img6U = L"IMG_6-U.PNG";
		LPCTSTR img7U = L"IMG_7-U.PNG";
		LPCTSTR img8U = L"IMG_8-U.PNG";
		LPCTSTR img9U = L"IMG_9-U.PNG";
		LPCTSTR img10U = L"IMG_10-U.PNG";
		LPCTSTR img11U = L"IMG_11-U.PNG";
		LPCTSTR img12U = L"IMG_12-U.PNG";

		LPCTSTR img1 = L"IMG_1.JPG";
		LPCTSTR img2 = L"IMG_2.JPG";
		LPCTSTR img3 = L"IMG_3.JPG";
		LPCTSTR img4 = L"IMG_4.JPG";
		LPCTSTR img5 = L"IMG_5.JPG";
		LPCTSTR img6 = L"IMG_6.JPG";
		LPCTSTR img7 = L"IMG_7.JPG";
		LPCTSTR img8 = L"IMG_8.JPG";
		LPCTSTR img9 = L"IMG_9.JPG";
		LPCTSTR img10 = L"IMG_10.JPG";
		LPCTSTR img11 = L"IMG_11.JPG";
		LPCTSTR img12 = L"IMG_12.JPG";


		//here we begin each of the 12 threads that manipulate each image. these threads perform asynchronously
		thread thread1(imageFunctions, img1, img1U);
		thread thread2(imageFunctions, img2, img2U);
		thread thread3(imageFunctions, img3, img3U);
		thread thread4(imageFunctions, img4, img4U);
		thread thread5(imageFunctions, img5, img5U);
		thread thread6(imageFunctions, img6, img6U);
		thread thread7(imageFunctions, img7, img7U);
		thread thread8(imageFunctions, img8, img8U);
		thread thread9(imageFunctions, img9, img9U);
		thread thread10(imageFunctions, img10, img10U);
		thread thread11(imageFunctions, img11, img11U);
		thread thread12(imageFunctions, img12, img12U);

		//and here we wait for all 12 threads to complete, using the threads native 'join' function
		thread1.join();
		thread2.join();
		thread3.join();
		thread4.join();
		thread5.join();
		thread6.join();
		thread7.join();
		thread8.join();
		thread9.join();
		thread10.join();
		thread11.join();
		thread12.join();

		//and thats the end! thanks for reading, hope it was fast :)

		//-------------------------------------------------------------------------------------------------------

		// How long did it take?...   DO NOT CHANGE FROM HERE...

		TIMER end;

		TIMER elapsed;

		elapsed = end - start;

		__int64 ticks_per_second = start.get_frequency();

		// Display the resulting time...

		double elapsed_seconds = (double)elapsed.get_time() / (double)ticks_per_second;

		cout << "Elapsed time (seconds): " << elapsed_seconds;
		cout << endl;
		cout << "Press a key to continue" << endl;

		char c;
		cin >> c;
	}

	return nRetCode;
}
