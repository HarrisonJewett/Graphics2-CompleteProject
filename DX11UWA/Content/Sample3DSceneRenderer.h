﻿#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"

#include <vector>
#include "..\Common\DDSTextureLoader.h"


namespace DX11UWA
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources(void);
		void CreateWindowSizeDependentResources(void);
		void ReleaseDeviceDependentResources(void);
		void Update(DX::StepTimer const& timer);
		void Render(void);
		void postRender(ID3D11DeviceContext3 * context);
		void StartTracking(void);
		void TrackingUpdate(float positionX);
		void StopTracking(void);
		inline bool IsTracking(void) { return m_tracking; }

		// Helper functions for keyboard and mouse input
		void SetKeyboardButtons(const char* list);
		void SetMousePosition(const Windows::UI::Input::PointerPoint^ pos);
		void SetInputDeviceData(const char* kb, const Windows::UI::Input::PointerPoint^ pos);


	private:
		void Rotate(float radians);
		void UpdateCamera(DX::StepTimer const& timer, float const moveSpd, float const rotSpd);

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// Direct3D resources for cube geometry.
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_cubeResourceView;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>		 m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			 m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			 m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		 m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		 m_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			 m_constantBuffer;

		// System resources for cube geometry.
		ModelViewProjectionConstantBuffer	m_constantBufferData;
		uint32	m_indexCount;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_degreesPerSecond;
		bool	m_tracking;

		// Data members for keyboard and mouse input
		char	m_kbuttons[256];
		Windows::UI::Input::PointerPoint^ m_currMousePos;
		Windows::UI::Input::PointerPoint^ m_prevMousePos;

		// Matrix data member for the camera
		DirectX::XMFLOAT4X4 m_camera;

		//Loading Floor object (now castle)
		std::vector<VertexPositionUVNormal>					m_floorVerticies;
		std::vector<unsigned int>							m_floorIndicies;
		std::vector<VertexPositionUVNormal>					m_floorVertexPositionUVNormal;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_floorVertBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_floorIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_floorVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_floorPixelShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_floorConstantBuffer;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>			m_floorInputLayout;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>			m_floorSampleState;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_floorResourceView;
		ModelViewProjectionConstantBuffer					m_floorConstantBufferData;

		//Wolves
		std::vector<VertexPositionUVNormal>					m_wolfVerticies;
		std::vector<unsigned int>							m_wolfIndicies;
		std::vector<VertexPositionUVNormal>					m_wolfVertexPositionUVNormal;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_wolfVertBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_wolfIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_wolfVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_wolfPixelShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_wolfConstantBuffer;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>			m_wolfInputLayout;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>			m_wolfSampleState;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_wolfResourceView;
		ModelViewProjectionConstantBuffer					m_wolfConstantBufferData;

		//Skybox
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_skyBoxResourceView;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>		 m_skyBoxInput;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			 m_skyBoxVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			 m_skyBoxIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		 m_skyBoxVS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		 m_skyBoxPS;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			 m_skyBoxConstantBuffer;
		uint32 m_skyICount;

		//Viewports
		D3D11_VIEWPORT * m_vp1;
		D3D11_VIEWPORT * m_vp2;
		D3D11_VIEWPORT * m_vp3;
		bool multipleViewports = false;

		//planes
		float nearPlane = 0.01f;
		float farPlane = 200.0f;
		float fov = 70.0f * 3.14f / 180.0f;
		bool planeChange = false;
		float moveSpeed = 1.0;

		//Scene within a scene
		Microsoft::WRL::ComPtr<ID3D11InputLayout>			   m_innerSceneInputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				   m_innerSceneVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				   m_innerSceneIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>			   m_innerSceneVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			   m_innerScenePixelShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				   m_innerSceneConstantBuffer;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>			   m_innerSceneSampleState;
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				   m_innerTargetTexture;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>		   m_innerRenderTarget;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	   m_innerShaderResourceView;

		ModelViewProjectionConstantBuffer	m_innerSceneConstantBufferData;
		uint32								m_innerSceneIndexCount;

		//New Floor
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_stoneResourceView;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>		 m_stoneInput;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			 m_stoneVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			 m_stoneIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		 m_stoneVS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		 m_stonePS;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			 m_stoneConstantBuffer;
		uint32 m_stoneICount;

		//need constant buffer for each light
		//constant buffers need to be 16 bytes
	};
}

