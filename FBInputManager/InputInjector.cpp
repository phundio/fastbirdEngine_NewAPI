#include "stdafx.h"
#include "InputInjector.h"
#include "IKeyboard.h"
#include "IMouse.h"
#include "InputManager.h"
using namespace fastbird;

class InputInjector::InputInjectorImpl{
public:
	IKeyboardPtr mKeyboard;
	IMousePtr mMouse;
};

InputInjector::InputInjector()
	:mImpl(new InputInjectorImpl){

}

InputInjector::~InputInjector(){
	delete mImpl;
}

//---------------------------------------------------------------------------
//-------------------------------------------------------------------
// IInputInjector
//-------------------------------------------------------------------
bool InputInjector::IsValid(FBInputDevice::Enum type) const{
	auto im = InputManager::GetInstance();
	if (im){
		return im->IsValid(type);
	}
	return false;
}
void InputInjector::Invalidate(FBInputDevice::Enum type) const{
	auto im = InputManager::GetInstance();
	if (im){
		im->Invalidate(type);
	}
}
void InputInjector::InvalidTemporary(FBInputDevice::Enum type, bool invalidate){
	auto im = InputManager::GetInstance();
	if (im){
		im->InvalidTemporary(type, invalidate);
	}
}

//-------------------------------------------------------------------
// Keyboard
//-------------------------------------------------------------------
bool InputInjector::IsKeyDown(unsigned short keycode) const{
	if (mImpl->mKeyboard)
		return mImpl->mKeyboard->IsKeyDown(keycode);
	return false;
}

bool InputInjector::IsKeyPressed(unsigned short keycode) const{
	if (mImpl->mKeyboard)
		return mImpl->mKeyboard->IsKeyPressed(keycode);
	return false;
}

bool InputInjector::IsKeyUp(unsigned short keycode) const{
	if (mImpl->mKeyboard)
		return mImpl->mKeyboard->IsKeyUp(keycode);
	return false;
}

unsigned InputInjector::GetChar(){
	if (mImpl->mKeyboard)
		return mImpl->mKeyboard->GetChar();
	return false;
}

void InputInjector::PopChar(){
	if (mImpl->mKeyboard)
		mImpl->mKeyboard->PopChar();
}
void InputInjector::ClearBuffer(){
	if (mImpl->mKeyboard)
		mImpl->mKeyboard->ClearBuffer();
}

//-------------------------------------------------------------------
// Mouse
//-------------------------------------------------------------------
// Positions
void InputInjector::GetHDDeltaXY(long &x, long &y) const{
	if (mImpl->mMouse)
		mImpl->mMouse->GetHDDeltaXY(x, y);
}

void InputInjector::GetDeltaXY(long &x, long &y) const{
	if (mImpl->mMouse)
		mImpl->mMouse->GetDeltaXY(x, y);
}

CoordinatesI InputInjector::GetDeltaXY() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->GetDeltaXY();
	return{ 0, 0 };
}

void InputInjector::GetMousePos(long &x, long &y) const{
	if (mImpl->mMouse)
		mImpl->mMouse->GetPos(x, y);
}

CoordinatesI InputInjector::GetMousePos() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->GetPos();
	return{ 0, 0 };
}

void InputInjector::GetMousePrevPos(long &x, long &y) const{
	if (mImpl->mMouse)
		mImpl->mMouse->GetPrevPos(x, y);
}

void InputInjector::GetMouseNPos(Real &x, Real &y) const{ // normalized pos(0.0~1.0)
	if (mImpl->mMouse)
		mImpl->mMouse->GetNPos(x, y);
}

CoordinatesR InputInjector::GetMouseNPos() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->GetNPos();
	return{ 0., 0. };
}

bool InputInjector::IsMouseMoved() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsMoved();
	return false;
}

void InputInjector::LockMousePos(bool lock, void* key){
	if (mImpl->mMouse)
		mImpl->mMouse->LockMousePos(lock, key);
}

bool InputInjector::IsMouseIn(int left, int top, int right, int bottom){
	if (mImpl->mMouse)
		return mImpl->mMouse->IsIn(left, top, right, bottom);
	return false;
}

Real InputInjector::GetSensitivity() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->GetSensitivity();
	return 0;
}

// Dragging
void InputInjector::GetDragStart(long &x, long &y) const{
	if (mImpl->mMouse)
		mImpl->mMouse->GetDragStart(x, y);
}

CoordinatesI InputInjector::GetDragStartedPos() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->GetDragStartedPos();
	return{ 0, 0 };
}

bool InputInjector::IsDragStartIn(int left, int top, int right, int bottom) const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsDragStartIn(left, top, right, bottom);
	return false;
}

bool InputInjector::IsDragStarted(int& outX, int& outY) const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsDragStarted(outX, outY);
	return false;
}

bool InputInjector::IsDragEnded() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsDragEnded();
	return false;
}

void InputInjector::PopDragEvent(){
	if (mImpl->mMouse)
		mImpl->mMouse->PopDragEvent();
}

bool InputInjector::IsRDragStarted(int& outX, int& outY) const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsRDragStarted(outX, outY);
	return false;
}

bool InputInjector::IsRDragEnded(int& outX, int& outY) const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsRDragEnded(outX, outY);
	return false;
}

void InputInjector::PopRDragEvent(){
	if (mImpl->mMouse)
		mImpl->mMouse->PopRDragEvent();
}

// Buttons
bool InputInjector::IsLButtonDownPrev() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsLButtonDownPrev();
	return false;
}

bool InputInjector::IsLButtonDown(Real* time) const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsLButtonDown(time);
	return false;
}

bool InputInjector::IsLButtonClicked() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsLButtonClicked();
	return false;
}

bool InputInjector::IsLButtonDoubleClicked() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsLButtonDoubleClicked();
	return false;
}

bool InputInjector::IsLButtonPressed() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsLButtonPressed();
	return false;
}

bool InputInjector::IsRButtonDown(Real* time) const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsRButtonDown(time);
	return false;
}

bool InputInjector::IsRButtonDownPrev() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsRButtonDownPrev();
	return false;
}

bool InputInjector::IsRButtonClicked() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsRButtonClicked();
	return false;
}

bool InputInjector::IsRButtonPressed() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsRButtonPressed();
	return false;
}

bool InputInjector::IsMButtonDown() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->IsMButtonDown();
	return false;
}

void InputInjector::ClearButton(){
	if (mImpl->mMouse)
		mImpl->mMouse->ClearButton();
}

void InputInjector::NoClickOnce(){
	if (mImpl->mMouse)
		mImpl->mMouse->NoClickOnce();
}

void InputInjector::ClearRightDown(){
	if (mImpl->mMouse)
		mImpl->mMouse->ClearRightDown();
}

Real InputInjector::GetLButtonDownTime() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->GetLButtonDownTime();
	return 0;
}


// Wheel		
long InputInjector::GetWheel() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->GetWheel();
	return 0;
}

void InputInjector::PopWheel(){
	if (mImpl->mMouse)
		mImpl->mMouse->PopWheel();
}

void InputInjector::ClearWheel(){
	if (mImpl->mMouse)
		mImpl->mMouse->ClearWheel();
}

Real InputInjector::GetWheelSensitivity() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->GetWheelSensitivity();
	return 0.;
}

unsigned long InputInjector::GetNumLinesWheelScroll() const{
	if (mImpl->mMouse)
		return mImpl->mMouse->GetNumLinesWheelScroll();
	return 0;
}