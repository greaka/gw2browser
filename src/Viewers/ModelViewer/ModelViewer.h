/* \file       Viewers/ModelViewer.h
*  \brief      Contains the declaration of the model viewer class.
*  \author     Khral Steelforge
*/

/*
Copyright (C) 2015-2017 Khral Steelforge <https://github.com/kytulendu>
Copyright (C) 2012 Rhoot <https://github.com/rhoot>

This file is part of Gw2Browser.

Gw2Browser is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifndef VIEWERS_MODELVIEWER_MODELVIEWER_H_INCLUDED
#define VIEWERS_MODELVIEWER_MODELVIEWER_H_INCLUDED

#include <vector>

#include "Camera.h"
#include "FrameBuffer.h"
#include "Light.h"
#include "LightBox.h"
#include "Model.h"
#include "Shader.h"
#include "Text2D.h"

#include "DatFile.h"
#include "ViewerGLCanvas.h"
#include "Readers/ModelReader.h"

namespace gw2b {

	class ModelViewer;
	class RenderTimer : public wxTimer {
		ModelViewer* canvas;
	public:
		RenderTimer( ModelViewer* canvas );
		void Notify( );
		void start( );
	}; // class RenderTimer

	class ModelViewer : public ViewerGLCanvas {
		DatFile&                    m_datFile;

		wxGLContext*				m_glContext;
		std::unique_ptr<FrameBuffer> m_framebuffer;
		RenderTimer*				m_renderTimer;
		wxTimer*					m_movementKeyTimer;

		wxSize						m_clientSize;

		// Internal status
		bool						m_glInitialized = false;			// Is OpenGL is initialized?
		bool						m_statusText = true;				// Toggle display text
		bool						m_statusWireframe = false;			// Toggle wireframe rendering
		bool						m_statusCullFace = false;			// Cull triangles which normal is not towards the camera
		bool						m_statusTextured = true;			// Toggle texture
		bool						m_statusNormalMapping = true;		// Toggle normal maping
		bool						m_statusLighting = true;			// Toggle lighting
		bool						m_statusRenderLightSource = false;	// Toggle visualization of light source
		bool						m_statusVisualizeNormal = false;	// Toggle visualization of normal
		bool						m_statusVisualizeZbuffer = false;	// Toggle visualization of z-buffer
		bool						m_cameraMode = false;				// Toggle camera mode

		// Model
		std::vector<std::unique_ptr<Model>>	m_model;

		// Light
		Light						m_light;
		std::unique_ptr<LightBox>	m_lightBox;			// For render cube at light position

		// Shader stuff
		Shader*						m_mainShader;
		Shader*						m_normalVisualizerShader;
		Shader*						m_zVisualizerShader;
		Shader*						m_screenShader;

		// Camera
		Camera                      m_camera;
		wxPoint                     m_lastMousePos;
		float                       m_minDistance;
		float                       m_maxDistance;

		// Text rendering stuff
		std::unique_ptr<Text2D>		m_text;

		//float angle = 0.0f;

		// Frame time
		GLfloat						m_deltaTime = 0.0f;
		GLfloat						m_lastFrame = 0.0f;

	public:
		/** Constructor. Creates the model viewer with the given parent.
		*  \param[in]  p_parent     Parent of the control.
		*  \param[in]  p_pos        Optional position of the control.
		*  \param[in]  p_size       Optional size of the control. */
		ModelViewer( wxWindow* p_parent, const int* p_attrib, const wxPoint& p_pos, const wxSize& p_size, long p_style, DatFile& p_datFile );
		/** Destructor. */
		virtual ~ModelViewer( );

		/** Clear the viewer. */
		virtual void clear( ) override;
		virtual void setReader( FileReader* p_reader ) override;
		/** Gets the model reader containing the data displayed by this viewer.
		*  \return ModelReader*    Reader containing the data. */
		ModelReader* modelReader( ) {
			return reinterpret_cast<ModelReader*>( this->reader( ) );
		} // already asserted with a dynamic_cast
		/** Gets the model reader containing the data displayed by this viewer.
		*  \return ModelReader*    Reader containing the data. */
		const ModelReader* modelReader( ) const {
			return reinterpret_cast<const ModelReader*>( this->reader( ) );
		} // already asserted with a dynamic_cast

	private:
		void clearShader( );
		void loadShader( );
		void reloadShader( );
		int initGL( );
		void onPaintEvt( wxPaintEvent& p_event );
		void render( );
		void drawModel( Shader* p_shader, const glm::mat4& p_trans );
		void displayStatusText( );
		void updateMatrices( );
		void focus( );
		void createFrameBuffer( );
		void onMotionEvt( wxMouseEvent& p_event );
		void onMouseWheelEvt( wxMouseEvent& p_event );
		void onKeyDownEvt( wxKeyEvent& p_event );
		void onMovementKeyTimerEvt( wxTimerEvent&p_event );
		void onClose( wxCloseEvent& evt );
		void onResize( wxSizeEvent& evt );

	}; // class ModelViewer

}; // namespace gw2b

#endif // VIEWERS_MODELVIEWER_MODELVIEWER_H_INCLUDED
