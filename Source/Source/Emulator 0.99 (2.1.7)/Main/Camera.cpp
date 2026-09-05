#include "stdafx.h"
#include "Camera.h"
#include "Offset.h"
#include "Util.h"

CCamera gCamera;

CCamera::CCamera()
{
	this->m_Enable = 0;

	this->m_IsMove = 0;

	this->m_CursorX = 0;

	this->m_CursorY = 0;

	this->m_Zoom.MinPercent = 50.0;

	this->m_Zoom.MaxPercent = 130.0;

	this->m_Zoom.Precision = 2.0;

	this->m_Address.Zoom = (float*)0x005C67F1;

	this->m_Address.RotX = (float*)0x081B95A4;

	this->m_Address.RotY = (float*)0x005FBD74;

	this->m_Address.PosZ = (float*)0x005FA5B8;

	this->m_Address.ClipX[0] = (float*)0x00593085;

	this->m_Address.ClipX[1] = (float*)0x00593099;

	this->m_Address.ClipY[0] = (float*)0x005930A7;

	this->m_Address.ClipY[1] = (float*)0x005930B5;

	this->m_Address.ClipZ = (float*)0x005FBCDC;

	this->m_Address.ClipGL = (float*)0x006105DC;

	this->SetCurrentValue();

	this->m_Default.IsLoad = 0;

	MemorySet(0x005B2214,0x90,6);

	SetCompleteHook(0xE9,0x005B2280,&this->RotateDamageAngle);
}

void CCamera::Toggle() // OK
{
	if(*(DWORD*)(MAIN_SCREEN_STATE) == 5)
	{
		pDrawMessage(pGetTextLine((805+this->m_Enable)),1);

		this->m_Enable ^= 1;

		if(((this->m_Default.IsLoad==0)?(this->m_Default.IsLoad++):this->m_Default.IsLoad) == 0)
		{
			this->m_Default.Zoom = (*this->m_Address.Zoom);

			this->m_Default.RotX = (*this->m_Address.RotX);

			this->m_Default.RotY = (*this->m_Address.RotY);

			this->m_Default.PosZ = (*this->m_Address.PosZ);

			this->m_Default.ClipX[0] = (*this->m_Address.ClipX[0]);

			this->m_Default.ClipX[1] = (*this->m_Address.ClipX[1]);

			this->m_Default.ClipY[0] = (*this->m_Address.ClipY[0]);

			this->m_Default.ClipY[1] = (*this->m_Address.ClipY[1]);

			this->m_Default.ClipZ = (*this->m_Address.ClipZ);

			this->m_Default.ClipGL = (*this->m_Address.ClipGL);
		}
	}
}

void CCamera::Restore() // OK
{
	if(this->m_Enable != 0 && *(DWORD*)(MAIN_SCREEN_STATE) == 5)
	{
		this->SetDefaultValue();
	}
}

void CCamera::SetIsMove(BOOL IsMove) // OK
{
	if(this->m_Enable != 0 && *(DWORD*)(MAIN_SCREEN_STATE) == 5)
	{
		this->m_IsMove = IsMove;
	}
}

void CCamera::SetCursorX(LONG CursorX) // OK
{
	if(this->m_Enable != 0 && *(DWORD*)(MAIN_SCREEN_STATE) == 5)
	{
		this->m_CursorX = CursorX;
	}
}

void CCamera::SetCursorY(LONG CursorY) // OK
{
	if(this->m_Enable != 0 && *(DWORD*)(MAIN_SCREEN_STATE) == 5)
	{
		this->m_CursorY = CursorY;
	}
}

void CCamera::Zoom(MOUSEHOOKSTRUCTEX* lpMouse) // OK
{
	if(this->m_Enable == 0 || this->m_IsMove != 0 || *(DWORD*)(MAIN_SCREEN_STATE) != 5)
	{
		return;
	}

	this->m_Zoom.MinLimit = (this->m_Default.Zoom/100)*this->m_Zoom.MinPercent;

	this->m_Zoom.MaxLimit = (this->m_Default.Zoom/100)*this->m_Zoom.MaxPercent;

	if(((int)lpMouse->mouseData) > 0)
	{
		if((*this->m_Address.Zoom) >= this->m_Zoom.MinLimit)
		{
			SetFloat((DWORD)this->m_Address.Zoom,((*this->m_Address.Zoom)-this->m_Zoom.Precision));
		}
	}

	if(((int)lpMouse->mouseData) < 0)
	{
		if((*this->m_Address.Zoom) <= this->m_Zoom.MaxLimit)
		{
			SetFloat((DWORD)this->m_Address.Zoom,((*this->m_Address.Zoom)+this->m_Zoom.Precision));
		}
	}

	this->SetCurrentValue();
}

