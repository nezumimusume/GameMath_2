/*!
 *@brief	�`��G���W���B
 */

#include "tkEngine/tkEnginePreCompile.h"
#include "tkEngine/graphics/tkGraphicsEngine.h"
#include "tkEngine/graphics/tkPresetRenderState.h"

namespace tkEngine{
	CGraphicsEngine::CGraphicsEngine()
	{
	}
	CGraphicsEngine::~CGraphicsEngine()
	{
	}
	void CGraphicsEngine::Release()
	{

#if BUILD_LEVEL != BUILD_LEVEL_MASTER
		if (m_userAnnoation) {
			m_userAnnoation->Release();
			m_userAnnoation = nullptr;
		}
#endif
		if (m_backBufferRT) {
			m_backBufferRT->Release();
			m_backBufferRT = nullptr;
		}
		if (m_pImmediateContext) {
			m_pImmediateContext->ClearState();
			m_pImmediateContext = nullptr;
		}
		m_mainRenderTarget.Release();
		
		if (m_pSwapChain) {
			m_pSwapChain->Release();
			m_pSwapChain = nullptr;
		}
		if (m_pImmediateContext) {
			m_pImmediateContext->Release();
			m_pImmediateContext = nullptr;
		}
		if (m_pd3dDevice) {
			m_pd3dDevice->Release();
			m_pd3dDevice = nullptr;
		}
	}
	
	bool CGraphicsEngine::InitD3DDeviceAndSwapChain(HWND hwnd, const SInitParam& initParam)
	{
		UINT createDeviceFlags = 0;
#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		D3D_DRIVER_TYPE driverTypes[] =
		{
			D3D_DRIVER_TYPE_HARDWARE,
			D3D_DRIVER_TYPE_WARP,
			D3D_DRIVER_TYPE_REFERENCE,
		};

		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
		};
		UINT numFeatureLevels = ARRAYSIZE(featureLevels);

		m_frameBufferWidth = initParam.frameBufferWidth;
		m_frameBufferHeight = initParam.frameBufferHeight;
		m_2dSpaceScreenWidth = initParam.screenWidth2D;
		m_2dSpaceScreenHeight = initParam.screenHeight2D;
		//�X���b�v�`�F�C�����쐬�B
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 1;							//�X���b�v�`�F�C���̃o�b�t�@���B�ʏ�͂P�B
		sd.BufferDesc.Width = m_frameBufferWidth;	//�t���[���o�b�t�@�̕��B
		sd.BufferDesc.Height = m_frameBufferHeight;	//�t���[���o�b�t�@�̍����B
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	//�t���[���o�b�t�@�̃t�H�[�}�b�g�BR8G8B8A8��32bit�B
		sd.BufferDesc.RefreshRate.Numerator = 60;			//���j�^�̃��t���b�V�����[�g�B(�o�b�N�o�b�t�@�ƃt�����g�o�b�t�@�����ւ���^�C�~���O�ƂȂ�B)
		sd.BufferDesc.RefreshRate.Denominator = 1;			//�Q�ɂ�����30fps�ɂȂ�B���ƂŎ����B
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//�T�[�t�F�X�܂��̓��\�[�X���o�̓����_�[ �^�[�Q�b�g�Ƃ��Ďg�p���܂��B
		sd.OutputWindow = hwnd;
		sd.SampleDesc.Count = 1;							//�s�N�Z���P�ʂ̃}���`�T���v�����O�̐��BMSAA�͂Ȃ��B
		sd.SampleDesc.Quality = 0;							//MSAA�Ȃ��B
		sd.Windowed = TRUE;

