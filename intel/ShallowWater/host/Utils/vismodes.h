/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file vismodes.h
 *
 * @brief Defines visual modes and methods
 */

#pragma once

#ifndef __INTEL_COMPILER
#pragma warning(disable : 4995)
#endif

#include <vector>

/**
 * @struct VisColor
 *
 * @brief Defines a visible color
 */
struct VisColor
{

    /**
     * @fn VisColor()
     *
     * @brief Class constructor
     */
    VisColor() 
    {

    }

    /**
     * @fn VisColor(float r_, float g_, float b_)
     *
     * @brief Parameterized class constructor
     *
     * @param r_ - Red component of the color
     *
     * @param g_ - Green component of the color
     *
     * @param b_ - Blue component of the color
     */
    VisColor(float r_, float g_, float b_) : r(r_), g(g_), b(b_) 
    {

    }

    /**
     * @fn VisColor(VisColor const &color)
     *
     * @brief Copy constructor
     *
     * @param color - The ViswColor object to copy
     */
    VisColor(VisColor const &color) : r(color.r), g(color.g), b(color.b) 
    {

    }

    /**
     * @var r
     *
     * @brief Red component of the color 
     */
    float r;

    /**
     * @var g
     *
     * @brief Green component of the color 
     */
    float g;

    /**
     * @var b
     *
     * @brief Blue component of the color 
     */
    float b;
};

/**
 * @struct VisMode
 *
 * @brief Contains the color values and name for an address in a flux grid
 */
struct VisMode
{
    /**
     * @fn ~VisMode()
     *
     * @brief Structure destructor
     */
    virtual ~VisMode() 
    {

    }

    /**
     * @fn GetColor(class CFlux2DGrid &flux, int x, int y)
     *
     * @brief Gets the color of the grid in the cell (X,Y)
     *
     * @param flux - The flux grid
     *
     * @param x - The cell X coordinate in the grid
     *
     * @param y - The cell Y coordinate in the grid
     *
     * @return - An integer representation of the color
     */
    virtual int GetColor(class CFlux2DGrid &flux, int x, int y) const = 0;

    /**
     * @fn GetName()
     *
     * @brief Gets the name of the color calculator
     *
     * @return - The name of the color calculator
     */
    virtual wchar_t const * GetName() const = 0;
};

/**
 * @class VisModeValue
 *
 * @brief Defines base class for color calculator 
 */
class VisModeValue : 
    public VisMode
{
public: 

    /**
     * @fn VisModeValue(float maxValue)
     *
     * @brief Class constructor
     *
     * @param maxValue - The saturation value
     */
    VisModeValue(float maxValue) 
        : m_a(255.0f / (2.0f * maxValue)),
        m_b(127.5f)
    {

    }

protected:

    /**
     * @var m_a
     *
     * @brief scale coefficient for value/color transform
     */
    float m_a;

    /**
     * @var m_b
     *
     * @brief zero value color
     */
    float m_b;
};


/**
 * @class VisModeLevel
 *
 * @brief water level visualization color calculator
 */
class VisModeLevel : 
    public VisModeValue
{
public: 

    /**
     * @fn VisModeLevel(float maxHeight)
     *
     * @brief Class constructor
     *
     * @param maxHeight - The color saturation value 
     */
    VisModeLevel(float maxHeight) : VisModeValue(maxHeight)
    {

    }

    /**
     * @fn GetColor(class CFlux2DGrid &flux, int x, int y)
     *
     * @brief Gets the color of the grid in the cell (X,Y)
     *
     * @param flux - The flux grid
     *
     * @param x - The cell X coordinate in the grid
     *
     * @param y - The cell Y coordinate in the grid
     *
     * @return - An integer representation of the color
     */
    virtual int GetColor(class CFlux2DGrid &flux, int x, int y) const;

    /**
     * @fn GetName()
     *
     * @brief Gets the name of the level
     *
     * @return - The name of the level
     */
    virtual wchar_t const * GetName() const 
    { 
        return L"Level"; 
    }
};


/**
 * @class VisModeLevelColored
 *
 * @brief color calculator for water level visualization
 */
