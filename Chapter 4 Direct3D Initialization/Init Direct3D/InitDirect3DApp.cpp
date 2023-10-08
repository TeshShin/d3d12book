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
	// ����� ���忡���� ������� �޸� ���� ����� �Ҵ�.
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

void InitDirect3DApp::Update(const GameTimer& gt) // �� �߻� �޼���� �� ������ ȣ��ȴ�. �ð��� �帧�� ���� ���� ���α׷� ����(�̸��׸� 
{												  // �ִϸ��̼� ����, ī�޶� �̵�, �浹 ����, ����� �Է� ó�� ��)�� �� �޼���� �����ؾ��Ѵ�.

}

void InitDirect3DApp::Draw(const GameTimer& gt) // �� �߻� �޼��嵵 �� ������ ȣ��ȴ�. �� �޼��忡���� ���� �������� �ĸ� ���ۿ� ������ �׸��� ����
{												// ������ ��ɵ��� �����ϴ� �۾��� ����. �������� �� �׸� �Ŀ��� IDXGISwapChain::Present �޼��带 ȣ���ؼ�
												// �ĸ� ���۸� ȭ�鿡 �����ؾ��Ѵ�.
    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
	// ��� ��Ͽ� ���õ� �޸��� ��Ȱ���� ���� ��� �Ҵ��ڸ� �缳���Ѵ�.
	// �缳���� GPU�� ���� ��� ��ϵ��� ��� ó���� �Ŀ� �Ͼ��.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
	// ��� ����� ExecuteCommandList�� ���ؼ� ��� ��⿭�� �߰��ߴٸ� ��� ����� �缳���� �� �ִ�.
	// ��� ����� �缳���ϸ� �޸𸮰� ��Ȱ��ȴ�.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Indicate a state transition on the resource usage.
	// �ڿ� �뵵�� ���õ� ���� ���̸� Direct3D�� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
	// ����Ʈ�� ���� ���簢���� �����Ѵ�. ��� ����� �缳���� ������ �̵鵵 �缳���ؾ� �Ѵ�.
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Clear the back buffer and depth buffer.
	// �ĸ� ���ۿ� ���� ���۸� �����.
	// ������ ���� ����� ������ ������ �����.
	// �Ű�����
	// - RenderTargetView: ���� ���� ����� �����ϴ� RTV
	// - ColorRGBA: ���� ����� ����� �� ����� ����
	// - NumRects: pRects �迭�� ���� ����. pRects�� �� �������̸� 0�� �����ϸ� �ȴ�.
	// - pRects: ���� ��󿡼� ���� ���簢 �������� ��Ÿ���� D3D12_RECT���� �迭. ���� ��� ��ü�� ����� ������ nullptr�� �����ϸ� �ȴ�.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr); 
	// ������ ����-���ٽ� ���۸� �����.
	// å�� �������� �Ź� ���ο� �̹����� �������� �����ϱ� ����, �����Ӹ��� ���� �ĸ�
	// ���� ���� ���� ����-���ٽ� ���۸� ���� �Ŀ� �׸��� ��ɵ��� �����Ѵ�.
	// �Ű�����
	// - DepthStencilView: ���� ����-���ٽ� ���۸� �����ϴ� DSV
	// - ClearFlags: ����-���ٽ� ���� �� ������� �ϴ� ���(���� �� �Ǵ� ���ٽ� ��)���� ��Ÿ���� �÷���. D3D12_CLEAR_FLAG_DEPTH��
	// D3D12_CLEAR_FLAG_STENCIL�Ǵ� �� ���� ��Ʈ�� OR(����)�� ������ ���� �����ϸ� �ȴ�.
	// - Depth: ���� ���۸� ����� �� ����� ���� ��
	// - Stencil: ���ٽ� ���۸� ����� �� ����� ���ٽ� ��
	// - NumRects: pRects �迭�� ���� ����. pRects�� �� �������̸� 0�� �����ϸ� �ȴ�.
	// - pRects: ����-���ٽ� ���ۿ��� ���� ���簢 �������� ��Ÿ���� D3D12_RECT���� �迭, ����-���ٽ� ���� ��ü�� ����� ������ nullptr�� �����ϸ� �ȴ�.
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
    // Specify the buffers we are going to render to.
	// ������ ����� ��ϵ� ���� ��� ���۵��� �����Ѵ�.
	// �Ű�����
	// - NumRenderTargetDescriptors: ���������ο� ���� ���� ������ �����ϴ� RTV���� �迭�� ����Ű�� ������.
	// - pRenderTargetDescriptors: ���������ο� ���� ���� ������ �����ϴ� RTV���� �迭�� ����Ű�� ������.
	// - RTsSingleHandleToDescriptorRange: ���� �迭�� ��� RTV�� ������ �� �ȿ� ���������� ����Ǿ� ������ true, �׷��� ������ false����
	// - pDepthStencilDescriptor: ���������ο� ���� ����-���ٽ� ���۸� �����ϴ� DSV�� ����Ű�� ������
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	
    // Indicate a state transition on the resource usage.
	// �ڿ� �뵵�� ���õ� ���� ���̸� Direct3D�� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
	// ��ɵ��� ����� ��ģ��.
	ThrowIfFailed(mCommandList->Close());
 
    // Add the command list to the queue for execution.
	// ��� ������ ���� ��� ����� ��� ��⿭�� �߰��Ѵ�.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	
	// swap the back and front buffers
	// �ĸ���ۿ� ������۸� ��ü�Ѵ�.
	// Present �޼���� �ĸ���ۿ� ���� ���۸� ��ȯ�Ѵ�.
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	// �� �������� ��ɵ��� ��� ó���Ǳ� ��ٸ���. �̷��� ���� ��ȿ�����̴�.
	// �̹����� ������ �������� ���� �� ����� ���������, ������ �����鿡���� ������ �ڵ带 ������
	// ����ȭ�ؼ� �����Ӹ��� ����� �ʿ䰡 ���� �����.
	FlushCommandQueue();
}

