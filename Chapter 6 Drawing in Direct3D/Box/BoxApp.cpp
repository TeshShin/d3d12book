//***************************************************************************************
// BoxApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Shows how to draw a box in Direct3D 12.
//
// Controls:
//   Hold the left mouse button down and move the mouse to rotate.
//   Hold the right mouse button down and move the mouse to zoom in and out.
//***************************************************************************************

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};
// 물체당 상수 자료
struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

class BoxApp : public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
    BoxApp(const BoxApp& rhs) = delete;
    BoxApp& operator=(const BoxApp& rhs) = delete;
	~BoxApp();

	virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void BuildDescriptorHeaps();
	void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPSO();

private:
    
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;

    ComPtr<ID3DBlob> mvsByteCode = nullptr;
    ComPtr<ID3DBlob> mpsByteCode = nullptr;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mTheta = 1.5f*XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 5.0f;

    POINT mLastMousePos;
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
        BoxApp theApp(hInstance);
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

BoxApp::BoxApp(HINSTANCE hInstance)
: D3DApp(hInstance) 
{
}

BoxApp::~BoxApp()
{
}

bool BoxApp::Initialize()
{
    if(!D3DApp::Initialize())
		return false;
		
    // Reset the command list to prep for initialization commands.
    // 초기화 명령들을 준비하기 위해 명령 목록을 재설정한다.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
 
    BuildDescriptorHeaps();
	BuildConstantBuffers();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildBoxGeometry();
    BuildPSO();

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    // 초기화가 완료될 때까지 기다린다.
    FlushCommandQueue();

	return true;
}

void BoxApp::OnResize()
{
	D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    // 창의 크기가 바뀌었으므로 종횡비를 갱신하고 투영 행렬을 다시 계산한다.
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f); // 수직 시야각 45도, 가까운평면은 z = 1, 먼평면은 z = 1000
    XMStoreFloat4x4(&mProj, P);
}

void BoxApp::Update(const GameTimer& gt)
{
    // Convert Spherical to Cartesian coordinates.
    // 구면 좌표를 데카르트 좌표(직교 좌표)로 변환한다.
    float x = mRadius*sinf(mPhi)*cosf(mTheta);
    float z = mRadius*sinf(mPhi)*sinf(mTheta);
    float y = mRadius*cosf(mPhi);

    // Build the view matrix.
    // 시야 행렬을 구축한다.
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    XMMATRIX worldViewProj = world*view*proj;

	// Update the constant buffer with the latest worldViewProj matrix.
    // 최신의 worldViewProj 행렬로 상수 버퍼를 갱신한다.
	ObjectConstants objConstants;
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
    mObjectCB->CopyData(0, objConstants);
}

