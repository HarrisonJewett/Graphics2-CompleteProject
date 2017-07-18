#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"

using namespace DX11UWA;

using namespace DirectX;
using namespace Windows::Foundation;

bool loadObject(const char * path, std::vector <VertexPositionUVNormal> & outVertices, std::vector <unsigned int> & outIndicies);

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_degreesPerSecond(45),
	m_indexCount(0),
	m_tracking(false),
	m_deviceResources(deviceResources)
{
	memset(m_kbuttons, 0, sizeof(m_kbuttons));
	m_currMousePos = nullptr;
	m_prevMousePos = nullptr;
	memset(&m_camera, 0, sizeof(XMFLOAT4X4));

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources(void)
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	m_vp1 = new D3D11_VIEWPORT();
	m_vp1->Height = outputSize.Height;
	m_vp1->Width = outputSize.Width / 2;
	m_vp1->MinDepth = 0.0f;
	m_vp1->MaxDepth = 0.1f;
	m_vp1->TopLeftX = 0.0f;
	m_vp1->TopLeftY = 0.0f;

	m_vp2 = new D3D11_VIEWPORT();
	m_vp2->Height = outputSize.Height;
	m_vp2->Width = outputSize.Width / 2;
	m_vp2->MinDepth = 0.0f;
	m_vp2->MaxDepth = 0.1f;
	m_vp2->TopLeftX = outputSize.Width / 2;
	m_vp2->TopLeftY = 0.0f;

	m_vp3 = new D3D11_VIEWPORT();
	m_vp3->Height = outputSize.Height;
	m_vp3->Width = outputSize.Width;
	m_vp3->MinDepth = 0.0f;
	m_vp3->MaxDepth = 0.1f;
	m_vp3->TopLeftX = 0.0f;
	m_vp3->TopLeftY = 0.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearPlane, farPlane);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(&m_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
	XMStoreFloat4x4(&m_floorConstantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));


	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.0f, 0.7f, -1.5f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(&m_camera, XMMatrixInverse(nullptr, XMMatrixLookAtLH(eye, at, up)));
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));
	XMStoreFloat4x4(&m_floorConstantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	if (!m_tracking)
	{
		// Convert degrees to radians, then convert seconds to rotation angle
		float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
		double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
		float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

		Rotate(radians);
	}


	// Update or move camera here
	UpdateCamera(timer, 2.0f, 0.75f);

}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
	XMStoreFloat4x4(&m_floorConstantBufferData.model, XMMatrixTranspose(XMMatrixIdentity()));

}

