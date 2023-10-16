
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
// �����ϵ� ���̴� ����Ʈ�ڵ带 �����ϴ� �Լ�
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
    // ���� �⺻ ���� �ڿ��� �����Ѵ�.
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap. 
    // cpu �޸��� �ڷḦ �⺻ ���ۿ� �����Ϸ��� �ӽ� ���ε� ���� ������ �Ѵ�.
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    // Describe the data we want to copy into the default buffer.
    // �⺻���ۿ� ������ �ڷḦ �����Ѵ�.
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
    // will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
    // the intermediate upload heap data will be copied to mBuffer.
    // �⺻ ���� �ڿ������� �ڷ� ���縦 ��û�Ѵ�.
    // ���������� �����ڸ�, ���� �Լ� UpdateSubresources�� cpu�޸𸮸� �ӽ� ���ε� ���� �����ϰ�,
    // ID3D12CommandList::CopySubresourceRegion�� �̿��ؼ� �ӽ� ���ε� ���� �ڷḦ mBUffer�� �����Ѵ�.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), 
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    // Note: uploadBuffer has to be kept alive after the above function calls because
    // the command list has not been executed yet that performs the actual copy.
    // The caller can Release the uploadBuffer after it knows the copy has been executed.
    // ����: ���� �Լ� ȣ�� ���Ŀ��� uploadBuffer�� ��� �����ؾ��Ѵ�. ������ ���縦 �����ϴ� ��� �����
    // ���� ������� �ʾұ� �����̴�. ���簡 �Ϸ�Ǿ����� Ȯ������ �Ŀ� ȣ���ڰ� uploadBuffer�� �����ϸ� �ȴ�.

    return defaultBuffer;
}
// �� �Լ��� ������ ���� �޽����� Visual Studio�� ����� â�� ����Ѵ�(����� ��忡��).
ComPtr<ID3DBlob> d3dUtil::CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
    // ����� ��忡���� ����� ���� �÷��׵��� ����Ѵ�.
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
    // 6.7 ���̴��� ������
    // 1. pFileName: �������� HLSL �ҽ� �ڵ带 ���� .hlsl ������ �̸�.
    // 2. pDefines: ��� �ɼ�����, �� å������ ��� ����. �ڼ��� ������ SDK ����ȭ ����
    // 3. pInclude: ��� �ɼ�����, �� å������ ��� ����. �ڼ��� ������ SDK ����ȭ ����
    // 4. pEntrypoint: ���̴� ���α׷��� ������ �Լ��� �̸�. �ϳ��� .hlsl ���Ͽ� ���� ���� ���̴� ���α׷��� ���� �� �����Ƿ�
    // (�̸��׸� ���� ���̴� �ϳ��� �ȼ� ���̴� �ϳ�), �������� Ư�� ���̴��� �������� ����� �־�� �Ѵ�.
    // 5. pTarget: ����� ���̴� ���α׷��� ������ ��� ������ ��Ÿ���� ���ڿ�. �� å�� �������� 5.0�� 5.1�� ����Ѵ�.
    // (a) vs_5_0�� vs_5_1: ���� ���� ���̴� 5.0�� 5.1
    // (b) hs_5_0�� hs_5_1: ���� ����(��14.2) ���̴� 5.0�� 5.1
    // (c) ds_5_0�� ds_5_1: ���� ����(��14.4) ���̴� 5.0�� 5.1
    // (d) gs_5_0�� gs_5_1: ���� ����(�� 12��) ���̴� 5.0�� 5.1
    // (e) ps_5_0�� ps_5_1: ���� �ȼ� ���̴� 5.0�� 5.1
    // (f) cs_5_0�� cs_5_1: ���� ���(�� 13��) ���̴� 5.0�� 5.1
    // 6. Flags1: ���̴� �ڵ��� �������� ������ ����� �����ϴ� �÷��׵�. SDK���� ���� �÷��װ� ������ å���� �ΰ�����
    // (a) D3DCOMPILE_DEBUG: ���̴��� ����� ��忡�� ������ �Ѵ�.
    // (b) D3DCOMPILE_SKIP_OPTIMIZATION: ����ȭ�� �����Ѵ�(����뿡 ������).
    // 7. Flags2: ȿ��(effect)�� �����Ͽ� ���� ��� �ɼ�����, �� å������ ������� �ʴ´�.
    // 8. ppCode: �����ϵ� ���̴� ���� ����Ʈ�ڵ�(shader object bytecode)�� ���� ID3DBlob����ü�� �����͸� �� �Ű������� ���ؼ� �����ش�.
    // 9. ppErrorMsgs: ������ ������ �߻��� ��� ���� �޽��� ���ڿ��� ���� ID3DBlob ����ü�� �����͸� �� �Ű������� ���ؼ� �����ش�.
    // ID3DBlob�� ���� �޸� ���۸� ��Ÿ���� ��������, ���� �� �޼��带 �����Ѵ�.
    // (a) LPVOID GetBufferPointer: ���۸� ����Ű�� void* �����͸� �����ش�. �� ��Ͽ� ��� ��ü�� ������ ����Ϸ��� ���� ������ �������� ĳ�����ؾ� �Ѵ�.
    // (b) SIZE_T GetBufferSize: ������ ũ��(����Ʈ ����)�� �����ش�.
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);
    // ���� �޽����� ����� â�� ����Ѵ�.
    // HLSL�� ���� �޽����� ��� �޽����� ppErrorMsgs �Ű������� ���ؼ� ��ȯ�ȴ�. ���⼱ errors
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