void CCamera::Move(MOUSEHOOKSTRUCTEX* lpMouse) // OK
{
	if(this->m_Enable == 0 || this->m_IsMove == 0 || *(DWORD*)(MAIN_SCREEN_STATE) != 5)
	{
		return;
	}

	if(this->m_CursorX < lpMouse->pt.x)
	{
		if((*this->m_Address.RotX) > 309.0f)
		{
			SetFloat((DWORD)this->m_Address.RotX,-45.0f);
		}
		else
		{
			SetFloat((DWORD)this->m_Address.RotX,((*this->m_Address.RotX)+6.0f));
		}
	}

	if(this->m_CursorX > lpMouse->pt.x)
	{
		if((*this->m_Address.RotX) < -417.0f)
		{
			SetFloat((DWORD)this->m_Address.RotX,-45.0f);
		}
		else
		{
			SetFloat((DWORD)this->m_Address.RotX,((*this->m_Address.RotX)-6.0f));
		}
	}

	if(this->m_CursorY < lpMouse->pt.y)
	{
		if((*this->m_Address.RotY) > 30.0f)
		{
			SetFloat((DWORD)this->m_Address.RotY,((*this->m_Address.RotY)-2.420f));
			SetFloat((DWORD)this->m_Address.PosZ,((*this->m_Address.PosZ)-44.0f));
		}
	}

	if(this->m_CursorY > lpMouse->pt.y)
	{
		if((*this->m_Address.RotY) < 90.0f)
		{
			SetFloat((DWORD)this->m_Address.RotY,((*this->m_Address.RotY)+2.420f));
			SetFloat((DWORD)this->m_Address.PosZ,((*this->m_Address.PosZ)+44.0f));
		}
	}

	this->m_CursorX = lpMouse->pt.x;

	this->m_CursorY = lpMouse->pt.y;

	this->SetCurrentValue();
}

void CCamera::SetCurrentValue() // OK
{
	SetFloat((DWORD)this->m_Address.ClipX[0],(1272+(abs(*this->m_Address.PosZ-150)*3)+1000));

	SetFloat((DWORD)this->m_Address.ClipX[1],(1272+(abs(*this->m_Address.PosZ-150)*3)+1000));

	SetFloat((DWORD)this->m_Address.ClipY[0],(-672-(abs(*this->m_Address.PosZ-150)*3)-3000));

	SetFloat((DWORD)this->m_Address.ClipY[1],(-672-(abs(*this->m_Address.PosZ-150)*3)-3000));

	SetFloat((DWORD)this->m_Address.ClipZ,(1190+(abs(*this->m_Address.PosZ-150)*3)+3000));

	SetFloat((DWORD)this->m_Address.ClipGL,(2000+(abs(*this->m_Address.PosZ-150)*3)+1000));
}

void CCamera::SetDefaultValue() // OK
{
	if (this->m_Default.IsLoad != 0)
	{
		SetFloat((DWORD)this->m_Address.Zoom,this->m_Default.Zoom);

		SetFloat((DWORD)this->m_Address.RotX,this->m_Default.RotX);

		SetFloat((DWORD)this->m_Address.RotY,this->m_Default.RotY);

		SetFloat((DWORD)this->m_Address.PosZ,this->m_Default.PosZ);

		SetFloat((DWORD)this->m_Address.ClipX[0],this->m_Default.ClipX[0]);

		SetFloat((DWORD)this->m_Address.ClipX[1],this->m_Default.ClipX[1]);

		SetFloat((DWORD)this->m_Address.ClipY[0],this->m_Default.ClipY[0]);

		SetFloat((DWORD)this->m_Address.ClipY[1],this->m_Default.ClipY[1]);

		SetFloat((DWORD)this->m_Address.ClipZ,this->m_Default.ClipZ);

		SetFloat((DWORD)this->m_Address.ClipGL,this->m_Default.ClipGL);
	}

	pDrawMessage(pGetTextLine(807),1);
}

void CCamera::CalcDamageAngle(float& X,float& Y,float D) // OK
{
	const float DamageAngle = 0.01745329f;

	float AngleX = cos(DamageAngle*(*gCamera.m_Address.RotX));
	float AngleY = sin(DamageAngle*(*gCamera.m_Address.RotX));

	X += D/0.7071067f*AngleX/2;
	Y -= D/0.7071067f*AngleY/2;
}

void __declspec(naked) CCamera::RotateDamageAngle() // OK
{
	static DWORD RotateDamageAngleAddress1 = 0x005B2298;

	_asm
	{
		Add Esp,0x1C
		Lea Eax,[Ebp-0x8]
		Lea Ecx,[Ebp-0xC]
		Push Dword Ptr[Ebp+0x8]
		Push Eax
		Push Ecx
		Call CalcDamageAngle
		Add Esp,0x0C
		Inc Esi
		Cmp Esi,Ebx
		Jmp[RotateDamageAngleAddress1]
	}
}