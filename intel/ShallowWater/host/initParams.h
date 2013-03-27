/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/*
 * @file initParams.h
 *
 * @brief Defines constants that are used by the host application
 */
#define AUXILIARY_STRING_LENGTH (100)


// UI Controls
#define BUTTON_WIDTH            (35)
#define STATIC_WIDTH            (125)
#define SLIDER_WIDTH            (140)
#define COMBOBOX_WIDTH          (140)
#define CHECKBOX_WIDTH          (140)
#define CONTROLS_HEIGHT         (22)
#define CONTROLS_OFFSET         (24)
#define SLIDER_OFFSET           (0)
#define BUTTON_OFFSET           (35)
#define STATIC_OFFSET           (35)
#define COMBOBOX_OFFSET         (0)
#define CHECKBOX_OFFSET         (0)

// Noise factors
#define MIN_NOISE                   (1.0f)
#define MAX_NOISE                   (2.0f)
#define INITIAL_NOISE               (1.5f)
#define TIME_COHERENSE_NOISE        (50.0f)
#define SPACE_COHERENSE_NOISE        (5.0f)

// Gradient thresholds
#define MIN_GRADIENT_THRESHOLD      (0.1f)
#define MAX_GRADIENT_THRESHOLD      (2.0f)
#define INITIAL_GRADIENT_THRESHOLD  (1.1f)

// Wave velocity thresholds
#define MIN_WAVE_VEL_THRESHOLD          (1.5f)
#define MAX_WAVE_VEL_THRESHOLD          (8.0f)
#define INITIAL_WAVE_VEL_THRESHOLD      (4.0f)

// Force multipliers
#define MIN_FORCE_MULTIPLIER         (0.0f)
#define MAX_FORCE_MULTIPLIER         (1.0f)
#define INITIAL_FORCE_MULTIPLIER     (1.0f)

// Depth
#define MIN_MAX_DEPTH                (0.1f)
#define MAX_MAX_DEPTH                (140.0f)
#define INITIAL_MAX_DEPTH            (70.0f)

// Scale
#define MIN_LOCAL_NORMAL_SCALE       (0.1f)
#define MAX_LOCAL_NORMAL_SCALE       (10.0f)
#define INITIAL_LOCAL_NORMAL_SCALE   (2.0f)

// Reset latency
#define MIN_RESET_LATENCY            (0.05f)
#define MAX_RESET_LATENCY            (20.0f)
#define INITIAL_RESET_LATENCY        (5.0f)

// Omega values
#define MIN_OMEGA                   (1.5f)
#define MAX_OMEGA                   (3.0f)
#define INITIAL_OMEGA               (2.0f)

// Amplitude values
#define MIN_AMPL                    (0.0f)
///#define MAX_AMPL                    (2.0f)
#define MAX_AMPL                    (3.0f)
#define INITIAL_AMPL                (1.5f)

// Level of detail values
#define MIN_LOD                     (0.0f)
#define MAX_LOD                     (1.0f)
#define INITIAL_LOD                 (1.0f)

// Wind waves
#define MIN_WW_NUMBER               (1.0f)
#define MAX_WW_NUMBER               (50.0f)
#define INITIAL_WW_NUMBER           (3.0f)

#define MIN_WW_LENGTH               (1.0f)
#define MAX_WW_LENGTH               (200.0f)
#define INITIAL_WW_LENGTH           (3.0f)

#define MIN_WW_SPEED                (1.0f)
#define MAX_WW_SPEED                (200.0f)
#define INITIAL_WW_SPEED            (3.0f)

#define MIN_WW_AMPLITUDE            (0.01f)
#define MAX_WW_AMPLITUDE            (2.0f)
#define INITIAL_WW_AMPLITUDE        (0.2f)

#define MIN_WW_VSHAPE        (0.00f)
#define MAX_WW_VSHAPE        (1.0f)
#define INITIAL_WW_VSHAPE    (1.0f)

