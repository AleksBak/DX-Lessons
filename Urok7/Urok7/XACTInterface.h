#pragma once

#include <xact3.h>

enum XACT_INTERFACE_STATE {
	XACT_INTERFACE_STATE_NULL,
	XACT_INTERFACE_STATE_PLAYING,
	XACT_INTERFACE_STATE_WAITING
};

class CXACTInterface
{
	struct AUDIO_STATE
	{
		IXACT3Engine* pEngine;			// ������� ��������� XACT
		IXACT3WaveBank* pWaveBank;		// ���� "����" (���������)
		IXACT3SoundBank* pSoundBank;	// ���� ������ (�����������)
		XACTINDEX iCurrentCue;

		VOID* pbWaveBank;				// ����� ����� � ������ � ������� ����� ����
		VOID* pbSoundBank;				// ��������� �� ������ �����������
	} m_audioState;

	XACT_INTERFACE_STATE m_Flag;		// ���� ��������� �������������
	BOOL m_bLoop;						// ����-�������� ���������������
	XACT_NOTIFICATION_DESCRIPTION xactCueStopDesc;	// ���������-�������� ����������� XACT
	
	bool DoesCommandLineContainAuditionSwitch();
public:
	CXACTInterface(void) {
		ZeroMemory(&m_audioState, sizeof(AUDIO_STATE));
		ZeroMemory(&xactCueStopDesc, sizeof(XACT_NOTIFICATION_DESCRIPTION));
		m_Flag = XACT_INTERFACE_STATE_NULL; m_bLoop = FALSE;
	}
	~CXACTInterface(void);

	HRESULT LoadBank(LPCWSTR strWaveBank, LPCWSTR strSoundBank);
	HRESULT PlayCue(PCSTR strCueName, BOOL bLoop = FALSE);
	HRESULT Stop()
	{
		if (m_Flag == XACT_INTERFACE_STATE_PLAYING) { 
			m_audioState.pSoundBank->Stop(m_audioState.iCurrentCue, 0);
			return S_OK;
		} else return E_FAIL;
	};
	XACT_INTERFACE_STATE DoWork();
};

