#pragma once

#include "d3dUtil.h"
// 업로드 버퍼를 손쉽게 다룰 수 있는 예제 프레임워크
template<typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) : // 상수 버퍼에 사용할 때에는 isConstantBuffer를 true로 지정. 
        mIsConstantBuffer(isConstantBuffer)                                        // 그러면 각 상수 버퍼가 256바이트의 배수가 되도록 적절히 바이트 채움
    {
        mElementByteSize = sizeof(T);

        // Constant buffer elements need to be multiples of 256 bytes.
        // This is because the hardware can only view constant data 
        // at m*256 byte offsets and of n*256 byte lengths. 
        // 상수 버퍼 원소의 크기는 반드시 256바이트의 배수이어야 한다.
        // 이는 하드웨어가 m*256바이트 오프셋에서 시작하는 n*256바이트 길이의
        // 상수 자료만 볼 수 있기 때문이다.
        // typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
        // UINT64 OffsetInBytes; // multiple of 256(256의 배수)
        // UINT   SizeInBytes;   // multiple of 256
        // } D3D12_CONSTANT_BUFFER_VIEW_DESC;
        if(isConstantBuffer)
            mElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize*elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mUploadBuffer)));

        ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData))); // 매개변수: 1. CPU 메모리에 대응(mapping;사상)시키려는 부분 자원의 색인이다. 버퍼의 경우에는
                                                                                               // 버퍼 자체가 유일한 부분 자원이므로 그냥 0을 지정하면 된다.
                                                                                               // 2. 대응시킬 메모리의 범위를 서술하는 D3D12_RANGE 구조체의 포인터. 자원 전체를 대응하려면 nullptr
                                                                                               // 3. 대응된 자료를 가리키는 포인터. 시스템 메모리에 있는 자료를 상수 버퍼에 복사하려면 memcpy이용
                                                                                               // 상수 버퍼에 자료를 다 복사했으면, 해당 메모리를 해제하기 전에 Unmap을 호출해 주어야한다.
                                                                                               // 
                                                                                               // Unmap의 매개변수: 1. 대응을 해제할 부분 자원의 색인. 버퍼는 0
                                                                                               // 2. 대응을 해제할 메모리 범위를 서술하는 D3D12_RANGE 구조체의 포인터. 자원 전체를 대응하려면 nullptr
        // We do not need to unmap until we are done with the resource.  However, we must not write to
        // the resource while it is in use by the GPU (so we must use synchronization techniques).
        // 자원을 다 사용하기 전에는 대응을 해제할 필요가 없다.
        // 그러나, 자원을 GPU가 사용하는 중에는 CPU에서 자원을
        // 갱신하지 않아야한다(따라서 반드시 동기화 기법을 사용해야 한다).
    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    ~UploadBuffer()
    {
        if(mUploadBuffer != nullptr)
            mUploadBuffer->Unmap(0, nullptr);

        mMappedData = nullptr;
    }

    ID3D12Resource* Resource()const
    {
        return mUploadBuffer.Get();
    }

    void CopyData(int elementIndex, const T& data) // 버퍼의 특정 항목 갱신. CPU에서 업로드 버퍼의 내용을 번경해야 할 때(EX.시야행렬이 변했을 때) 사용
    {
        memcpy(&mMappedData[elementIndex*mElementByteSize], &data, sizeof(T));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
    BYTE* mMappedData = nullptr;

    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
};