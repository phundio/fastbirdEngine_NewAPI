#include "stdafx.h"
#include "Color.h"
#include "FBStringLib/StringLib.h"
#include "FBStringMathLib/StringMathConverter.h"
#include "FBStringLib/StringConverter.h"

namespace fastbird
{
const Color Color::White(1, 1, 1);
const Color Color::Black(0, 0, 0);
const Color Color::Red(1, 0, 0);
const Color Color::BrightRed(1, 0.2, 0.2);
const Color Color::DarkGray(0.15f, 0.15f, 0.15f);
const Color Color::Gray(0.5f, 0.5f, 0.5f);
const Color Color::Silver(0.8f, 0.8f, 0.8f);
const Color Color::Green(0, 1, 0);
const Color Color::Yellow(1, 1, 0);
const Color Color::Blue(0, 0, 1);
const Color Color::SkyBlue(0, 0.5f, 1);
const Color Color::Zero(0, 0, 0, 0);

Color::Color(){
}

Color::Color(const Vec3& color)
	: mValue(color, 1.0f)
{
}

Color::Color(const Vec4& color)
	: mValue(color) 
{
}

Color::Color(Real r, Real g, Real b, Real a)
	: mValue(r, g, b, a) 
{
}

Color::Color(Real r, Real g, Real b)
	: mValue(r, g, b, 1.f)
{
}

Color::Color(const char* str)
{
	if (str[0] == '0' && str[1] == 'x')
	{
		if (strlen(str) == 8)
		{
			TString strColor = str;
			strColor += _T("ff");
			*this = Color(Color::FixColorByteOrder(StringConverter::ParseHexa(strColor.c_str())));
		}
		else
		{
			*this = Color(Color::FixColorByteOrder(StringConverter::ParseHexa(str)));
		}
	}
	else
	{
		mValue = StringMathConverter::ParseVec4(str, Vec4::ZERO);
	}
}

Color::Color(unsigned int c)
{
	RGBA* rgba = (RGBA*)&c;
	mValue.x = rgba->r / 255.0f;
	mValue.y = rgba->g / 255.0f;
	mValue.z = rgba->b / 255.0f;
	mValue.w = rgba->a / 255.0f;
}

// when you want to send the data to gpu
unsigned int Color::Get4Byte() const
{
	RGBA color;
	color.r = BYTE(mValue.x * 255.f);
	color.g = BYTE(mValue.y * 255.f);
	color.b = BYTE(mValue.z * 255.f);
	color.a = BYTE(mValue.w * 255.f);
	return *(unsigned int*)&color;
}

// when you want to create hexa string
unsigned int Color::Get4ByteReversed() const
{
	RGBA color;
	color.r = BYTE(mValue.w * 255.f);
	color.g = BYTE(mValue.z * 255.f);
	color.b = BYTE(mValue.y * 255.f);
	color.a = BYTE(mValue.x * 255.f);
	return *(unsigned int*)&color;
}

Color::operator unsigned int() const {
	return Get4Byte(); 
}

const Vec4& Color::GetVec4() const {
	return mValue; 
}

Color Color::operator* (Real scalar) const{
	return Color(mValue*scalar);
}

Color Color::operator*(const Color& other) const {
	return Color(mValue*other.mValue);
}

Color& Color::operator*= (Real scalar){
	mValue *= scalar;
	return *this;
}

Color& Color::operator*= (const Color& c)
{
	mValue *= c.mValue;
	return *this;
}

Color Color::operator+ (const Color& r) const {
	return mValue + r.GetVec4();
}

bool Color::operator== (const Color& other) const{
	return mValue == other.mValue;
}

void Color::SetColor(Real r, Real g, Real b, Real a){
	mValue.x = r;
	mValue.y = g;
	mValue.z = b;
	mValue.w = a;
}

Real Color::r() const {
	return mValue.x; 
}

Real Color::g() const {
	return mValue.y; 
}

Real Color::b() const {
	return mValue.z; 
}

Real Color::a() const { 
	return mValue.w; 
}

Real& Color::r(){
	return mValue.x; 
}

Real& Color::g(){
	return mValue.y; 
}

Real& Color::b(){
	return mValue.z; 
}

Real& Color::a(){ 
	return mValue.w; 
}

unsigned Color::FixColorByteOrder(unsigned c)
{
	RGBA color;
	color.r = c >> 24;
	color.g = c >> 16 & 0xff;
	color.b = c >> 8 & 0xff;
	color.a = c & 0xff;
	return *(unsigned*)&color;
}



Color Color::RandomColor(){
	return Color(Random(), Random(), Random(), 1.f);
}
inline Color Random(const Color& min, const Color& max)
{
	return Color(Random(min.r(), max.r()), Random(min.g(), max.g()),
		Random(min.b(), max.b()));
}
}
