#include "LineManager.h"
#include "Line.h"
#include "ScrollManager.h"
#include "KeyManager.h"

void LineManager::Init()
{
	ZeroMemory(LinePoint, sizeof(FPOINT) * END);
	CurrentLineType = ELineType::Normal;
	bStraight = false;
}

void LineManager::AddLine(float InX, float InY)
{	
	// 첫 피킹
	if (!LinePoint[LEFT].x && !LinePoint[LEFT].y)
	{
		LinePoint[LEFT].x = InX;
		LinePoint[LEFT].y = InY;
	}
	// 두번 째 피킹
	else
	{
		float LeftX = LinePoint[LEFT].x;
		float LeftY = LinePoint[LEFT].y;

		// Wall은 수직만 됨 보정함
		if (CurrentLineType == ELineType::Wall)
		{
			if (LinePoint[LEFT].y < InY)
			{
				LinePoint[RIGHT].y = LinePoint[LEFT].y;

				LinePoint[LEFT].y = InY;
			}
			else
				LinePoint[RIGHT].y = InY;
			
			LinePoint[RIGHT].x = LinePoint[LEFT].x;
		}
		else
		{
			if (LinePoint[LEFT].x > InX)
			{
				LinePoint[RIGHT].x = LinePoint[LEFT].x;
				LinePoint[RIGHT].y = LinePoint[LEFT].y;

				LinePoint[LEFT].x = InX;
				LinePoint[LEFT].y = InY;
			}
			else
			{
				LinePoint[RIGHT].x = InX;
				LinePoint[RIGHT].y = InY;
			}

			// 직선 보정
			if (bStraight)
			{
				LinePoint[LEFT].y = LeftY;
				LinePoint[RIGHT].y = LeftY;
			}
		}

		LineList[(int)CurrentLineType].push_back(new Line(LinePoint[LEFT], LinePoint[RIGHT], CurrentLineType));

		if (LeftX > InX)
		{
			LinePoint[LEFT].x = InX;
			LinePoint[LEFT].y = InY;
		}
		else
		{
			LinePoint[LEFT].x = LinePoint[RIGHT].x;
			LinePoint[LEFT].y = LinePoint[RIGHT].y;
		}
	}
}

void LineManager::ResetLinePoint()
{
	LinePoint[LEFT].x = 0.f;
	LinePoint[LEFT].y = 0.f;
	LinePoint[RIGHT].x = 0.f;
	LinePoint[RIGHT].y = 0.f;
}

bool LineManager::CollisionLine(FPOINT InPos, FLineResult& OutResult, float tolerance, bool IsDown)
{
	if (LineList[(int)ELineType::Normal].empty() && LineList[(int)ELineType::DownLine].empty())
		return false;

	// 직선의 방정식으로 라인을 태우자.
	// 캐릭터의 X 값으로 높이를 알 수 있다.
	// 두점을 사용해 직선의 방정식을 구하자.

	const float HalfTolerance = tolerance * 0.5f;
	const float ObjectBottom = InPos.y + HalfTolerance;

	float NormalY = 0.f;
	float DownY = 0.f;
	Line* NormalTarget = nullptr;
	Line* DownTarget = nullptr;

	for (int i = 0; i < (int)ELineType::DownLine + 1; ++i)
	{
		if (IsDown && i == (int)ELineType::DownLine)
			break;

		for (auto& iter : LineList[i])
		{
			// X Offset
			// 해당 라인 안에 있다면
			if (InPos.x >= iter->GetLine().LeftPoint.x && InPos.x <= iter->GetLine().RightPoint.x)
			{
				float x1 = iter->GetLine().LeftPoint.x;
				float y1 = iter->GetLine().LeftPoint.y;

				float x2 = iter->GetLine().RightPoint.x;
				float y2 = iter->GetLine().RightPoint.y;

				float dY = ((y2 - y1) / (x2 - x1)) * (InPos.x - x1) + y1;

				if (ObjectBottom >= dY - HalfTolerance && ObjectBottom <= dY + HalfTolerance)
				{
					if (i == (int)ELineType::Normal)
					{
						NormalY = dY - HalfTolerance;
						NormalTarget = iter;
					}
						
					else
					{
						DownY = dY - HalfTolerance;
						DownTarget = iter;
					}
				}

				/*float dy = abs(InPos.y - y);
				if (dy <= tolerance)
				{
					OutResult.OutPos.y = y - tolerance;
					OutResult.LineType = iter->GetLineType();
					Target = iter;
				}*/
			}
		}
	}
	
	if (NormalTarget == nullptr && DownTarget == nullptr)
		return false;

	// 예외
	if (NormalTarget == nullptr || DownTarget == nullptr)
	{		
		OutResult.OutPos.y = DownTarget == nullptr ? NormalY : DownY;
		OutResult.LineType = DownTarget == nullptr ? NormalTarget->GetLineType() : DownTarget->GetLineType();

		return true;
	}

	//두개의 라인 중 작은거
	const float NormalDY = abs(ObjectBottom - (NormalY + HalfTolerance));
	const float DownDY = abs(ObjectBottom - (DownY + HalfTolerance));
	if (NormalDY >= DownDY)
	{
		OutResult.OutPos.y = DownY;
		OutResult.LineType =DownTarget->GetLineType();
	}
	else
	{
		OutResult.OutPos.y = NormalY;
		OutResult.LineType = NormalTarget->GetLineType();
	}

	return true;
} 

