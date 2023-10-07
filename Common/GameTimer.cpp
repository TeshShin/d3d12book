//***************************************************************************************
// GameTimer.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include <windows.h>
#include "GameTimer.h"

GameTimer::GameTimer()
: mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), 
  mPausedTime(0), mPrevTime(0), mCurrTime(0), mStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

// Returns the total time elapsed since Reset() was called, NOT counting any
// time when the clock is stopped.
float GameTimer::TotalTime()const
{
	// Ÿ�̸Ӱ� ���� �����̸�, ������ �������� �帥 �ð��� ������� ���ƾ� �Ѵ�.
	// ����, ������ �̹� �Ͻ� ������ ���� �ִٸ� �ð��� mStopTime - mBaseTime����
	// �Ͻ� ���� ���� �ð��� ���ԵǾ� �ִµ�, �� ���� �ð��� ��ü �ð��� �������� ���ƾ��Ѵ�.
	// �̸� �ٷ� ��� ����, mStopTime���� �Ͻ� ���� ���� �ð��� ����:  
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

	if( mStopped )
	{
		return (float)(((mStopTime - mPausedTime)-mBaseTime)*mSecondsPerCount);
	}

	// �ð��� mCurrTime - mBaseTime���� �Ͻ� ���� ���� �ð��� ���ԵǾ� �ִ�.
	// �̸� ��ü �ð��� �����ϸ� �ȵǹǷ�, �� �ð��� mCurrTime���� ����.
	//  
	//
	//  (mCurrTime - mPausedTime) - mBaseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mCurrTime
	
	else
	{
		return (float)(((mCurrTime-mPausedTime)-mBaseTime)*mSecondsPerCount);
	}
}

float GameTimer::DeltaTime()const
{
	return (float)mDeltaTime;
}

void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped  = false;
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);


	// Accumulate the time elapsed between stop and start pairs.
	// ����(�Ͻ� ����)�� ����(�簳) ���̿� �帥 �ð��� �����Ѵ�.
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	// ���� ���¿��� Ÿ�̸Ӹ� �簳�ϴ� ���
	if( mStopped )
	{
		// �Ͻ� ������ �ð��� �����Ѵ�.
		mPausedTime += (startTime - mStopTime);	
		// Ÿ�̸Ӹ� �ٽ� �����ϴ� ���̹Ƿ�, ������ mPrevTime(���� �ð�)��
		// ��ȿ���� �ʴ�(�Ͻ� ���� ���߿� ���ŵǾ��� ���̹Ƿ�).
		// ���� ���� �ð����� �ٽ� �����Ѵ�.
		mPrevTime = startTime;

		// ������ ���� ���°� �ƴϹǷ� ���� ������� �����Ѵ�.
		mStopTime = 0;
		mStopped  = false;
	}
}

void GameTimer::Stop()
{
	// �̹� ���� ���¸� �ƹ� �ϵ� ���� �ʴ´�.
	if( !mStopped )
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
		// �׷��� �ʴٸ� ���� �ð��� Ÿ�̸� ���� ���� �ð����� �����ϰ�,
		// Ÿ�̸Ӱ� �����Ǿ����� ���ϴ� �ο� �÷��׸� �����Ѵ�.
		mStopTime = currTime;
		mStopped  = true;
	}
}

void GameTimer::Tick()
{
	if( mStopped )
	{
		mDeltaTime = 0.0;
		return;
	}

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	// Time difference between this frame and the previous.
	mDeltaTime = (mCurrTime - mPrevTime)*mSecondsPerCount;

	// Prepare for next frame.
	mPrevTime = mCurrTime;

	// ������ ���� �ʰ� �Ѵ�.  The DXSDK's CDXUTTimer mentions that if the 
	// processor goes into a power save mode or we get shuffled to another
	// processor, then mDeltaTime can be negative.
	if(mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}

