#pragma once

#include "d3dUtil.h"
// ���ε� ���۸� �ս��� �ٷ� �� �ִ� ���� �����ӿ�ũ
template<typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) : // ��� ���ۿ� ����� ������ isConstantBuffer�� true�� ����. 
        mIsConstantBuffer(isConstantBuffer)                                        // �׷��� �� ��� ���۰� 256����Ʈ�� ����� �ǵ��� ������ ����Ʈ ä��
    {
        mElementByteSize = sizeof(T);

        // Constant buffer elements need to be multiples of 256 bytes.
        // This is because the hardware can only view constant data 
        // at m*256 byte offsets and of n*256 byte lengths. 
        // ��� ���� ������ ũ��� �ݵ�� 256����Ʈ�� ����̾�� �Ѵ�.
        // �̴� �ϵ��� m*256����Ʈ �����¿��� �����ϴ� n*256����Ʈ ������
        // ��� �ڷḸ �� �� �ֱ� �����̴�.
        // typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
        // UINT64 OffsetInBytes; // multiple of 256(256�� ���)
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

        ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData))); // �Ű�����: 1. CPU �޸𸮿� ����(mapping;���)��Ű���� �κ� �ڿ��� �����̴�. ������ ��쿡��
                                                                                               // ���� ��ü�� ������ �κ� �ڿ��̹Ƿ� �׳� 0�� �����ϸ� �ȴ�.
                                                                                               // 2. ������ų �޸��� ������ �����ϴ� D3D12_RANGE ����ü�� ������. �ڿ� ��ü�� �����Ϸ��� nullptr
                                                                                               // 3. ������ �ڷḦ ����Ű�� ������. �ý��� �޸𸮿� �ִ� �ڷḦ ��� ���ۿ� �����Ϸ��� memcpy�̿�
                                                                                               // ��� ���ۿ� �ڷḦ �� ����������, �ش� �޸𸮸� �����ϱ� ���� Unmap�� ȣ���� �־���Ѵ�.
                                                                                               // 
                                                                                               // Unmap�� �Ű�����: 1. ������ ������ �κ� �ڿ��� ����. ���۴� 0
                                                                                               // 2. ������ ������ �޸� ������ �����ϴ� D3D12_RANGE ����ü�� ������. �ڿ� ��ü�� �����Ϸ��� nullptr
        // We do not need to unmap until we are done with the resource.  However, we must not write to
        // the resource while it is in use by the GPU (so we must use synchronization techniques).
        // �ڿ��� �� ����ϱ� ������ ������ ������ �ʿ䰡 ����.
        // �׷���, �ڿ��� GPU�� ����ϴ� �߿��� CPU���� �ڿ���
        // �������� �ʾƾ��Ѵ�(���� �ݵ�� ����ȭ ����� ����ؾ� �Ѵ�).
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

    void CopyData(int elementIndex, const T& data) // ������ Ư�� �׸� ����. CPU���� ���ε� ������ ������ �����ؾ� �� ��(EX.�þ������ ������ ��) ���
    {
        memcpy(&mMappedData[elementIndex*mElementByteSize], &data, sizeof(T));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
    BYTE* mMappedData = nullptr;

    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
};