class VisModeLevelColored : 
    public VisModeValue
{
public: 

    /**
     * @fn VisModeLevelColored(float maxHeight, VisColor const &color)
     *
     * @brief Class constructor
     *
     * @param maxHeight
     *
     * @param color - The color
     */
    VisModeLevelColored(float maxHeight, VisColor const &color) 
        : VisModeValue(maxHeight),
        m_color(color)
    {
    }

    /**
     * @fn GetColor(class CFlux2DGrid &flux, int x, int y)
     *
     * @brief Gets the color of the grid in the cell (X,Y)
     *
     * @param flux - The flux grid
     *
     * @param x - The cell X coordinate in the grid
     *
     * @param y - The cell Y coordinate in the grid
     *
     * @return - An integer representation of the color
     */
    virtual int GetColor(class CFlux2DGrid &flux, int x, int y) const;

    /**
     * @fn GetName()
     *
     * @brief Gets the name of the level
     *
     * @return - The name of the level
     */
    virtual wchar_t const * GetName() const 
    { 
        return L"LevelC"; 
    }
private:

    /**
     * @var m_color
     *
     * @brief The visual color
     */
    VisColor m_color;
};


/**
 * @class VisModeHeightColored
 *
 * @brief color calculator for water height map visualization
 */
class VisModeHeightColored : 
    public VisMode
{
public: 

    /**
     * @fn VisModeHeightColored( float maxHeight, VisColor const &colLo, VisColor const &colHi, VisColor const &colG )
     *
     * @brief Class constructor
     *
     * @param maxHeight
     *
     * @param colLo - The color for low saturation values
     *
     * @param colHi - The color for high saturation values
     *
     * @param colG - The color at ground level
     */
    VisModeHeightColored( float maxHeight, 
        VisColor const &colLo, 
        VisColor const &colHi, 
        VisColor const &colG )
        : m_rcpMaxHeight(1.0f / maxHeight),
        m_colorLo(colLo),
        m_colorHi(colHi),
        m_colorGround(colG)
    {

    }

    /**
     * @fn GetColor(class CFlux2DGrid &flux, int x, int y)
     *
     * @brief Gets the color of the grid in the cell (X,Y)
     *
     * @param flux - The flux grid
     *
     * @param x - The cell X coordinate in the grid
     *
     * @param y - The cell Y coordinate in the grid
     *
     * @return - An integer representation of the color
     */
    virtual int GetColor(class CFlux2DGrid &flux, int x, int y) const;

    /**
     * @fn GetName()
     *
     * @brief Gets the name of the level
     *
     * @return - The name of the level
     */
    virtual wchar_t const * GetName() const 
    { 
        return L"HeightC"; 
    }
private:

    /**
     * @var m_rcpMaxHeight
     *
     * @brief Recipricol of the maximum height
     */
    float    m_rcpMaxHeight;

    /**
     * @var m_colorLo
     *
     * @brief Color for low saturation values
     */
    VisColor m_colorLo;

    /**
     * @var m_colorHi
     *
     * @brief Color for high saturation values
     */
    VisColor m_colorHi;

    /**
     * @var m_colorGround
     *
     * @brief Color on the ground
     */
    VisColor m_colorGround;
};


/**
 * @class VisModeSpeed
 *
 * @brief color calculator for water speed map visualization
 */
class VisModeSpeed : 
    public VisModeValue
{
public: 

    /**
     * @fn VisModeSpeed(float maxSpeed, bool visCritical = false)
     *
     * @brief Class constructor
     *
     * @param maxSpeed - color saturation speed value
     *
     * @param visCritical - Boolean variable indicating criticality
     */
    VisModeSpeed(float maxSpeed, bool visCritical = false) 
        : VisModeValue(maxSpeed)
        , m_visCritical(visCritical)
    {
    }

    /**
     * @fn GetColor(class CFlux2DGrid &flux, int x, int y)
     *
     * @brief Gets the color of the grid in the cell (X,Y)
     *
     * @param flux - The flux grid
     *
     * @param x - The cell X coordinate in the grid
     *
     * @param y - The cell Y coordinate in the grid
     *
     * @return - An integer representation of the color
     */
    virtual int GetColor(class CFlux2DGrid &flux, int x, int y) const;

    /**
     * @fn GetName()
     *
     * @brief Gets the name of the level
     *
     * @return - The name of the level
     */
    virtual wchar_t const * GetName() const 
    { 
        return L"Speed"; 
    }
private:

    /**
     * @var m_visCritical
     *
     * @brief Boolean variable to indicate whether or not use critical speed visualization
     */
    bool m_visCritical;
};