		//���ׂẴh���C�o�^�C�v�ŃX���b�v�`�F�C���̍쐬�������B
		HRESULT hr = E_FAIL;
		for (auto driverType : driverTypes) {
			m_driverType = driverType;
			hr = D3D11CreateDeviceAndSwapChain(NULL, m_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
				D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pd3dDevice, &m_featureLevel, &m_pImmediateContext);
			if (SUCCEEDED(hr)) {
				//�X���b�v�`�F�C�����쐬�ł����̂Ń��[�v�𔲂���B
				break;
			}
		}
		if (FAILED(hr)) {
			//�X���b�v�`�F�C�����쐬�ł��Ȃ������B
			return false;
		}
		return true;
	}
	
	bool CGraphicsEngine::InitBackBuffer()
	{
		//�������ݐ�ɂȂ郌���_�����O�^�[�Q�b�g���쐬�B
		ID3D11Texture2D* pBackBuffer = NULL;
		HRESULT hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		if (FAILED(hr)) {
			return false;
		}
		hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_backBufferRT);
		if (FAILED(hr)) {
			return false;
		}
		return true;
	}
	bool CGraphicsEngine::InitMainRenderTarget()
	{
		//MSAA�̐ݒ������B	
		ZeroMemory(&m_mainRenderTargetMSAADesc, sizeof(m_mainRenderTargetMSAADesc));
		m_mainRenderTargetMSAADesc.Count = 1;
		m_mainRenderTargetMSAADesc.Quality = 0;
		//�ō���MSAAx4
		for (int i = 1; i <= 4; i <<= 1)
		{
			UINT Quality;
			if (SUCCEEDED(m_pd3dDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_D32_FLOAT, i, &Quality)))
			{
				if (0 < Quality)
				{
					m_mainRenderTargetMSAADesc.Count = i;
					m_mainRenderTargetMSAADesc.Quality = Quality - 1;
				}
			}
		}
		bool ret = m_mainRenderTarget.Create(
			m_frameBufferWidth,
			m_frameBufferHeight,
			1,
			1,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_D32_FLOAT,
			m_mainRenderTargetMSAADesc
		);
		
		if (!ret) {
			//�쐬���s
			return false;
		}
		return true;
	}
	bool CGraphicsEngine::Init(HWND hwnd, const SInitParam& initParam)
	{
		//D3D�f�o�C�X�ƃX���b�v�`�F�C���̍쐬�B
		if (!InitD3DDeviceAndSwapChain(hwnd, initParam)) {
			return false;
		}
		//�o�b�N�o�b�t�@�̍쐬�B
		if (!InitBackBuffer()) {
			return false;
		}
		//���C�������_�����O�^�[�Q�b�g�̏������B
		if (!InitMainRenderTarget()) {
			return false;
		}
		//�����_�����O�R���e�L�X�g�̏������B
		m_renderContext.Init(m_pImmediateContext);
		CRenderTarget* renderTargets[] = {
			&m_mainRenderTarget
		};
		m_renderContext.OMSetRenderTargets(1, renderTargets);
		//�r���[�|�[�g��ݒ�B
		m_renderContext.RSSetViewport(0.0f, 0.0f, (FLOAT)m_frameBufferWidth, (FLOAT)m_frameBufferHeight);
		//PreRender�̏������B
		m_preRender.Create(initParam.graphicsConfing);
		//PostEffect�̏������B
		m_postEffect.Create(initParam.graphicsConfing);
		//���C�g�Ǘ��҂̏������B
		m_lightManager.Init();

		//�R�s�[�p�̃V�F�[�_�[�����[�h�B
		m_copyVS.Load("shader/copy.fx", "VSMain", CShader::EnType::VS);
		m_copyPS.Load("shader/copy.fx", "PSMain", CShader::EnType::PS);

		//�t�H���g�p�̃f�[�^�̏������B
		m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(m_pImmediateContext);
		m_spriteFont = std::make_unique<DirectX::SpriteFont>(m_pd3dDevice, L"font/myfile.spritefont");

		//2D�J�����̏������B
		m_2dCamera.SetTarget(CVector3::Zero);
		m_2dCamera.SetPosition({0.0f, 0.0f, -10.0f});
		m_2dCamera.SetUpdateProjMatrixFunc(CCamera::enUpdateProjMatrixFunc_Ortho);
		m_2dCamera.SetNear(0.1f);
		m_2dCamera.SetFar(1000.0f);
		m_2dCamera.Update();
		//�e�탌���_�����O�X�e�[�g������������B
		AlphaBlendState::Init(*this);
		DepthStencilState::Init(*this);
		RasterizerState::Init(*this);
		
		//�G�t�F�N�g�G���W���̏������B
		m_effectEngine.Init();
#if BUILD_LEVEL != BUILD_LEVEL_MASTER
		m_pImmediateContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&m_userAnnoation);
#endif
		return true;

	}
	void CGraphicsEngine::BeginRender()
	{
	}
	/*!
	*@brief	�|�X�g�G�t�F�N�g�̏��������������Ƃ��ɌĂ΂�鏈���B
	*@details
	* �Q�[���w�ł͎g�p���Ȃ��悤�ɁB
	*/
	void CGraphicsEngine::EndPostEffect(CRenderContext& rc)
	{
		//�o�b�N�o�b�t�@�Ƀ��C�������_�����O�^�[�Q�b�g�̓��e���R�s�[�B
		ID3D11Texture2D* pBackBuffer = NULL;
		m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		ID3D11RenderTargetView* rts[] = {
			m_backBufferRT
		};
		m_pImmediateContext->OMSetRenderTargets(1, rts, nullptr);
		rc.VSSetShader(m_copyVS);
		rc.PSSetShader(m_copyPS);
		//���̓��C�A�E�g��ݒ�B
		rc.IASetInputLayout(m_copyVS.GetInputLayout());
		rc.PSSetShaderResource(0, m_postEffect.GetFinalRenderTarget().GetRenderTargetSRV());
		rc.RSSetState(RasterizerState::spriteRender);
		//�|�X�g�G�t�F�N�g�̃t���X�N���[���`��̋@�\���g���B
		m_postEffect.DrawFullScreenQuad(rc);
		pBackBuffer->Release();
		rc.PSUnsetShaderResource(0);
	}
	void CGraphicsEngine::EndRender()
	{
		m_lightManager.EndRender(m_renderContext);
		
		//�t���[�b�V��
		m_pSwapChain->Present(1, 0);
	}
}