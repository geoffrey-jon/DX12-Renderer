/*  ===================================================
	Direct3D 12 Project-Specific Application Definition
	===================================================  */

#ifndef MYAPP_H
#define MYAPP_H

#include "D3DApp.h"

class MyApp : public D3DApp
{
public:
	MyApp(HINSTANCE Instance);
	~MyApp();

	bool Init() override;

	void OnResize() override;
	void Update(const GameTimer& gt) override;
	void Draw(const GameTimer& gt) override;
};

#endif // MYAPP_H