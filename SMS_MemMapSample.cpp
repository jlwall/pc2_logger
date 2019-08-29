// Used for memory-mapped functionality
#include <windows.h>
#include "sharedmemory.h"

// Used for this example
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime> 
#include <string>

using namespace std;

// Name of the pCars memory mapped file
#define MAP_OBJECT_NAME "$pcars2$"

int main()
{
	std::ofstream loggout;

	// Open the memory-mapped file
	HANDLE fileHandle = OpenFileMapping( PAGE_READONLY, FALSE, MAP_OBJECT_NAME );
	if (fileHandle == NULL)
	{
		printf( "Could not open file mapping object (%d).\n", GetLastError() );
		return 1;
	}

	// Get the data structure
	const SharedMemory* sharedData = (SharedMemory*)MapViewOfFile( fileHandle, PAGE_READONLY, 0, 0, sizeof(SharedMemory) );
	SharedMemory* localCopy = new SharedMemory;
	if (sharedData == NULL)
	{
		printf( "Could not map view of file (%d).\n", GetLastError() );
		CloseHandle( fileHandle );
		return 1;
	}

	// Ensure we're sync'd to the correct data version
	if ( sharedData->mVersion != SHARED_MEMORY_VERSION )
	{
		printf( "Data version mismatch\n");
		return 1;
	}

	//------------------------------------------------------------------------------
	// TEST DISPLAY CODE
	//------------------------------------------------------------------------------
	unsigned int updateIndex(0);
	unsigned int indexChange(0);
	bool bLoggStarted = false;
	LARGE_INTEGER frequency;        // ticks per second
	LARGE_INTEGER t1, t2;           // ticks
	double elapsedTime;

	printf( "ESC TO EXIT\n\n" );
	while (true)
	{
		if ( sharedData->mSequenceNumber % 2 )
		{
			// Odd sequence number indicates, that write into the shared memory is just happening
			continue;
		}

		indexChange = sharedData->mSequenceNumber - updateIndex;
		updateIndex = sharedData->mSequenceNumber;

		//Copy the whole structure before processing it, otherwise the risk of the game writing into it during processing is too high.
		memcpy(localCopy,sharedData,sizeof(SharedMemory));
		if (localCopy->mSequenceNumber != updateIndex )
		{
			// More writes had happened during the read. Should be rare, but can happen.
			continue;
		}

		if (bLoggStarted == 0 && localCopy->mGameState >= 2)
		{
			// get ticks per second
			QueryPerformanceFrequency(&frequency);
			QueryPerformanceCounter(&t1);

			std::time_t t = std::time(0);   // get time now
			std::tm* now = std::localtime(&t);
			string ds = std::to_string(now->tm_year+1900) +"-"+ std::to_string(now->tm_mon) + "-" + std::to_string(now->tm_mday) + "-" + std::to_string(now->tm_hour) + "-" + std::to_string(now->tm_min);

			string fn = "log_" + ds + "_"+string(localCopy->mTrackLocation) + "_" + string(localCopy->mCarName) + ".csv";
			bLoggStarted = true;
			loggout.open("C:\\temp\\" + fn);
			loggout << "Time,nSequence,LapTime,vCar,nEngine,aPedal,aBrake,aSteering,nFuelLevel,vCarY,vVarZ" << endl;
		}
		
		if (localCopy->mGameState == 2 && indexChange >=2 )
		{
			QueryPerformanceCounter(&t2);

			// compute and print the elapsed time in millisec
			elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
			loggout << elapsedTime/1000 << ",";
			loggout << localCopy->mSequenceNumber << ",";
			loggout << localCopy->mCurrentTime << ",";
			loggout << localCopy->mSpeed << ",";
			loggout << localCopy->mEngineSpeed << ",";
			loggout << localCopy->mThrottle << ",";
			loggout << localCopy->mBrake << ",";
			loggout << localCopy->mSteering << ",";
			loggout << localCopy->mFuelLevel << ",";
			loggout << localCopy->mLocalVelocity[0] << ",";
			loggout << localCopy->mLocalVelocity[1] << ",";
			loggout << localCopy->mLocalVelocity[2] << ",";
			loggout << localCopy->mLocalAcceleration[0] << ",";
			loggout << localCopy->mLocalAcceleration[1] << ",";
			loggout << localCopy->mLocalAcceleration[2] << ",";
			loggout << localCopy->mOrientation[0] << ",";
			loggout << localCopy->mOrientation[1] << ",";
			loggout << localCopy->mOrientation[2] << ",";
			loggout << localCopy->mWheelLocalPositionY[0] << ",";
			loggout << localCopy->mWheelLocalPositionY[1] << ",";
			loggout << localCopy->mWheelLocalPositionY[2] << ",";
			loggout << localCopy->mWheelLocalPositionY[3] << ",";
			loggout << localCopy->mGear << "," << endl;		
		}
	
		if ( _kbhit() && _getch() == 27 ) // check for escape
		{
			loggout.close();
			break;
		}
	}
	//------------------------------------------------------------------------------

	loggout.close();

	// Cleanup
	UnmapViewOfFile( sharedData );
	CloseHandle( fileHandle );
	delete localCopy;

	return 0;
}