void Sample3DSceneRenderer::UpdateCamera(DX::StepTimer const& timer, float const moveSpd, float const rotSpd)
{
	const float delta_time = (float)timer.GetElapsedSeconds();
	Size outputSize = m_deviceResources->GetOutputSize();

	float aspectRatio = outputSize.Width / outputSize.Height;


	if (m_kbuttons['W'])
	{
		XMMATRIX translation = XMMatrixTranslation(0.0f, 0.0f, moveSpd * delta_time);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['S'])
	{
		XMMATRIX translation = XMMatrixTranslation(0.0f, 0.0f, -moveSpd * delta_time);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['A'])
	{
		XMMATRIX translation = XMMatrixTranslation(-moveSpd * delta_time, 0.0f, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['D'])
	{
		XMMATRIX translation = XMMatrixTranslation(moveSpd * delta_time, 0.0f, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['X'])
	{
		XMMATRIX translation = XMMatrixTranslation( 0.0f, -moveSpd * delta_time, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons[VK_SPACE])
	{
		XMMATRIX translation = XMMatrixTranslation( 0.0f, moveSpd * delta_time, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['9'])
	{
		multipleViewports = true;
	}
	if (m_kbuttons['0'])
	{
		multipleViewports = false;
	}
	if (m_kbuttons['U'])
	{
		planeChange = true;
		fov = 70.0f * XM_PI / 180.0f;
	}
	if (m_kbuttons['J'])
	{
		planeChange = true;
		fov *= 1.01f;
	}
	if (m_kbuttons['M'])
	{
		planeChange = true;
		fov *= 0.99f;
	}
	if (m_kbuttons['O'])
	{
		farPlane++;
		planeChange = true;
	}
	if (m_kbuttons['L'])
	{
		farPlane--;
		planeChange = true;
		if (farPlane < nearPlane)
			farPlane = nearPlane + 1;
	}
	if (m_kbuttons['I'])
	{
		nearPlane++;
		planeChange = true;
		if (nearPlane > farPlane)
			nearPlane = farPlane - 10;
	}
	if (m_kbuttons['K'])
	{
		nearPlane--;
		planeChange = true;
		if (nearPlane < 0)
			nearPlane = 0.01f;
	}


	if (planeChange)
	{
		XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);

		XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

		XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

		XMStoreFloat4x4(&m_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&m_floorConstantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		planeChange = false;
	}

	if (m_currMousePos) 
	{
		if (m_currMousePos->Properties->IsRightButtonPressed && m_prevMousePos)
		{
			float dx = m_currMousePos->Position.X - m_prevMousePos->Position.X;
			float dy = m_currMousePos->Position.Y - m_prevMousePos->Position.Y;

			XMFLOAT4 pos = XMFLOAT4(m_camera._41, m_camera._42, m_camera._43, m_camera._44);

			m_camera._41 = 0;
			m_camera._42 = 0;
			m_camera._43 = 0;

			XMMATRIX rotX = XMMatrixRotationX(dy * rotSpd * delta_time);
			XMMATRIX rotY = XMMatrixRotationY(dx * rotSpd * delta_time);

			XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
			temp_camera = XMMatrixMultiply(rotX, temp_camera);
			temp_camera = XMMatrixMultiply(temp_camera, rotY);

			XMStoreFloat4x4(&m_camera, temp_camera);

			m_camera._41 = pos.x;
			m_camera._42 = pos.y;
			m_camera._43 = pos.z;
		}
		m_prevMousePos = m_currMousePos;
	}
}

void Sample3DSceneRenderer::SetKeyboardButtons(const char* list)
{
	memcpy_s(m_kbuttons, sizeof(m_kbuttons), list, sizeof(m_kbuttons));
}

void Sample3DSceneRenderer::SetMousePosition(const Windows::UI::Input::PointerPoint^ pos)
{
	m_currMousePos = const_cast<Windows::UI::Input::PointerPoint^>(pos);
}

void Sample3DSceneRenderer::SetInputDeviceData(const char* kb, const Windows::UI::Input::PointerPoint^ pos)
{
	SetKeyboardButtons(kb);
	SetMousePosition(pos);
}

void DX11UWA::Sample3DSceneRenderer::StartTracking(void)
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
	if (m_tracking)
	{
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void Sample3DSceneRenderer::StopTracking(void)
{
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render(void)
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}
	auto context = m_deviceResources->GetD3DDeviceContext();

	if (multipleViewports)
	{
		context->RSSetViewports(1, m_vp1);
		postRender(context);

		context->RSSetViewports(1, m_vp2);
		postRender(context);
	}
	else
	{
		context->RSSetViewports(1, m_vp3);
		postRender(context);
	}
}

void Sample3DSceneRenderer::postRender(ID3D11DeviceContext3 * context)
{
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));

	//Sky
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixScaling(100.0f, 100.0f, 100.0f));
	context->UpdateSubresource1(m_skyBoxConstantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);
	UINT stride = sizeof(Sky);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, m_skyBoxVertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_skyBoxIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(m_skyBoxInput.Get());
	context->VSSetShader(m_skyBoxVS.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, m_skyBoxConstantBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetShader(m_skyBoxPS.Get(), nullptr, 0);
	context->PSSetShaderResources(0, 1, m_skyBoxResourceView.GetAddressOf());
	context->DrawIndexed(m_skyICount, 0, 0);

	//UV Cube
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixScaling(1.0f, 1.0f, 1.0f));


	context->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);
	stride = sizeof(VertexPositionUVNormal);
	offset = 0;
	context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(m_inputLayout.Get());
	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
	//context->DrawIndexed(m_indexCount, 0, 0);

	//Floor / ice castle
	XMStoreFloat4x4(&m_floorConstantBufferData.model, XMMatrixTranspose(XMMatrixMultiply(XMMatrixRotationY(3.14f), XMMatrixTranslation(0.0f, -2.0f, 2.0f))));

	XMStoreFloat4x4(&m_floorConstantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));

	context->UpdateSubresource1(m_floorConstantBuffer.Get(), 0, NULL, &m_floorConstantBufferData, 0, 0, 0);
	stride = sizeof(VertexPositionUVNormal);
	offset = 0;
	context->IASetVertexBuffers(0, 1, m_floorVertBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_floorIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(m_floorInputLayout.Get());
	context->VSSetShader(m_floorVertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, m_floorConstantBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetShader(m_floorPixelShader.Get(), nullptr, 0);
	context->PSSetShaderResources(0, 1, m_floorResourceView.GetAddressOf());
	context->PSSetSamplers(0, 1, m_floorSampleState.GetAddressOf());
	context->DrawIndexed(m_floorIndicies.size(), 0, 0);

	//Stone floor
	//XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixScaling(1.0f, 0.2f, 1.0f));
	//context->UpdateSubresource1(m_stoneConstantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);
	//stride = sizeof(Sky);
	//offset = 0;
	//context->IASetVertexBuffers(0, 1, m_stoneVertexBuffer.GetAddressOf(), &stride, &offset);
	//context->IASetIndexBuffer(m_stoneIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	//context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//context->IASetInputLayout(m_stoneInput.Get());
	//context->VSSetShader(m_stoneVS.Get(), nullptr, 0);
	//context->VSSetConstantBuffers1(0, 1, m_stoneConstantBuffer.GetAddressOf(), nullptr, nullptr);
	//context->PSSetShader(m_stonePS.Get(), nullptr, 0);
	//context->PSSetShaderResources(0, 1, m_stoneResourceView.GetAddressOf());
	//context->DrawIndexed(m_stoneICount, 0, 0);


	//Change this.
	//Need to draw to an offscreen texture, then draw the box, then put the texture on it. I think
	if (false)
	{
		context->OMSetRenderTargets(1, m_innerRenderTarget.GetAddressOf(), m_deviceResources->GetDepthStencilView());
		context->ClearRenderTargetView(m_innerRenderTarget.Get(), DirectX::Colors::SeaGreen);
		XMStoreFloat4x4(&m_innerSceneConstantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));
		XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));
		XMStoreFloat4x4(&m_floorConstantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));
		XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixIdentity());
		XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));

		//Sky
		XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixScaling(100.0f, 100.0f, 100.0f));
		context->UpdateSubresource1(m_skyBoxConstantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);
		UINT stride = sizeof(Sky);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, m_skyBoxVertexBuffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(m_skyBoxIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetInputLayout(m_skyBoxInput.Get());
		context->VSSetShader(m_skyBoxVS.Get(), nullptr, 0);
		context->VSSetConstantBuffers1(0, 1, m_skyBoxConstantBuffer.GetAddressOf(), nullptr, nullptr);
		context->PSSetShader(m_skyBoxPS.Get(), nullptr, 0);
		context->PSSetShaderResources(0, 1, m_skyBoxResourceView.GetAddressOf());
		context->DrawIndexed(m_skyICount, 0, 0);

		//UV Cube
		XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));
		XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		context->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);
		stride = sizeof(VertexPositionUVNormal);
		offset = 0;
		context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetInputLayout(m_inputLayout.Get());
		context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
		context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
		context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
		context->DrawIndexed(m_indexCount, 0, 0);

		//Floor / ice castle
		XMStoreFloat4x4(&m_floorConstantBufferData.model, XMMatrixTranspose(XMMatrixMultiply(XMMatrixRotationY(3.14f), XMMatrixTranslation(0.0f, -2.0f, 2.0f))));
		XMStoreFloat4x4(&m_floorConstantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));
		context->UpdateSubresource1(m_floorConstantBuffer.Get(), 0, NULL, &m_floorConstantBufferData, 0, 0, 0);
		stride = sizeof(VertexPositionUVNormal);
		offset = 0;
		context->IASetVertexBuffers(0, 1, m_floorVertBuffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(m_floorIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetInputLayout(m_floorInputLayout.Get());
		context->VSSetShader(m_floorVertexShader.Get(), nullptr, 0);
		context->VSSetConstantBuffers1(0, 1, m_floorConstantBuffer.GetAddressOf(), nullptr, nullptr);
		context->PSSetShader(m_floorPixelShader.Get(), nullptr, 0);
		context->PSSetShaderResources(0, 1, m_floorResourceView.GetAddressOf());
		context->PSSetSamplers(0, 1, m_floorSampleState.GetAddressOf());
		context->DrawIndexed(m_floorIndicies.size(), 0, 0);

		//Scene within a scene
		XMStoreFloat4x4(&m_innerSceneConstantBufferData.model, XMMatrixTranspose(XMMatrixTranslation(-2.0f, 0.0f, 2.0f)));
		XMStoreFloat4x4(&m_innerSceneConstantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));
		m_innerSceneConstantBufferData.projection = m_constantBufferData.projection;
		context->UpdateSubresource1(m_innerSceneConstantBuffer.Get(), 0, NULL, &m_innerSceneConstantBufferData, 0, 0, 0);
		stride = sizeof(VertexPositionUVNormal);
		offset = 0;
		context->IASetVertexBuffers(0, 1, m_innerSceneVertexBuffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(m_innerSceneIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetInputLayout(m_innerSceneInputLayout.Get());
		context->VSSetShader(m_innerSceneVertexShader.Get(), nullptr, 0);
		context->VSSetConstantBuffers1(0, 1, m_innerSceneConstantBuffer.GetAddressOf(), nullptr, nullptr);
		context->PSSetShader(m_innerScenePixelShader.Get(), nullptr, 0);
		context->PSSetShaderResources(0, 1, m_innerShaderResourceView.GetAddressOf());
		context->PSSetSamplers(0, 1, m_innerSceneSampleState.GetAddressOf());
		context->DrawIndexed(m_innerSceneIndexCount, 0, 0);
	}
}

void Sample3DSceneRenderer::CreateDeviceDependentResources(void)
{
	// Load shaders asynchronously.
	//Cube
	//Normal
	auto loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");
	//Attempting lighting on cube. Currently give weird green lines
	//auto loadVSTask = DX::ReadDataAsync(L"LightingVertexShader.cso");
	//auto loadPSTask = DX::ReadDataAsync(L"LightingPixelShader.cso");

	//Floor
	//auto loadFloorVSTask = DX::ReadDataAsync(L"FloorVertexShader.cso");
	//auto loadFloorPSTask = DX::ReadDataAsync(L"FloorPixelShader.cso");
	//Floor with lighting
	auto loadFloorVSTask = DX::ReadDataAsync(L"LightingVertexShader.cso");
	auto loadFloorPSTask = DX::ReadDataAsync(L"LightingPixelShader.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateVertexShader(&fileData[0], fileData.size(), nullptr, &m_vertexShader));

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "UV", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			//testing. Lets the code build and run while the cube uses the lighting shaders
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			//end testing
		};

		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), &fileData[0], fileData.size(), &m_inputLayout));
	});

	auto createFloorVSTask = loadFloorVSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateVertexShader(&fileData[0], fileData.size(), nullptr, &m_floorVertexShader));

		static const D3D11_INPUT_ELEMENT_DESC floorVertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "UV", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateInputLayout(floorVertexDesc, ARRAYSIZE(floorVertexDesc), &fileData[0], fileData.size(), &m_floorInputLayout));
	});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &m_pixelShader));

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, &m_constantBuffer));
	});

	auto createFloorPSTask = loadFloorPSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &m_floorPixelShader));

		CD3D11_BUFFER_DESC floorConstantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&floorConstantBufferDesc, nullptr, &m_floorConstantBuffer));


	});

	//----------------CREATING SKYBOX-------------------//

	auto loadVSSkyTask = DX::ReadDataAsync(L"SkyVertexShader.cso");

	auto createSkyVSTask = loadVSSkyTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateVertexShader(&fileData[0], fileData.size(), nullptr, &m_skyBoxVS));

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), &fileData[0], fileData.size(), &m_skyBoxInput));
	});

	auto loadPSSkyTask = DX::ReadDataAsync(L"SkyPixelShader.cso");

	auto createSkyPSTask = loadPSSkyTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &m_skyBoxPS));

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);

		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, &m_skyBoxConstantBuffer));
	});

	auto createSkyBox = (createSkyVSTask && createSkyPSTask).then([this]()
	{
		static const Sky skyBox[] =
		{
			{ XMFLOAT3(-1.0f, -1.0f, -1.0f) },
			{ XMFLOAT3(-1.0f, -1.0f,  1.0f) },
			{ XMFLOAT3(-1.0f,  1.0f, -1.0f) },
			{ XMFLOAT3(-1.0f,  1.0f,  1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, -1.0f) },
			{ XMFLOAT3(1.0f, -1.0f,  1.0f) },
			{ XMFLOAT3(1.0f,  1.0f, -1.0f) },
			{ XMFLOAT3(1.0f,  1.0f,  1.0f) },
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = skyBox;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(skyBox), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_skyBoxVertexBuffer));

		static const unsigned short Indices[] =
		{
			3,7,5,
			5,1,3,

			7,6,4,
			4,5,7,

			6,2,0,
			0,4,6,

			2,3,1,
			1,0,2,

			2,6,7,
			7,3,2,

			5,4,0,
			0,1,5,
		};

		D3D11_SAMPLER_DESC sampleDesc;
		ZeroMemory(&sampleDesc, sizeof(sampleDesc));
		sampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
		sampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
		sampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;

		DX::ThrowIfFailed(CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets/SkyboxOcean.dds", NULL, m_skyBoxResourceView.GetAddressOf()));

		m_skyICount = ARRAYSIZE(Indices);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = Indices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(Indices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, m_skyBoxIndexBuffer.GetAddressOf()));

	});

	//-----------------END SKYBOX-----------------------//

	//-----------------Scene within a Scene-------------//
	//
	//auto createInnerSceneVSTask = loadVSTask.then([this](const std::vector<byte>& fileData)
	//{
	//	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateVertexShader(&fileData[0], fileData.size(), nullptr, &m_innerSceneVertexShader));
	//
	//	static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	//	{
	//		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	//		{ "UV", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	//		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	//	};
	//
	//	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), &fileData[0], fileData.size(), &m_innerSceneInputLayout));
	//});
	//
	//auto createInnerScenePSTask = loadPSTask.then([this](const std::vector<byte>& fileData)
	//{
	//	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &m_innerScenePixelShader));
	//
	//	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	//	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, &m_innerSceneConstantBuffer));
	//
	//});
	//
	//auto createInnerSceneTask = (createInnerScenePSTask && createInnerSceneVSTask).then([this]()
	//{
	//	Size outSize = m_deviceResources->GetOutputSize();
	//
	//	D3D11_TEXTURE2D_DESC TextureDesc;
	//	ZeroMemory(&TextureDesc, sizeof(TextureDesc));
	//	TextureDesc.MipLevels = 1;
	//	TextureDesc.Width = outSize.Width;
	//	TextureDesc.Height = outSize.Height;
	//	TextureDesc.ArraySize = 1;
	//	TextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	//	TextureDesc.SampleDesc.Count = 1;
	//	TextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	//	TextureDesc.CPUAccessFlags = 0;
	//	TextureDesc.MiscFlags = 0;
	//
	//	m_deviceResources->GetD3DDevice()->CreateTexture2D(&TextureDesc, NULL, &m_innerTargetTexture);
	//
	//	D3D11_RENDER_TARGET_VIEW_DESC renderView;
	//	renderView.Format = TextureDesc.Format;
	//	renderView.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	//	renderView.Texture2D.MipSlice = 0;
	//
	//	m_deviceResources->GetD3DDevice()->CreateRenderTargetView(m_innerTargetTexture.Get(), &renderView, &m_innerRenderTarget);
	//
	//	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResource;
	//	shaderResource.Format = TextureDesc.Format;
	//	shaderResource.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
	//	shaderResource.Texture2D.MostDetailedMip = 0;
	//	shaderResource.Texture2D.MipLevels = 1;
	//
	//	m_deviceResources->GetD3DDevice()->CreateShaderResourceView(m_innerTargetTexture.Get(), &shaderResource, &m_innerShaderResourceView);
	//
	//	static const VertexPositionUVNormal CubeUV[] =
	//	{
	//		//Front Face
	//		/*0*/{ XMFLOAT3(-5.0f, 1.0f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*1*/{ XMFLOAT3(-4.0f, 1.0f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*2*/{ XMFLOAT3(-5.0f, 0.0f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*3*/{ XMFLOAT3(-4.0f, 0.0f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//
	//		//Right Face
	//		/*4*/{ XMFLOAT3(-4.0f, 1.0f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*5*/{ XMFLOAT3(-4.0f, 1.0f, 0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*6*/{ XMFLOAT3(-4.0f, 0.0f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*7*/{ XMFLOAT3(-4.0f, 0.0f, 0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//
	//		//Back Face
	//		/*8*/{ XMFLOAT3(-4.0f, 1.0f, 0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*9*/{ XMFLOAT3(-5.0f, 1.0f, 0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*10*/{ XMFLOAT3(-4.0f, 0.0f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*11*/{ XMFLOAT3(-5.0f, 0.0f, 0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//
	//		//Left Face
	//		/*12*/{ XMFLOAT3(-5.0f, 1.0f, 0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*13*/{ XMFLOAT3(-5.0f, 1.0f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*14*/{ XMFLOAT3(-5.0f, 0.0f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*15*/{ XMFLOAT3(-5.0f, 0.0f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//
	//		//Top Face
	//		/*16*/{ XMFLOAT3(-5.0f, 1.0f, 0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*17*/{ XMFLOAT3(-4.0f, 1.0f, 0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*18*/{ XMFLOAT3(-5.0f, 1.0f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*19*/{ XMFLOAT3(-4.0f, 1.0f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//
	//		//Bottom Face
	//		/*20*/{ XMFLOAT3(-5.0f, 0.0f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*21*/{ XMFLOAT3(-4.0f, 0.0f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*22*/{ XMFLOAT3(-5.0f, 0.0f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*23*/{ XMFLOAT3(-4.0f, 0.0f, 0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//	};
	//
	//	static const unsigned short CubeUVIndices[] =
	//	{
	//		0,1,3,
	//		3,2,0,
	//
	//		4,5,7,
	//		7,6,4,
	//
	//		8,9,11,
	//		11,10,8,
	//
	//		12,13,15,
	//		15,14,12,
	//
	//		16,17,19,
	//		19,18,16,
	//
	//		20,21,23,
	//		23,22,20,
	//	};
	//
	//	//m_indexCount = ARRAYSIZE(cubeIndices);
	//	m_innerSceneIndexCount = ARRAYSIZE(CubeUVIndices);
	//
	//	D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
	//
	//	//pointing the memory to the cube verts
	//	//vertexBufferData.pSysMem = cubeVertices;
	//	vertexBufferData.pSysMem = CubeUV;
	//	vertexBufferData.SysMemPitch = 0;
	//	vertexBufferData.SysMemSlicePitch = 0;
	//
	//	//CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
	//	CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(CubeUV), D3D11_BIND_VERTEX_BUFFER);
	//	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_innerSceneVertexBuffer));
	//
	//	D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
	//
	//	//indexBufferData.pSysMem = cubeIndices;
	//	indexBufferData.pSysMem = CubeUVIndices;
	//	indexBufferData.SysMemPitch = 0;
	//	indexBufferData.SysMemSlicePitch = 0;
	//
	//	//CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);
	//	CD3D11_BUFFER_DESC indexBufferDesc(sizeof(CubeUVIndices), D3D11_BIND_INDEX_BUFFER);
	//	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_innerSceneIndexBuffer));
	//
	//
	//});

	//------------------END SCENE WITHIN A SCENE------------------//

	//---------------Stone Floor---------------------------//

	//auto loadStoneVSTask = DX::ReadDataAsync(L"LightingVertexShader.cso");
	//
	//auto createStoneVSTask = loadStoneVSTask.then([this](const std::vector<byte>& fileData)
	//{
	//	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateVertexShader(&fileData[0], fileData.size(), nullptr, &m_stoneVS));
	//
	//	static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	//	{
	//		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	//	};
	//	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), &fileData[0], fileData.size(), &m_stoneInput));
	//});
	//
	//auto loadStonePSTask = DX::ReadDataAsync(L"LightingPixelShader.cso");
	//
	//auto createStonePSTask = loadStonePSTask.then([this](const std::vector<byte>& fileData)
	//{
	//	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &m_stonePS));
	//
	//	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	//
	//	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, &m_stoneConstantBuffer));
	//});
	//
	//auto createStoneFloor = (createStoneVSTask && createStonePSTask).then([this]()
	//{
	//	static const VertexPositionUVNormal stoneFloor[] =
	//	{
	//		//Front Face
	//		/*0*/{ XMFLOAT3(-1.0f, 1.0f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*1*/{ XMFLOAT3(1.0f, 1.0f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*2*/{ XMFLOAT3(-1.0f, 0.0f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*3*/{ XMFLOAT3(1.0f, 0.0f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//
	//		//Right Face
	//		/*4*/{ XMFLOAT3(1.0f, 1.0f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*5*/{ XMFLOAT3(1.0f, 1.0f, 0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*6*/{ XMFLOAT3(1.0f, 0.0f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*7*/{ XMFLOAT3(1.0f, 0.0f, 0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//
	//		//Back Face
	//		/*8*/{ XMFLOAT3(1.0f, 1.0f, 0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*9*/{ XMFLOAT3(-1.0f, 1.0f, 0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*10*/{ XMFLOAT3(1.0f, 0.0f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*11*/{ XMFLOAT3(-1.0f, 0.0f, 0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//
	//		//Left Face
	//		/*12*/{ XMFLOAT3(-1.0f, 1.0f, 0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*13*/{ XMFLOAT3(-1.0f, 1.0f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*14*/{ XMFLOAT3(-1.0f, 0.0f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*15*/{ XMFLOAT3(-1.0f, 0.0f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//
	//		//Top Face
	//		/*16*/{ XMFLOAT3(-1.0f, 1.0f, 0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*17*/{ XMFLOAT3(1.0f, 1.0f, 0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*18*/{ XMFLOAT3(-1.0f, 1.0f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*19*/{ XMFLOAT3(1.0f, 1.0f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//
	//		//Bottom Face
	//		/*20*/{ XMFLOAT3(-1.0f, 0.0f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*21*/{ XMFLOAT3(1.0f, 0.0f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*22*/{ XMFLOAT3(-1.0f, 0.0f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//		/*23*/{ XMFLOAT3(1.0f, 0.0f, 0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	//	};
	//
	//	static const unsigned short groundIndices[] =
	//	{
	//		0,1,3,
	//		3,2,0,
	//
	//		4,5,7,
	//		7,6,4,
	//
	//		8,9,11,
	//		11,10,8,
	//
	//		12,13,15,
	//		15,14,12,
	//
	//		16,17,19,
	//		19,18,16,
	//
	//		20,21,23,
	//		23,22,20,
	//	};
	//
	//	D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
	//	vertexBufferData.pSysMem = stoneFloor;
	//	vertexBufferData.SysMemPitch = 0;
	//	vertexBufferData.SysMemSlicePitch = 0;
	//	CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(stoneFloor), D3D11_BIND_VERTEX_BUFFER);
	//	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_stoneVertexBuffer));
	//
	//
	//	D3D11_SAMPLER_DESC sampleDesc;
	//	ZeroMemory(&sampleDesc, sizeof(sampleDesc));
	//	sampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	//	sampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	//	sampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	//	sampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	//
	//	DX::ThrowIfFailed(CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets/GroundTexture.dds", NULL, m_stoneResourceView.GetAddressOf()));
	//
	//	m_stoneICount = ARRAYSIZE(groundIndices);
	//
	//	D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
	//	indexBufferData.pSysMem = groundIndices;
	//	indexBufferData.SysMemPitch = 0;
	//	indexBufferData.SysMemSlicePitch = 0;
	//	CD3D11_BUFFER_DESC indexBufferDesc(sizeof(groundIndices), D3D11_BIND_INDEX_BUFFER);
	//	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, m_stoneIndexBuffer.GetAddressOf()));
	//
	//});

	//---------------End Stone Floor-----------------------//

	auto createCubeTask = (createPSTask && createVSTask).then([this]()
	{
		static const VertexPositionUVNormal cubeUV[] =
		{
			//Front Face
			/*0*/{ XMFLOAT3(-0.5f, 1.0f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*1*/{ XMFLOAT3(0.5f, 1.0f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*2*/{ XMFLOAT3(-0.5f, 0.0f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*3*/{ XMFLOAT3(0.5f, 0.0f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },

			//Right Face
			/*4*/{ XMFLOAT3(0.5f, 1.0f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*5*/{ XMFLOAT3(0.5f, 1.0f, 0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*6*/{ XMFLOAT3(0.5f, 0.0f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*7*/{ XMFLOAT3(0.5f, 0.0f, 0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },

			//Back Face
			/*8*/{ XMFLOAT3(0.5f, 1.0f, 0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*9*/{ XMFLOAT3(-0.5f, 1.0f, 0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*10*/{ XMFLOAT3(0.5f, 0.0f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*11*/{ XMFLOAT3(-0.5f, 0.0f, 0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },

			//Left Face
			/*12*/{ XMFLOAT3(-0.5f, 1.0f, 0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*13*/{ XMFLOAT3(-0.5f, 1.0f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*14*/{ XMFLOAT3(-0.5f, 0.0f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*15*/{ XMFLOAT3(-0.5f, 0.0f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },

			//Top Face
			/*16*/{ XMFLOAT3(-0.5f, 1.0f, 0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*17*/{ XMFLOAT3(0.5f, 1.0f, 0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*18*/{ XMFLOAT3(-0.5f, 1.0f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*19*/{ XMFLOAT3(0.5f, 1.0f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },

			//Bottom Face
			/*20*/{ XMFLOAT3(-0.5f, 0.0f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*21*/{ XMFLOAT3(0.5f, 0.0f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*22*/{ XMFLOAT3(-0.5f, 0.0f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			/*23*/{ XMFLOAT3(0.5f, 0.0f, 0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = cubeUV;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeUV), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_vertexBuffer));

		static const unsigned short cubeIndices[] =
		{
			0,1,3,
			3,2,0,

			4,5,7,
			7,6,4,

			8,9,11,
			11,10,8,

			12,13,15,
			15,14,12,

			16,17,19,
			19,18,16,

			20,21,23,
			23,22,20,
		};

		m_indexCount = ARRAYSIZE(cubeIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = cubeIndices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_indexBuffer));
	});


	//start FLOOR
	bool loadFloor = loadObject("Assets/icyCastle.obj", m_floorVerticies, m_floorIndicies);

	D3D11_SUBRESOURCE_DATA floorVertBuffData = { 0 };

	floorVertBuffData.pSysMem = m_floorVerticies.data();
	floorVertBuffData.SysMemPitch = 0;
	floorVertBuffData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC floorVertBuffDesc(sizeof(VertexPositionUVNormal) * m_floorVerticies.size(), D3D11_BIND_VERTEX_BUFFER);
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&floorVertBuffDesc, &floorVertBuffData, &m_floorVertBuffer));

	D3D11_SUBRESOURCE_DATA floorIndexBuffData = { 0 };

	floorIndexBuffData.pSysMem = m_floorIndicies.data();
	floorIndexBuffData.SysMemPitch = 0;
	floorIndexBuffData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC floorIndexBuffDesc(sizeof(unsigned int) * m_floorIndicies.size(), D3D11_BIND_INDEX_BUFFER);
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&floorIndexBuffDesc, &floorIndexBuffData, &m_floorIndexBuffer));

	D3D11_SAMPLER_DESC floorTextureSampler;
	ZeroMemory(&floorTextureSampler, sizeof(floorTextureSampler));
	floorTextureSampler.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
	floorTextureSampler.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	floorTextureSampler.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	floorTextureSampler.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;

	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateSamplerState(&floorTextureSampler, &m_floorSampleState));
	DX::ThrowIfFailed(CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets/iceCastleTexture.dds", NULL, &m_floorResourceView));
	//END FLOOR

	// Once the cube is loaded, the object is ready to be rendered.
	createCubeTask.then([this]()
	{
		m_loadingComplete = true;
	});
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources(void)
{
	m_loadingComplete = false;
	m_vertexShader.Reset();
	m_inputLayout.Reset();
	m_pixelShader.Reset();
	m_constantBuffer.Reset();
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();

	//floor
	m_floorVertBuffer.Reset();
	m_floorConstantBuffer.Reset();


	//memory cleanup
	delete m_vp1;
	delete m_vp2;
	delete m_vp3;
}

#pragma warning(disable:4996)
bool loadObject(const char * path, std::vector <VertexPositionUVNormal> & outVertices, std::vector <unsigned int> & outIndicies)
{
	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
	std::vector<XMFLOAT3> tempVerts;
	std::vector<XMFLOAT3> tempUVs;
	std::vector<XMFLOAT3> tempNormals;

	FILE * file = fopen(path, "r");
	if (!file)
		return false;

	while (true)
	{
		char line[128];
		int endCheck = fscanf(file, "%s", line);
		if (endCheck == EOF)
			break;
		if (strcmp(line, "v") == 0)
		{
			XMFLOAT3 vertex;
			fscanf_s(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			tempVerts.push_back(vertex);
		}
		else if (strcmp(line, "vt") == 0)
		{
			XMFLOAT3 uv;
			fscanf_s(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = 1 - uv.y;
			tempUVs.push_back(uv);
		}
		else if (strcmp(line, "vn") == 0)
		{
			XMFLOAT3 norm;
			fscanf_s(file, "%f %f %f\n", &norm.x, &norm.y, &norm.z);
			tempNormals.push_back(norm);
		}
		else if (strcmp(line, "f") == 0)
		{
			std::string v1, v2, v3;
			UINT vIndex[3], uvIndex[3], normIndex[3];
			int check = fscanf_s(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vIndex[0], &uvIndex[0], &normIndex[0], &vIndex[1], &uvIndex[1], &normIndex[1], &vIndex[2], &uvIndex[2], &normIndex[2]);
			if (check != 9)
				return false;
			vertexIndices.push_back(vIndex[0]);
			vertexIndices.push_back(vIndex[1]);
			vertexIndices.push_back(vIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normIndex[0]);
			normalIndices.push_back(normIndex[1]);
			normalIndices.push_back(normIndex[2]);
		}
	}

	for (UINT i = 0; i < vertexIndices.size(); i++)
	{
		VertexPositionUVNormal temp;
		UINT vertInd = vertexIndices[i];
		temp.pos = tempVerts[vertInd - 1];

		UINT UVInd = uvIndices[i];
		temp.uv = tempUVs[UVInd - 1];

		UINT normInd = normalIndices[i];
		temp.normal = tempNormals[normInd - 1];

		outVertices.push_back(temp);
		outIndicies.push_back(i);
	}

	return true;
}