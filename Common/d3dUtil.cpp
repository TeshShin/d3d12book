
#include "d3dUtil.h"
#include <comdef.h>
#include <fstream>

using Microsoft::WRL::ComPtr;

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
    ErrorCode(hr),
    FunctionName(functionName),
    Filename(filename),
    LineNumber(lineNumber)
{
}

bool d3dUtil::IsKeyDown(int vkeyCode)
{
    return (GetAsyncKeyState(vkeyCode) & 0x8000) != 0;
}
// 컴파일된 셰이더 바이트코드를 적재하는 함수
ComPtr<ID3DBlob> d3dUtil::LoadBinary(const std::wstring& filename)
{
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;
    // 
    // Create the actual default buffer resource.
    // 실제 기본 버퍼 자원을 생성한다.
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap. 
    // cpu 메모리의 자료를 기본 버퍼에 복사하려면 임시 업로드 힙을 만들어야 한다.
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    // Describe the data we want to copy into the default buffer.
    // 기본버퍼에 복사할 자료를 서술한다.
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
    // will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
    // the intermediate upload heap data will be copied to mBuffer.
    // 기본 버퍼 자원으로의 자료 복사를 요청한다.
    // 개략적으로 말하자면, 보조 함수 UpdateSubresources는 cpu메모리를 임시 업로드 힙에 복사하고,
    // ID3D12CommandList::CopySubresourceRegion을 이용해서 임시 업로드 힙의 자료를 mBUffer에 복사한다.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), 
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    // Note: uploadBuffer has to be kept alive after the above function calls because
    // the command list has not been executed yet that performs the actual copy.
    // The caller can Release the uploadBuffer after it knows the copy has been executed.
    // 주의: 위의 함수 호출 이후에도 uploadBuffer를 계속 유지해야한다. 실제로 복사를 수행하는 명령 목록이
    // 아직 실행되지 않았기 때문이다. 복사가 완료되었음이 확실해진 후에 호출자가 uploadBuffer를 해제하면 된다.

    return defaultBuffer;
}
// 이 함수는 컴파일 오류 메시지를 Visual Studio의 디버그 창에 출력한다(디버그 모드에서).
ComPtr<ID3DBlob> d3dUtil::CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
    // 디버그 모드에서는 디버깅 관련 플래그들을 사용한다.
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
    // 6.7 셰이더의 컴파일
    // 1. pFileName: 컴파일할 HLSL 소스 코드를 담은 .hlsl 파일의 이름.
    // 2. pDefines: 고급 옵션으로, 이 책에서는 사용 안함. 자세한 사항은 SDK 문서화 참고
    // 3. pInclude: 고급 옵션으로, 이 책에서는 사용 안함. 자세한 사항은 SDK 문서화 참고
    // 4. pEntrypoint: 셰이더 프로그램의 진입점 함수의 이름. 하나의 .hlsl 파일에 여러 개의 셰이더 프로그램이 있을 수 있으므로
    // (이를테면 정점 셰이더 하나와 픽셀 셰이더 하나), 컴파일할 특정 셰이더의 진입점을 명시해 주어야 한다.
    // 5. pTarget: 사용할 셰이더 프로그램의 종류와 대상 버전을 나타내는 문자열. 이 책의 예제들은 5.0과 5.1을 사용한다.
    // (a) vs_5_0과 vs_5_1: 각각 정점 셰이더 5.0과 5.1
    // (b) hs_5_0과 hs_5_1: 각각 덮개(절14.2) 셰이더 5.0과 5.1
    // (c) ds_5_0과 ds_5_1: 각각 영역(절14.4) 셰이더 5.0과 5.1
    // (d) gs_5_0과 gs_5_1: 각각 기하(제 12장) 셰이더 5.0과 5.1
    // (e) ps_5_0과 ps_5_1: 각각 픽셀 셰이더 5.0과 5.1
    // (f) cs_5_0과 cs_5_1: 각각 계산(제 13장) 셰이더 5.0과 5.1
    // 6. Flags1: 셰이더 코드의 세부적인 컴파일 방식을 제어하는 플래그들. SDK에는 많은 플래그가 있지만 책에선 두가지만
    // (a) D3DCOMPILE_DEBUG: 셰이더를 디버그 모드에서 컴파일 한다.
    // (b) D3DCOMPILE_SKIP_OPTIMIZATION: 최적화를 생략한다(디버깅에 유용함).
    // 7. Flags2: 효과(effect)의 컴파일에 관한 고급 옵션으로, 이 책에서는 사용하지 않는다.
    // 8. ppCode: 컴파일된 셰이더 목적 바이트코드(shader object bytecode)를 담은 ID3DBlob구조체의 포인터를 이 매개변수를 통해서 돌려준다.
    // 9. ppErrorMsgs: 컴파일 오류가 발생한 경우 오류 메시지 문자열을 담은 ID3DBlob 구조체의 포인터를 이 매개변수를 통해서 돌려준다.
    // ID3DBlob은 범용 메모리 버퍼를 나타내는 형식으로, 다음 두 메서드를 제공한다.
    // (a) LPVOID GetBufferPointer: 버퍼를 가리키는 void* 포인터를 돌려준다. 그 블록에 담긴 객체를 실제로 사용하려면 먼저 적절한 형식으로 캐스팅해야 한다.
    // (b) SIZE_T GetBufferSize: 버퍼의 크기(바이트 개수)를 돌려준다.
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);
    // 오류 메시지를 디버그 창에 출력한다.
    // HLSL의 오류 메시지와 경고 메시지는 ppErrorMsgs 매개변수를 통해서 반환된다. 여기선 errors
	if(errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ThrowIfFailed(hr);

	return byteCode;
}

std::wstring DxException::ToString()const
{
    // Get the string description of the error code.
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}


