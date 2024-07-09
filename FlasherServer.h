
// FlasherServer.h : fichier d'en-tête principal de l'application PROJECT_NAME
//

#pragma once

#ifndef __AFXWIN_H__
	#error "incluez 'pch.h' avant d'inclure ce fichier pour PCH"
#endif

#include "resource.h"		// symboles principaux


// CFlasherServerApp :
// Consultez FlasherServer.cpp pour l'implémentation de cette classe
//

class CFlasherServerApp : public CWinApp
{
public:
	CFlasherServerApp();

// Substitutions
public:
	virtual BOOL InitInstance();

// Implémentation

	DECLARE_MESSAGE_MAP()
};

extern CFlasherServerApp theApp;
