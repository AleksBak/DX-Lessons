#define _WIN32_DCOM
#include <windows.h>
#include <commdlg.h>
#include <xact3.h>
#include <shellapi.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )

#include "XACTInterface.h"

void WINAPI XACTNotificationCallback( const XACT_NOTIFICATION* pNotification );

CXACTInterface::~CXACTInterface(void)
{
	// Освобождение всех занятых ресурсов
    if( m_audioState.pEngine )
    {
        m_audioState.pEngine->ShutDown();
        m_audioState.pEngine->Release();
    }

    if( m_audioState.pbSoundBank )
        delete[] m_audioState.pbSoundBank;
    m_audioState.pbSoundBank = NULL;

    // После pEngine->ShutDown() можно спокойно удалить файл из памяти
    if( m_audioState.pbWaveBank )
        UnmapViewOfFile( m_audioState.pbWaveBank );
    m_audioState.pbWaveBank = NULL;

    CoUninitialize();
}

bool CXACTInterface::DoesCommandLineContainAuditionSwitch()
{
	// Даже не разбирался, что происходит в этой функции. Микрософт знает, что делает
    const WCHAR* strAuditioning = L"-audition"; size_t nArgLen; int nNumArgs;
    LPWSTR* pstrArgList = CommandLineToArgvW( GetCommandLine(), &nNumArgs );
    for( int iArg = 1; iArg < nNumArgs; iArg++ )
    {
        StringCchLength( pstrArgList[iArg], 256, &nArgLen );
        if( _wcsnicmp( pstrArgList[iArg], strAuditioning, nArgLen ) == 0 && nArgLen == 9 )
            return true;
    }
    LocalFree( pstrArgList );
    return false;
}

HRESULT CXACTInterface::LoadBank(LPCWSTR strWaveBank, LPCWSTR strSoundBank)
{
    HRESULT hr;
    HANDLE hFile;
    DWORD dwFileSize;
    DWORD dwBytesRead;
    HANDLE hMapFile;

	if( m_audioState.pEngine ) return E_FAIL;

    // Очистить структуру
    ZeroMemory( &m_audioState, sizeof( AUDIO_STATE ) );

    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );  // COINIT_APARTMENTTHREADED тоже можно
    if( SUCCEEDED( hr ) )
    {
        // Переключить прослушивание в режим командной строки. Изменить при необходимости
        bool bAuditionMode = DoesCommandLineContainAuditionSwitch();
        bool bDebugMode = false;

        DWORD dwCreationFlags = 0;
        if( bAuditionMode ) dwCreationFlags |= XACT_FLAG_API_AUDITION_MODE;
        if( bDebugMode ) dwCreationFlags |= XACT_FLAG_API_DEBUG_MODE;

        hr = XACT3CreateEngine( dwCreationFlags, &m_audioState.pEngine );
    }
    if( FAILED( hr ) || m_audioState.pEngine == NULL )
        return E_FAIL;

    // Инициализация и создание главного движка XACT
    XACT_RUNTIME_PARAMETERS xrParams = {0};
    xrParams.lookAheadTime = XACT_ENGINE_LOOKAHEAD_DEFAULT;
    xrParams.fnNotificationCallback = XACTNotificationCallback;
    hr = m_audioState.pEngine->Initialize( &xrParams );
    if( FAILED( hr ) )
        return hr;

	// Включение уведомлений об окончании проигрывания музыки
	xactCueStopDesc.type               = XACTNOTIFICATIONTYPE_CUESTOP;
	xactCueStopDesc.cueIndex           = XACTINDEX_INVALID; // От любого аудио
	xactCueStopDesc.pvContext          = CreateEvent( NULL, FALSE, FALSE, NULL );
    xactCueStopDesc.flags			   = XACT_FLAG_NOTIFICATION_PERSIST; // Получать уведомления постоянно, а не один раз
    m_audioState.pEngine->RegisterNotification( &xactCueStopDesc );

    // Сoздать отраженный в памяти файл с "банком волн" XACT
    // Файлы в памяти работают намного быстрее, чем на жестком диске
    hr = E_FAIL;
    hFile = CreateFile( strWaveBank, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if( hFile != INVALID_HANDLE_VALUE )
    {
        dwFileSize = GetFileSize( hFile, NULL );
        if( dwFileSize != -1 )
        {
            hMapFile = CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, dwFileSize, NULL );
            if( hMapFile )
            {
                m_audioState.pbWaveBank = MapViewOfFile( hMapFile, FILE_MAP_READ, 0, 0, 0 );
                if( m_audioState.pbWaveBank )
                {
                    hr = m_audioState.pEngine->CreateInMemoryWaveBank( m_audioState.pbWaveBank, dwFileSize, 0,
                                                                       0, &m_audioState.pWaveBank );
                }
                CloseHandle( hMapFile ); // pbWaveBank содержит ссылку на объект файла, поэтому удалим ненужный хэндл
            }
        }
        CloseHandle( hFile ); // pbWaveBank содержит ссылку на объект файла, поэтому удалим ненужный хэндл
    }
    if( FAILED( hr ) )
        return E_FAIL;

    // Загрузить и зарегистрировать "банк звуков" в XACT. Отраженные в память файлы не используются, потому что
    // память должна быть для чтения и записи, а рабочий набор банка звуков маленький.
    hr = E_FAIL; // assume failure
    hFile = CreateFile( strSoundBank, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if( hFile != INVALID_HANDLE_VALUE )
    {
        dwFileSize = GetFileSize( hFile, NULL );
        if( dwFileSize != -1 )
        {
            // Выделение места в памяти для загрузки банка звуков. Память освобождается в деструкторе.
            m_audioState.pbSoundBank = new BYTE[dwFileSize];
            if( m_audioState.pbSoundBank )
            {
                if( 0 != ReadFile( hFile, m_audioState.pbSoundBank, dwFileSize, &dwBytesRead, NULL ) )
                {
                    hr = m_audioState.pEngine->CreateSoundBank( m_audioState.pbSoundBank, dwFileSize, 0,
                                                                0, &m_audioState.pSoundBank );
                }
            }
        }
        CloseHandle( hFile );
    }
    if( FAILED( hr ) )
        return E_FAIL;

	// Укажем, что проигрыватель находится в состоянии ожидания.
	m_Flag = XACT_INTERFACE_STATE_WAITING;

    return S_OK;
}