/**
 * @class VisModeMoment
 *
 * @brief color calculator for moment map visualization
 */
class VisModeMoment : 
    public VisModeValue
{
public: 

    /**
     * @fn VisModeSpeed(float maxSpeed, bool visCritical = false)
     *
     * @brief Class constructor
     *
     * @param maxMoment - color saturation moment value
     */
    VisModeMoment(float maxMoment) 
        : VisModeValue(maxMoment)
    {
    }

    /**
     * @fn GetColor(class CFlux2DGrid &flux, int x, int y)
     *
     * @brief Gets the color of the grid in the cell (X,Y)
     *
     * @param flux - The flux grid
     *
     * @param x - The cell X coordinate in the grid
     *
     * @param y - The cell Y coordinate in the grid
     *
     * @return - An integer representation of the color
     */
    virtual int GetColor(class CFlux2DGrid &flux, int x, int y) const;

    /**
     * @fn GetName()
     *
     * @brief Gets the name of the color visualization calculator
     *
     * @return - The name of the color visualization calculator
     */
    virtual wchar_t const * GetName() const 
    { 
        return L"Moment"; 
    }
};


/**
 * @class VisModeCritical
 *
 * @brief color calculator for critical speed map visualization
 */
class VisModeCritical : 
    public VisMode
{
public: 

    /**
     * @fn GetColor(class CFlux2DGrid &flux, int x, int y)
     *
     * @brief Gets the color of the grid in the cell (X,Y)
     *
     * @param flux - The flux grid
     *
     * @param x - The cell X coordinate in the grid
     *
     * @param y - The cell Y coordinate in the grid
     *
     * @return - An integer representation of the color
     */
    virtual int GetColor(class CFlux2DGrid &flux, int x, int y) const;

    /**
     * @fn GetName()
     *
     * @brief Gets the name of the color visualization calculator
     *
     * @return - The name of the color visualization calculator
     */
    virtual wchar_t const * GetName() const 
    { 
        return L"Case"; 
    }
};


/**
 * @class VisModeFroude
 *
 * @brief  color calculator for Froude number (shallow water model characteristic value) map visualization
 */
class VisModeFroude : 
    public VisMode
{
public: 

    /**
     * @fn GetColor(class CFlux2DGrid &flux, int x, int y)
     *
     * @brief Gets the color of the grid in the cell (X,Y)
     *
     * @param flux - The flux grid
     *
     * @param x - The cell X coordinate in the grid
     *
     * @param y - The cell Y coordinate in the grid
     *
     * @return - An integer representation of the color
     */
    virtual int GetColor(class CFlux2DGrid &flux, int x, int y) const;

    /**
     * @fn GetName()
     *
     * @brief Gets the name of the object type
     *
     * @return - The name of the object type
     */
    virtual wchar_t const * GetName() const 
    { 
        return L"Froude"; 
    }
};

/**
 * @class VisModeState
 *
 * @brief color calculator for state visualization
 */
class VisModeState : 
    public VisMode
{
public: 

    /**
     * @fn GetColor(class CFlux2DGrid &flux, int x, int y)
     *
     * @brief Gets the color of the grid in the cell (X,Y)
     *
     * @param flux - The flux grid
     *
     * @param x - The cell X coordinate in the grid
     *
     * @param y - The cell Y coordinate in the grid
     *
     * @return - An integer representation of the color
     */
    virtual int GetColor(class CFlux2DGrid &flux, int x, int y) const;

    /**
     * @fn GetName()
     *
     * @brief Gets the name of the object type
     *
     * @return - The name of the object type
     */
    virtual wchar_t const * GetName() const 
    { 
        return L"State"; 
    }
};
