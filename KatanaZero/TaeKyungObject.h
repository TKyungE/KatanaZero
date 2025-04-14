#pragma once
#include "GameObject.h"

class Image;
class Collider;
class TaeKyungObject : public GameObject
{
public:
	TaeKyungObject();
	virtual ~TaeKyungObject() = default;

	virtual HRESULT Init();
	virtual void Release();
	virtual void Update();
	virtual void Render(HDC hdc);

private:
	void Move();
	void Jump();
	void Collision();

	void InitOffset();
	void Offset();
private:
	Image* Image;
	Collider* ObjectCollider;
	float Speed;

	//Jump
	float gravity;
	float dy;
	bool bJump;
	bool bFalling;
};