void BoxApp::Draw(const GameTimer& gt)
{
    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    // 명령 기록에 관련된 메모리의 재활용을 위해 명령 할당자를 재설정한다.
    // 재설정은 GPU가 관련 명령 목록들을 모두 처리한 후에 일어난다.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    // 명령 목록을 ExecuteCommandList를 통해서 명령 대기열에 추가했다면
    // 명령 목록을 재설정할 수 있다. 명령 목록을 재설정하면 메모리가 재활용된다.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
    // 자원 용도에 관련된 상태 전이를 Direct3D에 통지한다.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    // 후면버퍼와 깊이버퍼를 지운다.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
    // Specify the buffers we are going to render to.
    // 랜더링 결과가 기록될 렌더 대상 버퍼들을 지정한다.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
    // 아래 3개와 SetGraphicsRootDescriptorTable코드는 루트 서명과 CBV 힙을 명령 목록에 설정하고, 파이프라인에 묶을 자원들을 지정하는 서술자 테이블을 설정한다.
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    //ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ//
	mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());
	mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // 루트 서명을 설정한 후에 호출해서 서술자 테이블을 파이프라인에 묶는다.
    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart()); // 매개변수: 1. 설정하고자 하는 루트 서명의 색인, 2. 설정하고자 하는
                                                                                                     // 서술자 테이블의 첫 서술자에 해당하는 서술자(힙에 있는)의 핸들
                                                                                                     // 예를 들어 서술자 다섯 개를 담는 서술자 테이블로 정의된 루트 서명의 경우,
                                                                                                     // 힙에서 BaseDescriptor에 해당하는 서술자와 그 다음의 네 서술자가 루트 서명의
                                                                                                     // 서술자 테이블에 설정된다.

    mCommandList->DrawIndexedInstanced(
		mBoxGeo->DrawArgs["box"].IndexCount, 
		1, 0, 0, 0);
	
    // Indicate a state transition on the resource usage.
    // 자원 용도에 관련된 상태 전이를 Direct3D에 통지한다.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    // 명령들의 기록을 마친다.
	ThrowIfFailed(mCommandList->Close());
 
    // Add the command list to the queue for execution.
    // 명령 실행을 위해 명령 목록을 명령 대기열에 추가한다.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	
	// swap the back and front buffers
    // 후면 버퍼와 전면 버퍼를 교환한다.
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
    // 이 프레임의 명령들이 모두 처리되길 기다린다. 이러한 대기는 비효율적이다.
    // 이번에는 예제의 간단함을 위해 이 방법을 사용하지만 이후의 예제들에서는 렌더링
    // 코드를 적절히 조직화해서 프레임마다 대기할 필요가 없게 만든다.
	FlushCommandQueue();
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        // 마우스의 한 픽셀 이동을 4분의 1도에 대응 시킨다.
        float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        // 입력에 기초해 각도를 갱신해서 카메라가 상자를 중심으로 공전하게 한다.
        mTheta += dx;
        mPhi += dy;

        // Restrict the angle mPhi.
        // mPhi 각도를 제한한다.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.005 unit in the scene.
        // 마우스 한 픽셀 이동을 장면의 0.005단위에 대응시킨다.
        float dx = 0.005f*static_cast<float>(x - mLastMousePos.x);
        float dy = 0.005f*static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        // 입력에 기초해서 카메라 반지름을 갱신한다.
        mRadius += dx - dy;

        // Restrict the radius.
        // 반지름을 제한한다.
        mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void BoxApp::BuildDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
        IID_PPV_ARGS(&mCbvHeap)));
}

