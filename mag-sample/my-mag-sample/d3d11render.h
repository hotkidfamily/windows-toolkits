#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "VideoFrame.h"

class d3d11render {
public:
    ~d3d11render();

    HRESULT init(HWND hwnd);

    HRESULT display(const VideoFrame &frame);

    HRESULT release();

    HRESULT setMode(int mode)
    {
        _methodRender = mode;
        return S_OK;
    }

    HRESULT onResize(int cx, int cy);

  private:
    HRESULT _updateTexture(const VideoFrame &frame);
    HRESULT _reallocTexture(CSize destBufSize, int destBufFormat);
    HRESULT _setupVertex();
    HRESULT _setupShader(VideoFrameType);

  private:
    int _methodRender = 0;
    Microsoft::WRL::ComPtr<ID3D11Device> _dev = nullptr;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> _ctx = nullptr;
    Microsoft::WRL::ComPtr<IDXGISwapChain> _swapChain = nullptr;

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _mainRTV = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> _mainText = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuf = nullptr;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> _vs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> _ps;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> _yText = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> _uText = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> _vText = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _ySRV = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _uSRV = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _vSRV = nullptr;

    Microsoft::WRL::ComPtr<ID3D11SamplerState> _sampler = nullptr;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> _layout = nullptr;


    D3D11_TEXTURE2D_DESC m_usingDesc = { 0 };
    D3D_FEATURE_LEVEL _featureLevel;

    HWND _hwnd = nullptr;
    CSize _hwndSettingSz{ 0 };
    CSize _hwndSz{ 0 };
    

    VideoFrameType _curPixType = VideoFrameType::kVideoFrameTypeRGBA;
    CSize _curBuffSize;
};