HRESULT CXACTInterface::PlayCue(PCSTR strCueName, BOOL bLoop)
{
	if (m_audioState.pEngine == NULL) return E_FAIL;

	// Воспроизведение в цикле?
	m_bLoop = bLoop;
	// Номер аудиозаписи для прослушивания
    m_audioState.iCurrentCue = m_audioState.pSoundBank->GetCueIndex( strCueName );
	// Укажем статус проигрывателя
	m_Flag = XACT_INTERFACE_STATE_PLAYING;
	// Команда на воспроизведение
	return m_audioState.pSoundBank->Play( m_audioState.iCurrentCue, 0, 0, NULL );
}

XACT_INTERFACE_STATE CXACTInterface::DoWork()
{
	m_audioState.pEngine->DoWork();
	
	// Проверяем, не установлено ли событие окончания воспроизведения
	if (WAIT_OBJECT_0 == WaitForSingleObject( (HANDLE) xactCueStopDesc.pvContext, 0 )) {
		// Если воспроизведение цикличное, идем дальше, если нет - переходим в режим ожидания
		if (m_bLoop) {
			XACTINDEX cnt;
			// Переходим к следующей композиции в списке
			m_audioState.pSoundBank->GetNumCues(&cnt);
			m_audioState.iCurrentCue = (m_audioState.iCurrentCue+1) % cnt;
			// Команда на воспроизведение
			m_audioState.pSoundBank->Play( m_audioState.iCurrentCue, 0, 0, NULL );
		} else
			m_Flag = XACT_INTERFACE_STATE_WAITING;
	}
	return m_Flag;
}

void WINAPI XACTNotificationCallback( const XACT_NOTIFICATION* pNotification )
{
	// Устанавливаем событие окончания воспроизведения
    if( ( NULL != pNotification ) && ( NULL != pNotification->pvContext ) )
    {
        SetEvent( (HANDLE) pNotification->pvContext );
    }
}