void BoxApp::BuildConstantBuffers()
{
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true); // 물체 2번째 매개변수 개의 상수 자료를 담을 상수 버퍼

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants)); // d3dUtil::CalcConstantBufferByteSize는 버퍼의 크기(바이트 개수)를 
                                                                                       // 최소 하드웨어 할당 크기(256바이트)의 배수가 되게하는 계산을 수행해준다.
    // 버퍼 자체의 시작 주소(0번째 상수 버퍼의 주소)
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
    // Offset to the ith object constant buffer in the buffer.
    // 버퍼에 담긴 i(여기선 0)번째 상수 버퍼의 오프셋을 얻는다.
    int boxCBufIndex = 0;
	cbAddress += boxCBufIndex*objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc; // 상수 버퍼 자원 중 HLSL 상수 버퍼 구조체에 묶일 부분을 서술한다.
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    //위에 둘을 지정함으로써 i번째 물체의 상수 자료에 대한 뷰를 얻을 수 있다.
	md3dDevice->CreateConstantBufferView(
		&cbvDesc,
		mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature()
{
	// Shader programs typically require resources as input (constant buffers,
	// textures, samplers).  The root signature defines the resources the shader
	// programs expect.  If we think of the shader programs as a function, and
	// the input resources as function parameters, then the root signature can be
	// thought of as defining the function signature.
    // 일반적으로 셰이더 프로그램은 특정 자원들(상수 버퍼, 텍스처, 표본추출기 등)이 입력된다고 기대한다.
    // 루트 서명은 셰이더 프로그램이 기대하는 자원들을 정의한다.
    // 세이더 프로그램은 본질적으로 하나의 함수이고 셰이더에 입력되는 자원들은 함수의 매개변수들에 해당하므로,
    // 루트 서명은 곧 함수 서명을 정의하는 수단이라 할 수 있다.

	// Root parameter can be a table, root descriptor or root constants.
    // 루트 매개변수는 서술자 테이블이거나 루트 서술자 또는 루트 상수이다.
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// Create a single descriptor table of CBVs.
    // CVB 하나를 담는 서술자 테이블을 생성한다.
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // 매개변수 2. 테이블의 서술자 개수, 3. 이 루트 매개변수에 묶일 셰이더 인수들의 기준 레지스터 번호
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable); // 1. 구간(range)개수, 2. 구간들의 배열을 가리키는 포인터

	// A root signature is an array of root parameters.
    // 루트 서명은 루트 매개변수들의 배열이다.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr, 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    // 상수 버퍼 하나로 구성된 서술자 구간을 가리키는
    // 슬롯 하나로 이루어진 루트 서명을 생성한다.
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if(errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);
    // CD3DX12_ROOT_PARAMETER와 CD3DX12_DESCRIPTOR_RANGE는 다음장에서 더 설명
    // 지금은 다음 코드가 CBV 하나(상수 버퍼 레지스터 0에 묶이는, 즉 HLSL 코드의 register(b0)에 대응되는)를 담은
    // 서술자 테이블을 시대하는 루트 매개변수를 생성한다는 점만 이해하고 넘어가자.
	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void BoxApp::BuildShadersAndInputLayout()
{
    HRESULT hr = S_OK;
    // 셰이더 프로그램 컴파일 호출
	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void BoxApp::BuildBoxGeometry()
{
    std::array<Vertex, 8> vertices =
    {
        Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
    };

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mBoxGeo = std::make_unique<MeshGeometry>();
	mBoxGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);

	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

	mBoxGeo->VertexByteStride = sizeof(Vertex);
	mBoxGeo->VertexBufferByteSize = vbByteSize;
	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mBoxGeo->DrawArgs["box"] = submesh;
}

void BoxApp::BuildPSO()
{
    //D3D12_GRAPHICS_PIPELINE_STATE_DESC: 파이프라인 상태를 서술한다.
    // 1. pRootSignature: 이 PSO와 함께 묶을 루트 서명을 가리키는 포인터. 루트 서명은 반드시 이 PSO로 묶는 셰이더들과 호환되어야 한다.
    // 2. VS: 묶을 정점 셰이더를 서술하는 D3D12_SHADER_BYTECODE 구조체. 이 구조체는 컴파일된 바이트코드 자료를 가리키는 포인터와
    // 그 바이트코드 자료의 크기(바이트 개수)로 구성된다.
    // 3. PS: 묶을 픽셀 셰이더
    // 4. DS: 묶을 영역 셰이더(영역 셰이더는 14.4절)
    // 5. HS: 묶을 덮개 셰이더(덮개 셰이더는 14.2절)
    // 6. GS: 묶을 기하 셰이더(기하 셰이더는 12장)
    // 7. StreamOutput: 스트림 출력이라고 하는 고급 기법에 쓰인다. 일단 지금은 이 필드에 그냥 0을 지정한다.
    // 8. BlendState: 혼합 방식을 서술하는 혼합 상태를 지정한다. 혼합과 혼합 상태 그룹은 제 10장에서 이야기한다.
    // 일단 지금은 기본값에 해당하는 CD3DX12_BLEND_DESC(D3D12_DEFAULT)를 지정한다.
    // 9. SampleMask: 다중표본화는 최대 32개의 표본을 취할 수 있다. 
    // 이 32비트 정수 필드의 각 비트는 각 표본의 활성화/비활성화 여부를 결정한다.
    // 예를 들어, 다섯 번째 비트를 끄면(0) 다섯 번째 표본은 추출되지 않는다.
    // 물론, 다중표본화가 다섯 개 이상의 표본을 사용하지 않는다면 다섯번째
    // 표본을 비활성화하는 것은 렌더링 결과에 영향을 미치지 않는다. 
    // 만일 응용 프로그램이 단일 표본화를 사용한다면, 이 필드에서 의미가 있는 것은 첫 비트 뿐이다.
    // 일반적으로 이 필드에는 기본값인 0xffffffff(그 어떤 표본도 비활성화하지 않는다)를 지정한다.
    // 10. RasterizerState: 래스터화 단계를 구성하는 래스터화기 상태를 지정한다.
    // 11. DepthStencilState: 깊이-스텐실 판정을 구성하는 깊이-스텐실 상태를 지정한다. 이 상태 그룹은 제 11장에서 논의한다.
    // 일단 지금은 기본값에 해당하는 CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT)를 지정한다.
    // 12. InputLayout: 입력 배치를 서술하는 구조체를 지정한다. 이 구조체는 다음과 같이 그냥 D3D12_INPUT_ELEMENT_DESC 원소들의 배열과 그 배열의
    // 원소 개수로 구성되어 있다.
    // 13. PrimitiveTopologyType: 기본도형 위상구조 종류를 지정한다.
    // 14. NumRenderTargets: 동시에 사용할 렌더 대상 개수.
    // 15. RTVFormats: 렌더 대상 형식들. 동시에 여러 렌더 대상에 장면을 그릴 수 있도록, 렌더 대상 형식들의 배열을 지정한다.
    // 그 형식들은 이 PSO와 함께 사용할 렌더 대상의 설정들과 부합해야 한다.
    // 16. DSVFormat: 깊이-스텐실 버퍼의 형식. 이 PSO와 함께 사용할 깊이-스텐실 버퍼의 설정들과 부합해야 한다.
    // 17. SampleDesc: 다중 표본화의 표본 개수와 품질 수준을 서술한다. 이 PSO와 함께 사용할 렌더 대상의 설정들과 부합해야 한다.
    // D3D12_GRAPHICS_PIPELINE_STATE_DESC을 채운 후에는 CreateGraphicsPipelineState 메서드를 이용해서 ID3D12PipelineState 객체를 생성한다.
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = 
	{ 
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()), 
		mvsByteCode->GetBufferSize() 
	};
    psoDesc.PS = 
	{ 
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()), 
		mpsByteCode->GetBufferSize() 
	};
    // CD3DX12_RASTERIZER_DESC(D3DX12_RASTERIZER_DESC을 상속해서 편의용 생성자를 추가한 보조 클래스) 타고 들어가면
    // D3DX12_RASTERIZER_DESC의 매개변수들이 있다.
    // D3DX12_RASTERIZER_DESC: 래스터화기 상태
    // 1. FillMode: 와이어프레임 렌더링을 위해서는 D3D12_FILL_WIREFRAME을,
    // 면의 속을 채운(solid) 렌더링을 위해서는 D3D12_FILL_SOLID를 지정한다.
    // 기본은 속을 채운 렌더링이다.
    // 2. CullMode: 선별을 끄려면 D3D12_CULL_NONE을, 후면 삼각형들을 선별(제외)하려면 D3D_CULL_BACK을,
    // 전면 삼각형들을 선별하려면 D3D12_CULL_FRONT를 지정한다.
    // 기본은 후면 삼각형 선별이다.
    // 3. FrontCounterClockWise: 정점들이 시계방향(카메라 기준)으로 감긴 삼각형을 전면 삼각형으로 취급하고
    // 반시계방향(카메라 기준)으로 감긴 삼각형을 후면 삼각형으로 취급하려면 false를 지정한다.
    // 정점들이 반시계방향(카메라 기준)으로 감긴 삼각형을 전면 삼각형으로 취급하고 시계방향(카메라 기준)으로
    // 감긴 삼각형을 후면 삼각형으로 취급하려면 true를 지정한다. 기본은 false이다.
    // 4. ScissorEnable: 가위 판정(4.3.10절)을 활성화하려면 true를, 비활성화하려면 false를 지정한다. 기본은 false이다.
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = mDepthStencilFormat;
    // 하나의 ID3D12PipelineState 객체(psoDesc)에 상당히 많은 상태가 들어있다.
    // 이 모든 객체를 하나의 집합체로서 렌더링 파이프라인에 지정하는 이유는 성능 때문이다.
    // 다렉3D는 모든 상태가 호환되는지 미리 검증할 수 있으며, 드라이버는 하드웨어 상태의 프로그래밍을 위한 모든 코드를 미리 생성할 수 있다.
    //
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}