bool LineManager::CollisionWallLine(FPOINT InPos, FLineResult& OutResult, FPOINT InSize)
{
	if (LineList[(int)ELineType::Wall].empty())
		return false;

	//Wall은 무조건 수직

	const FPOINT HalfSize = { InSize.x * 0.5f, InSize.y * 0.5f };

	// Object Rect
	RECT ObjectRC;
	ObjectRC.left = InPos.x - HalfSize.x;
	ObjectRC.right = InPos.x + HalfSize.x;
	ObjectRC.top = InPos.y - HalfSize.y;
	ObjectRC.bottom = InPos.y + HalfSize.y;

	float XMinLength = FLT_MAX;
	Line* Target = nullptr;

	for (auto& iter : LineList[(int)ELineType::Wall])
	{
		// 해당 수직안에 있다.		
		if (ObjectRC.bottom >= min(iter->GetLine().RightPoint.y, iter->GetLine().LeftPoint.y) && ObjectRC.top <= max(iter->GetLine().RightPoint.y, iter->GetLine().LeftPoint.y))
		{
			// 양쪽에 라인이 있을 때 
			// 수직이기 때문에 x값으로 비교하자.			
			if (XMinLength > abs(InPos.x - iter->GetLine().LeftPoint.x))
			{
				XMinLength = abs(InPos.x - iter->GetLine().LeftPoint.x);

				if (ObjectRC.right >= iter->GetLine().LeftPoint.x && ObjectRC.left < iter->GetLine().LeftPoint.x)
				{
					if (InPos.x <= iter->GetLine().LeftPoint.x)
					{
						OutResult.OutPos.x = iter->GetLine().LeftPoint.x - HalfSize.x;
					}
					if (InPos.x >= iter->GetLine().LeftPoint.x)
					{
						OutResult.OutPos.x = iter->GetLine().LeftPoint.x + HalfSize.x;
					}

					OutResult.LineType = iter->GetLineType();
					Target = iter;
				}			
			}
		}
	}

	if (Target == nullptr)
		return false;

	return true;
}

bool LineManager::CollisionCeilingLine(FPOINT InPos, FLineResult& OutResult, float tolerance)
{
	if (LineList[(int)ELineType::Ceiling].empty())
		return false;

	// 직선의 방정식으로 라인을 태우자.
	// 캐릭터의 X 값으로 높이를 알 수 있다.
	// 두점을 사용해 직선의 방정식을 구하자.
	const float HalfTolerance = tolerance * 0.5f;
	Line* Target = nullptr;

	for (auto& iter : LineList[(int)ELineType::Ceiling])
	{
		// X Offset
		// 해당 라인 안에 있다면
		if (InPos.x >= iter->GetLine().LeftPoint.x && InPos.x <= iter->GetLine().RightPoint.x)
		{
			float x1 = iter->GetLine().LeftPoint.x;
			float y1 = iter->GetLine().LeftPoint.y;

			float x2 = iter->GetLine().RightPoint.x;
			float y2 = iter->GetLine().RightPoint.y;

			float y = ((y2 - y1) / (x2 - x1)) * (InPos.x - x1) + y1;

			float ObjectTop = InPos.y - HalfTolerance;

			if (ObjectTop >= y - HalfTolerance && ObjectTop <= y + HalfTolerance)
			{
				OutResult.OutPos.y = y + HalfTolerance;
				OutResult.LineType = iter->GetLineType();
				Target = iter;
			}
		}
	}

	if (Target == nullptr)
		return false;

	return true;
}

