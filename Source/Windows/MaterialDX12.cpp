#include "MaterialDX12.h"

const MaterialDX12 MaterialDX12::Red =
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.1f, 0.0f, 0.0f, 1.0f },
    { 1.0f, 0.0f, 0.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    128.0f
};

const MaterialDX12 MaterialDX12::Green =
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.1f, 0.0f, 1.0f },
    { 0.0f, 1.0f, 0.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    128.0f
};

const MaterialDX12 MaterialDX12::Blue =
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.1f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    128.0f
};

const MaterialDX12 MaterialDX12::Cyan =
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.1f, 0.1f, 1.0f },
    { 0.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    128.0f
};

const MaterialDX12 MaterialDX12::Magenta =
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.1f, 0.0f, 0.1f, 1.0f },
    { 1.0f, 0.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    128.0f
};

const MaterialDX12 MaterialDX12::Yellow =
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.1f, 0.1f, 1.0f },
    { 0.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    128.0f
};

const MaterialDX12 MaterialDX12::White =
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.1f, 0.1f, 0.1f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    128.0f
};

const MaterialDX12 MaterialDX12::Black =
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    128.0f
};

const MaterialDX12 MaterialDX12::Emerald = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0215f, 0.1745f, 0.0215f, 1.0f },
    { 0.07568f, 0.61424f, 0.07568f, 1.0f },
    { 0.633f, 0.727811f, 0.633f, 1.0f },
    76.8f
};

const MaterialDX12 MaterialDX12::Jade = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.135f, 0.2225f, 0.1575f, 1.0f },
    { 0.54f, 0.89f, 0.63f, 1.0f },
    { 0.316228f, 0.316228f, 0.316228f, 1.0f },
    12.8f
};

const MaterialDX12 MaterialDX12::Obsidian = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.05375f, 0.05f, 0.06625f, 1.0f },
    { 0.18275f, 0.17f, 0.22525f, 1.0f },
    { 0.332741f, 0.328634f, 0.346435f, 1.0f },
    38.4f
};

const MaterialDX12 MaterialDX12::Pearl = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.25f, 0.20725f, 0.20725f, 1.0f },
    { 1.0f, 0.829f, 0.829f, 1.0f },
    { 0.296648f, 0.296648f, 0.296648f, 1.0f },
    11.264f
};

const MaterialDX12 MaterialDX12::Ruby = {
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.1745f, 0.01175f, 0.01175f, 1.0f },
    { 0.61424f, 0.04136f, 0.04136f, 1.0f },
    { 0.727811f, 0.626959f, 0.626959f, 1.0f },
    76.8f
};

const MaterialDX12 MaterialDX12::Turquoise = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.1f, 0.18725f, 0.1745f, 1.0f },
    { 0.396f, 0.74151f, 0.69102f, 1.0f },
    { 0.297254f, 0.30829f, 0.306678f, 1.0f },
    12.8f
};

const MaterialDX12 MaterialDX12::Brass = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.329412f, 0.223529f, 0.027451f, 1.0f },
    { 0.780392f, 0.568627f, 0.113725f, 1.0f },
    { 0.992157f, 0.941176f, 0.807843f, 1.0f },
    27.9f
};

const MaterialDX12 MaterialDX12::Bronze = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.2125f, 0.1275f, 0.054f, 1.0f },
    { 0.714f, 0.4284f, 0.18144f, 1.0f },
    { 0.393548f, 0.271906f, 0.166721f, 1.0f },
    25.6f
};

const MaterialDX12 MaterialDX12::Chrome = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.25f, 0.25f, 0.25f, 1.0f },
    { 0.4f, 0.4f, 0.4f, 1.0f },
    { 0.774597f, 0.774597f, 0.774597f, 1.0f },
    76.8f
};

const MaterialDX12 MaterialDX12::Copper = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.19125f, 0.0735f, 0.0225f, 1.0f },
    { 0.7038f, 0.27048f, 0.0828f, 1.0f },
    { 0.256777f, 0.137622f, 0.086014f, 1.0f },
    12.8f
};

const MaterialDX12 MaterialDX12::Gold = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.24725f, 0.1995f, 0.0745f, 1.0f },
    { 0.75164f, 0.60648f, 0.22648f, 1.0f },
    { 0.628281f, 0.555802f, 0.366065f, 1.0f },
    51.2f
};

const MaterialDX12 MaterialDX12::Silver = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.19225f, 0.19225f, 0.19225f, 1.0f },
    { 0.50754f, 0.50754f, 0.50754f, 1.0f },
    { 0.508273f, 0.508273f, 0.508273f, 1.0f },
    51.2f
};

const MaterialDX12 MaterialDX12::BlackPlastic = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.01f, 0.01f, 0.01f, 1.0f },
    { 0.5f, 0.5f, 0.5f, 1.0f },
    32.0f
};

const MaterialDX12 MaterialDX12::CyanPlastic = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.1f, 0.06f, 1.0f },
    { 0.0f, 0.50980392f, 0.50980392f, 1.0f },
    { 0.50196078f, 0.50196078f, 0.50196078f, 1.0f },
    32.0f
};

const MaterialDX12 MaterialDX12::GreenPlastic = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.1f, 0.35f, 0.1f, 1.0f },
    { 0.45f, 0.55f, 0.45f, 1.0f },
    32.0f
};

const MaterialDX12 MaterialDX12::RedPlastic = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.5f, 0.0f, 0.0f, 1.0f },
    { 0.7f, 0.6f, 0.6f, 1.0f },
    32.0f
};

const MaterialDX12 MaterialDX12::WhitePlastic = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.55f, 0.55f, 0.55f, 1.0f },
    { 0.7f, 0.7f, 0.7f, 1.0f },
    32.0f
};

const MaterialDX12 MaterialDX12::YellowPlastic = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.5f, 0.5f, 0.0f, 1.0f },
    { 0.6f, 0.6f, 0.5f, 1.0f },
    32.0f
};

const MaterialDX12 MaterialDX12::BlackRubber = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.02f, 0.02f, 0.02f, 1.0f },
    { 0.01f, 0.01f, 0.01f, 1.0f },
    { 0.4f, 0.4f, 0.4f, 1.0f },
    10.0f
};

const MaterialDX12 MaterialDX12::CyanRubber = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.05f, 0.05f, 1.0f },
    { 0.4f, 0.5f, 0.5f, 1.0f },
    { 0.04f, 0.7f, 0.7f, 1.0f },
    10.0f
};

const MaterialDX12 MaterialDX12::GreenRubber = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.05f, 0.0f, 1.0f },
    { 0.4f, 0.5f, 0.4f, 1.0f },
    { 0.04f, 0.7f, 0.04f, 1.0f },
    10.0f
};

const MaterialDX12 MaterialDX12::RedRubber = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.05f, 0.0f, 0.0f, 1.0f },
    { 0.5f, 0.4f, 0.4f, 1.0f },
    { 0.7f, 0.04f, 0.04f, 1.0f },
    10.0f
};

const MaterialDX12 MaterialDX12::WhiteRubber = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.05f, 0.05f, 0.05f, 1.0f },
    { 0.5f, 0.5f, 0.5f, 1.0f },
    { 0.7f, 0.7f, 0.7f, 1.0f },
    10.0f
};

const MaterialDX12 MaterialDX12::YellowRubber = 
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.05f, 0.05f, 0.0f, 1.0f },
    { 0.5f, 0.5f, 0.4f, 1.0f },
    { 0.7f, 0.7f, 0.04f, 1.0f },
    10.0f
};
