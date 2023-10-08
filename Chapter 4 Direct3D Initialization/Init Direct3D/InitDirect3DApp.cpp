//***************************************************************************************
// Init Direct3D.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Demonstrates the sample framework by initializing Direct3D, clearing 
// the screen, and displaying frame stats.
//
//***************************************************************************************

#include "../../Common/d3dApp.h"
#include <DirectXColors.h>

using namespace DirectX;

class InitDirect3DApp : public D3DApp
{
public:
	InitDirect3DApp(HINSTANCE hInstance);
	~InitDirect3DApp();

	virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
	// 디버그 빌드에서는 실행시점 메모리 점검 기능을 켠다.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    try
    {
        InitDirect3DApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance)
: D3DApp(hInstance) 
{
}

InitDirect3DApp::~InitDirect3DApp()
{
}

bool InitDirect3DApp::Initialize()
{
    if(!D3DApp::Initialize())
		return false;
		
	return true;
}

void InitDirect3DApp::OnResize()
{
	D3DApp::OnResize();
}

void InitDirect3DApp::Update(const GameTimer& gt) // 이 추상 메서드는 매 프레임 호출된다. 시간의 흐름에 따른 응용 프로그램 갱신(이를테면 
{												  // 애니메이션 수행, 카메라 이동, 충돌 검출, 사용자 입력 처리 등)을 이 메서드로 구현해야한다.

}

void InitDirect3DApp::Draw(const GameTimer& gt) // 이 추상 메서드도 매 프레임 호출된다. 이 메서드에서는 현재 프레임을 후면 버퍼에 실제로 그리기 위한
{												// 랜더링 명령들을 제출하는 작업을 수행. 프레임을 다 그린 후에는 IDXGISwapChain::Present 메서드를 호출해서
												// 후면 버퍼를 화면에 제시해야한다.
    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
	// 명령 기록에 관련된 메모리의 재활용을 위해 명령 할당자를 재설정한다.
	// 재설정은 GPU가 관련 명령 목록들을 모두 처리한 후에 일어난다.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
	// 명령 목록을 ExecuteCommandList을 통해서 명령 대기열에 추가했다면 명령 목록을 재설정할 수 있다.
	// 명령 목록을 재설정하면 메모리가 재활용된다.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Indicate a state transition on the resource usage.
	// 자원 용도에 관련된 상태 전이를 Direct3D에 통지한다.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
	// 뷰포트와 가위 직사각형을 설정한다. 명령 목록을 재설정할 때마다 이들도 재설정해야 한다.
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Clear the back buffer and depth buffer.
	// 후면 버퍼와 깊이 버퍼를 지운다.
	// 지정된 랜더 대상을 지정된 색으로 지운다.
	// 매개변수
	// - RenderTargetView: 지울 렌더 대상을 서술하는 RTV
	// - ColorRGBA: 렌더 대상을 지우는 데 사용할 색상
	// - NumRects: pRects 배열의 원소 개수. pRects가 널 포인터이면 0을 지정하면 된다.
	// - pRects: 렌더 대상에서 지울 직사각 영역들을 나타내는 D3D12_RECT들의 배열. 랜더 대상 전체를 지우고 싶으면 nullptr를 지정하면 된다.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr); 
	// 지정된 깊이-스텐실 버퍼를 지운다.
	// 책의 예제들은 매번 새로운 이미지로 프레임을 시작하기 위해, 프레임마다 먼저 후면
	// 버퍼 랜더 대상과 깊이-스텐실 버퍼를 지운 후에 그리기 명령들을 실행한다.
	// 매개변수
	// - DepthStencilView: 지울 깊이-스텐실 버퍼를 서술하는 DSV
	// - ClearFlags: 깊이-스텐실 버퍼 중 지우고자 하는 요소(깊이 값 또는 스텐실 값)들을 나타내는 플래그. D3D12_CLEAR_FLAG_DEPTH나
	// D3D12_CLEAR_FLAG_STENCIL또는 그 둘을 비트별 OR(논리합)로 결합한 값을 지정하면 된다.
	// - Depth: 깊이 버퍼를 지우는 데 사용할 깊이 값
	// - Stencil: 스텐실 버퍼를 지우는 데 사용할 스텐실 값
	// - NumRects: pRects 배열의 원소 개수. pRects가 널 포인터이면 0을 지정하면 된다.
	// - pRects: 깊이-스텐실 버퍼에서 지울 직사각 영역들을 나타내는 D3D12_RECT들의 배열, 깊이-스텐실 버퍼 전체를 지우고 싶으면 nullptr를 지정하면 된다.
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
    // Specify the buffers we are going to render to.
	// 랜더링 결과가 기록될 랜더 대상 버퍼들을 지정한다.
	// 매개변수
	// - NumRenderTargetDescriptors: 파이프라인에 묶을 랜더 대상들을 서술하는 RTV들의 배열을 가리키는 포인터.
	// - pRenderTargetDescriptors: 파이프라인에 묶을 랜더 대상들을 서술하는 RTV들의 배열을 가리키는 포인터.
	// - RTsSingleHandleToDescriptorRange: 앞의 배열의 모든 RTV가 서술자 힙 안에 연속적으로 저장되어 있으면 true, 그렇지 않으면 false지정
	// - pDepthStencilDescriptor: 파이프라인에 묶을 깊이-스텐실 버퍼를 서술하는 DSV를 가리키는 포인터
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	
    // Indicate a state transition on the resource usage.
	// 자원 용도에 관련된 상태 전이를 Direct3D에 통지한다.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
	// 명령들의 기록을 마친다.
	ThrowIfFailed(mCommandList->Close());
 
    // Add the command list to the queue for execution.
	// 명령 실행을 위해 명력 목록을 명령 대기열에 추가한다.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	
	// swap the back and front buffers
	// 후면버퍼와 전면버퍼를 교체한다.
	// Present 메서드는 후면버퍼와 전면 버퍼를 교환한다.
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	// 이 프레임의 명령들이 모두 처리되길 기다린다. 이러한 대기는 비효율적이다.
	// 이번에는 예제의 간단함을 위해 이 방법을 사용하지만, 이후의 예제들에서는 랜더링 코드를 적절히
	// 조직화해서 프레임마다 대기할 필요가 없게 만든다.
	FlushCommandQueue();
}