bool LineManager::CollisionDownLine(FPOINT InPos, FLineResult& OutResult, float tolerance)
{
	if (LineList[(int)ELineType::DownLine].empty())
		return false;

	const float HalfTolerance = tolerance * 0.5f;
	Line* Target = nullptr;

	for (auto& iter : LineList[(int)ELineType::DownLine])
	{
		// X Offset
		// 해당 라인 안에 있다면
		if (InPos.x >= iter->GetLine().LeftPoint.x && InPos.x <= iter->GetLine().RightPoint.x)
		{
			float x1 = iter->GetLine().LeftPoint.x;
			float y1 = iter->GetLine().LeftPoint.y;

			float x2 = iter->GetLine().RightPoint.x;
			float y2 = iter->GetLine().RightPoint.y;

			float y = ((y2 - y1) / (x2 - x1)) * (InPos.x - x1) + y1;

			float ObjectBottom = InPos.y + HalfTolerance;

			if (ObjectBottom >= y - HalfTolerance && ObjectBottom <= y + HalfTolerance)
			{
				OutResult.OutPos.y = y - HalfTolerance;
				OutResult.LineType = iter->GetLineType();
				Target = iter;
			}
		}
	}

	if (Target == nullptr)
		return false;

	return true;
}

HRESULT LineManager::SaveFile()
{
	HANDLE hFile = CreateFile(
		L"TestLineData.dat", GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		MessageBox(g_hWnd, L"LineManager SaveFile Failed.", TEXT("경고"), MB_OK);
		return E_FAIL;
	}

	DWORD dwByte = 0;
	for (int i = 0; i < (int)ELineType::End; ++i)
	{
		LINE SaveLine;
		ELineType SaveLineType;
		for (auto& iter : LineList[i])
		{
			SaveLine = iter->GetLine();
			SaveLineType = iter->GetLineType();
			WriteFile(hFile, &SaveLine,sizeof(LINE), &dwByte, NULL);
			WriteFile(hFile, &SaveLineType, sizeof(ELineType), &dwByte, NULL);
		}
	}
	
	CloseHandle(hFile);

	return S_OK;
}

HRESULT LineManager::LoadFile()
{
	return S_OK;
}

void LineManager::Render(HDC hdc)
{
	bStraight = KeyManager::GetInstance()->IsStayKeyDown(VK_SHIFT);

	if (LinePoint[LEFT].x && LinePoint[LEFT].y)
	{
		HPEN hPen = nullptr;
		POINT pt;
		pt = g_ptMouse;

		if (bStraight && CurrentLineType != ELineType::Wall)
			pt.y = LinePoint[LEFT].y;

		switch (CurrentLineType)
		{
		case ELineType::Normal:
			hPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0)); // 초록
			break;
		case ELineType::Wall:
			hPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0)); // 빨강
			pt.x = LinePoint[LEFT].x;
			break;
		case ELineType::DownLine:
			hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 255)); // 블루
			break;
		case ELineType::Ceiling:
			hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 0)); // 노랑
			break;
		}

		HPEN hOldPen = (HPEN)SelectObject(hdc, hPen); // 현재 DC에 펜을 설정
		const FPOINT Scroll = ScrollManager::GetInstance()->GetScroll();				

		MoveToEx(hdc, LinePoint[LEFT].x + Scroll.x, LinePoint[LEFT].y + Scroll.y, nullptr);
		LineTo(hdc, pt.x, pt.y);

		// 사용한 펜을 원래대로 복원
		SelectObject(hdc, hOldPen);

		// 펜 메모리 해제
		DeleteObject(hPen);
	}

	for (int i = 0; i < (int)ELineType::End; ++i)
	{
		for (auto& iter : LineList[i])
			iter->Render(hdc);
	}	
}

void LineManager::Release()
{
	for (int i = 0; i < (int)ELineType::End; ++i)
	{
		for (auto& iter : LineList[i])
			delete iter;

		LineList[i].clear();
	}

	ReleaseInstance();
}


